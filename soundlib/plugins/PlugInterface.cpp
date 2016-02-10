/*
 * PlugInterface.cpp
 * -----------------
 * Purpose: Default plugin interface implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "PlugInterface.h"
#include "PluginManager.h"
#ifdef MODPLUG_TRACKER
#include "../../mptrack/Moddoc.h"
#include "../../mptrack/Mainfrm.h"
#include "../../mptrack/InputHandler.h"
#include "../../mptrack/AbstractVstEditor.h"
#include "../../mptrack/DefaultVstEditor.h"
// LoadProgram/SaveProgram
#include "../../mptrack/FileDialog.h"
#include "../../mptrack/VstPresets.h"
#include "../../common/FileReader.h"
#include "../../common/mptFileIO.h"
#include "../mod_specifications.h"
#endif // MODPLUG_TRACKER

#ifndef NO_PLUGINS

OPENMPT_NAMESPACE_BEGIN


#ifdef MODPLUG_TRACKER
CModDoc *IMixPlugin::GetModDoc() { return m_SndFile.GetpModDoc(); }
const CModDoc *IMixPlugin::GetModDoc() const { return m_SndFile.GetpModDoc(); }
#endif // MODPLUG_TRACKER


IMixPlugin::IMixPlugin(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//-----------------------------------------------------------------------------------------
	: m_pNext(nullptr)
	, m_pPrev(nullptr)
	, m_Factory(factory)
	, m_SndFile(sndFile)
	, m_pMixStruct(mixStruct)
#ifdef MODPLUG_TRACKER
	, m_pEditor(nullptr)
#endif // MODPLUG_TRACKER
	, m_fGain(1.0f)
	, m_nSlot(0)
	, m_bSongPlaying(false)
	, m_bPlugResumed(false)
	, m_bRecordAutomation(false)
	, m_bPassKeypressesToPlug(false)
	, m_bRecordMIDIOut(false)
{
	m_MixState.dwFlags = 0;
	m_MixState.nVolDecayL = 0;
	m_MixState.nVolDecayR = 0;

	// Update Mix structure
	m_pMixStruct->pMixState = &m_MixState;
}


IMixPlugin::~IMixPlugin()
//-----------------------
{
	if(m_pEditor != nullptr)
	{
		if(m_pEditor->m_hWnd) m_pEditor->OnClose();
		delete m_pEditor;
		m_pEditor = nullptr;
	}

	if (m_pNext) m_pNext->m_pPrev = m_pPrev;
	if (m_pPrev) m_pPrev->m_pNext = m_pNext;
	m_pPrev = nullptr;
	m_pNext = nullptr;
}


void IMixPlugin::InsertIntoFactoryList()
//--------------------------------------
{
	m_pNext = m_Factory.pPluginsList;
	if(m_Factory.pPluginsList)
	{
		m_Factory.pPluginsList->m_pPrev = this;
	}
	m_Factory.pPluginsList = this;
}


PLUGINDEX IMixPlugin::FindSlot() const
//------------------------------------
{
	PLUGINDEX slot = 0;
	while(m_pMixStruct != &(m_SndFile.m_MixPlugins[slot]) && slot < MAX_MIXPLUGINS - 1)
	{
		slot++;
	}
	return slot;
}

#ifdef MODPLUG_TRACKER

CString IMixPlugin::GetFormattedParamName(PlugParamIndex param)
//-------------------------------------------------------------
{
	CString paramName = GetParamName(param);
	CString name;
	if(paramName.IsEmpty())
	{
		name.Format("%02u: Parameter %02u", param, param);
	} else
	{
		name.Format("%02u: %s", param, paramName);
	}
	return name;
}


// Get a parameter's current value, represented by the plugin.
CString IMixPlugin::GetFormattedParamValue(PlugParamIndex param)
//--------------------------------------------------------------
{

	CString paramDisplay = GetParamDisplay(param);
	CString paramUnits = GetParamLabel(param);
	paramDisplay.Trim();
	paramUnits.Trim();
	paramDisplay += _T(" ") + paramUnits;

	return paramDisplay;
}


CString IMixPlugin::GetFormattedProgramName(int32 index)
//------------------------------------------------------
{
	CString rawname = GetProgramName(index);
	
	// Let's start counting at 1 for the program name (as most MIDI hardware / software does)
	index++;

	CString formattedName;
	if((unsigned char)rawname[0] < ' ')
		formattedName.Format("%02u - Program %u", index, index);
	else
		formattedName.Format("%02u - %s", index, rawname);

	return formattedName;
}


void IMixPlugin::SetEditorPos(int32 x, int32 y)
//---------------------------------------------
{
	m_pMixStruct->editorX= x;
	m_pMixStruct->editorY = y;
}


void IMixPlugin::GetEditorPos(int32 &x, int32 &y) const
//-----------------------------------------------------
{
	x = m_pMixStruct->editorX;
	y = m_pMixStruct->editorY;
}


#endif // MODPLUG_TRACKER


bool IMixPlugin::IsBypassed() const
//---------------------------------
{
	return m_pMixStruct != nullptr && m_pMixStruct->IsBypassed();
}


void IMixPlugin::RecalculateGain()
//--------------------------------
{
	float gain = 0.1f * static_cast<float>(m_pMixStruct ? m_pMixStruct->GetGain() : 10);
	if(gain < 0.1f) gain = 1.0f;

	if(IsInstrument())
	{
		gain /= m_SndFile.GetPlayConfig().getVSTiAttenuation();
		gain = static_cast<float>(gain * (m_SndFile.m_nVSTiVolume / m_SndFile.GetPlayConfig().getNormalVSTiVol()));
	}
	m_fGain = gain;
}


void IMixPlugin::SetDryRatio(uint32 param)
//----------------------------------------
{
	param = std::min(param, uint32(127));
	m_pMixStruct->fDryRatio = 1.0f - (param / 127.0f);
}


void IMixPlugin::Bypass(bool bypass)
//----------------------------------
{
	m_pMixStruct->Info.SetBypass(bypass);

#ifdef MODPLUG_TRACKER
	if(m_SndFile.GetpModDoc())
		m_SndFile.GetpModDoc()->UpdateAllViews(nullptr, PluginHint(m_nSlot).Info(), nullptr);
#endif // MODPLUG_TRACKER
}


// Get list of plugins to which output is sent. A nullptr indicates master output.
size_t IMixPlugin::GetOutputPlugList(std::vector<IMixPlugin *> &list)
//-------------------------------------------------------------------
{
	// At the moment we know there will only be 1 output.
	// Returning nullptr means plugin outputs directly to master.
	list.clear();

	IMixPlugin *outputPlug = nullptr;
	if(!m_pMixStruct->IsOutputToMaster())
	{
		PLUGINDEX nOutput = m_pMixStruct->GetOutputPlugin();
		if(nOutput > m_nSlot && nOutput != PLUGINDEX_INVALID)
		{
			outputPlug = m_SndFile.m_MixPlugins[nOutput].pMixPlugin;
		}
	}
	list.push_back(outputPlug);

	return 1;
}


// Get a list of plugins that send data to this plugin.
size_t IMixPlugin::GetInputPlugList(std::vector<IMixPlugin *> &list)
//------------------------------------------------------------------
{
	std::vector<IMixPlugin *> candidatePlugOutputs;
	list.clear();

	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		IMixPlugin *candidatePlug = m_SndFile.m_MixPlugins[plug].pMixPlugin;
		if(candidatePlug)
		{
			candidatePlug->GetOutputPlugList(candidatePlugOutputs);

			for(std::vector<IMixPlugin *>::iterator iter = candidatePlugOutputs.begin(); iter != candidatePlugOutputs.end(); iter++)
			{
				if(*iter == this)
				{
					list.push_back(candidatePlug);
					break;
				}
			}
		}
	}

	return list.size();
}


// Get a list of instruments that send data to this plugin.
size_t IMixPlugin::GetInputInstrumentList(std::vector<INSTRUMENTINDEX> &list)
//---------------------------------------------------------------------------
{
	list.clear();
	const PLUGINDEX nThisMixPlug = m_nSlot + 1;		//m_nSlot is position in mixplug array.

	for(INSTRUMENTINDEX ins = 0; ins <= m_SndFile.GetNumInstruments(); ins++)
	{
		if(m_SndFile.Instruments[ins] != nullptr && m_SndFile.Instruments[ins]->nMixPlug == nThisMixPlug)
		{
			list.push_back(ins);
		}
	}

	return list.size();
}


size_t IMixPlugin::GetInputChannelList(std::vector<CHANNELINDEX> &list)
//---------------------------------------------------------------------
{
	list.clear();

	PLUGINDEX nThisMixPlug = m_nSlot + 1;		//m_nSlot is position in mixplug array.
	const CHANNELINDEX chnCount = m_SndFile.GetNumChannels();
	for(CHANNELINDEX nChn=0; nChn<chnCount; nChn++)
	{
		if(m_SndFile.ChnSettings[nChn].nMixPlugin == nThisMixPlug)
		{
			list.push_back(nChn);
		}
	}

	return list.size();

}


void IMixPlugin::SaveAllParameters()
//----------------------------------
{
	if (m_pMixStruct == nullptr)
	{
		return;
	}
	m_pMixStruct->defaultProgram = -1;
	
	// Default implementation: Save all parameter values
	PlugParamIndex numParams = std::min<uint32>(GetNumParameters(), std::numeric_limits<uint32>::max() / sizeof(IEEE754binary32LE));
	uint32 nLen = numParams * sizeof(IEEE754binary32LE);
	if (!nLen) return;
	nLen += 4;
	if ((m_pMixStruct->pPluginData) && (m_pMixStruct->nPluginDataSize >= nLen))
	{
		m_pMixStruct->nPluginDataSize = nLen;
	} else
	{
		if (m_pMixStruct->pPluginData) delete[] m_pMixStruct->pPluginData;
		m_pMixStruct->nPluginDataSize = 0;
		m_pMixStruct->pPluginData = new (std::nothrow) char[nLen];
		if (m_pMixStruct->pPluginData)
		{
			m_pMixStruct->nPluginDataSize = nLen;
		}
	}
	if (m_pMixStruct->pPluginData != nullptr)
	{
		IEEE754binary32LE *p = reinterpret_cast<IEEE754binary32LE *>(m_pMixStruct->pPluginData);
		memset(p, 0, sizeof(uint32));	// Plugin data type
		p++;
		for(PlugParamIndex i = 0; i < numParams; i++)
		{
			p[i] = IEEE754binary32LE(GetParameter(i));
		}
	}
}


void IMixPlugin::RestoreAllParameters(int32 /*program*/)
//------------------------------------------------------
{
	if(m_pMixStruct != nullptr && m_pMixStruct->pPluginData != nullptr && m_pMixStruct->nPluginDataSize >= 4)
	{
		uint32 type = *reinterpret_cast<const uint32 *>(m_pMixStruct->pPluginData);
		if (type == 0)
		{
			const uint32 numParams = std::min<uint32>(GetNumParameters(), (m_pMixStruct->nPluginDataSize - 4u) / sizeof(IEEE754binary32LE));
			BeginSetProgram(-1);
			const IEEE754binary32LE *p = reinterpret_cast<const IEEE754binary32LE *>(m_pMixStruct->pPluginData) + 1;
			for (uint32 i = 0; i < numParams; i++)
			{
				SetParameter(i, p[i]);
			}
			EndSetProgram();
		}
	}
}


#ifdef MODPLUG_TRACKER
void IMixPlugin::ToggleEditor()
//-----------------------------
{
	if (m_pEditor)
	{
		if (m_pEditor->m_hWnd) m_pEditor->DoClose();
		delete m_pEditor;
		m_pEditor = nullptr;
	} else
	{
		m_pEditor = OpenEditor();

		if (m_pEditor)
			m_pEditor->OpenEditor(CMainFrame::GetMainFrame());
	}
}


// Provide default plugin editor
CAbstractVstEditor *IMixPlugin::OpenEditor()
//------------------------------------------
{
	try
	{
		return new CDefaultVstEditor(*this);
	} catch(MPTMemoryException)
	{
		return nullptr;
	}
}


// Automate a parameter from the plugin GUI (both custom and default plugin GUI)
void IMixPlugin::AutomateParameter(PlugParamIndex param)
//------------------------------------------------------
{
	CModDoc *modDoc = GetModDoc();
	if(modDoc == nullptr)
	{
		return;
	}

	// TODO: Check if any params are actually automatable, and if there are but this one isn't, chicken out

	if(m_bRecordAutomation)
	{
		// Record parameter change
		modDoc->RecordParamChange(GetSlot(), param);
	}

	modDoc->PostMessageToAllViews(WM_MOD_PLUGPARAMAUTOMATE, m_nSlot, param);
	// TODO: This should rather be posted to the GUI thread!
	CAbstractVstEditor *pVstEditor = GetEditor();

	if(pVstEditor && pVstEditor->m_hWnd)
	{
		// Mark track modified if GUI is open and format supports plugins
		if(m_SndFile.GetModSpecifications().supportsPlugins)
		{
			CMainFrame::GetMainFrame()->ThreadSafeSetModified(modDoc);
		}


		if (CMainFrame::GetInputHandler()->ShiftPressed())
		{
			// Shift pressed -> Open MIDI mapping dialog
			CMainFrame::GetInputHandler()->SetModifierMask(0); // Make sure that the dialog will open only once.
			CMainFrame::GetMainFrame()->PostMessage(WM_MOD_MIDIMAPPING, m_nSlot, param);
		}

		// Learn macro
		int macroToLearn = pVstEditor->GetLearnMacro();
		if (macroToLearn > -1)
		{
			modDoc->LearnMacro(macroToLearn, param);
			pVstEditor->SetLearnMacro(-1);
		}
	}
}


bool IMixPlugin::SaveProgram()
//----------------------------
{
	mpt::PathString defaultDir = TrackerSettings::Instance().PathPluginPresets.GetWorkingDir();
	bool useDefaultDir = !defaultDir.empty();
	if(!useDefaultDir)
	{
		defaultDir = m_Factory.dllPath.GetPath();
	}

	CString progName = GetCurrentProgramName();
	SanitizeFilename(progName);

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension("fxb")
		.DefaultFilename(progName)
		.ExtensionFilter("VST Plugin Programs (*.fxp)|*.fxp|"
			"VST Plugin Banks (*.fxb)|*.fxb||")
		.WorkingDirectory(defaultDir);
	if(!dlg.Show(m_pEditor)) return false;

	if(useDefaultDir)
	{
		TrackerSettings::Instance().PathPluginPresets.SetWorkingDir(dlg.GetWorkingDirectory());
	}

	bool bank = (dlg.GetExtension() == MPT_PATHSTRING("fxb"));

	mpt::fstream f(dlg.GetFirstFile(), std::ios::out | std::ios::trunc | std::ios::binary);
	if(f.good() && VSTPresets::SaveFile(f, *this, bank))
	{
		return true;
	} else
	{
		Reporting::Error("Error saving preset.", m_pEditor);
		return false;
	}

}


bool IMixPlugin::LoadProgram(mpt::PathString fileName)
//----------------------------------------------------
{
	mpt::PathString defaultDir = TrackerSettings::Instance().PathPluginPresets.GetWorkingDir();
	bool useDefaultDir = !defaultDir.empty();
	if(!useDefaultDir)
	{
		defaultDir = m_Factory.dllPath.GetPath();
	}

	if(fileName.empty())
	{
		FileDialog dlg = OpenFileDialog()
			.DefaultExtension("fxp")
			.ExtensionFilter("VST Plugin Programs and Banks (*.fxp,*.fxb)|*.fxp;*.fxb|"
			"VST Plugin Programs (*.fxp)|*.fxp|"
			"VST Plugin Banks (*.fxb)|*.fxb|"
			"All Files|*.*||")
			.WorkingDirectory(defaultDir);
		if(!dlg.Show(m_pEditor)) return false;

		if(useDefaultDir)
		{
			TrackerSettings::Instance().PathPluginPresets.SetWorkingDir(dlg.GetWorkingDirectory());
		}
		fileName = dlg.GetFirstFile();
	}

	const char *errorStr = nullptr;
	InputFile f(fileName);
	if(f.IsValid())
	{
		FileReader file = GetFileReader(f);
		errorStr = VSTPresets::GetErrorMessage(VSTPresets::LoadFile(file, *this));
	} else
	{
		errorStr = "Can't open file.";
	}

	if(errorStr == nullptr)
	{
		if(GetModDoc() != nullptr && GetSoundFile().GetModSpecifications().supportsPlugins)
		{
			GetModDoc()->SetModified();
		}
		return true;
	} else
	{
		Reporting::Error(errorStr, m_pEditor);
		return false;
	}
}


#endif // MODPLUG_TRACKER


////////////////////////////////////////////////////////////////////
// IMidiPlugin: Default implementation of plugins with MIDI input //
////////////////////////////////////////////////////////////////////

IMidiPlugin::IMidiPlugin(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMixPlugin(factory, sndFile, mixStruct)
//-------------------------------------------------------------------------------------------
{
	MemsetZero(m_MidiCh);
	for(int ch = 0; ch < 16; ch++)
	{
		m_MidiCh[ch].midiPitchBendPos = EncodePitchBendParam(MIDIEvents::pitchBendCentre); // centre pitch bend on all channels
		m_MidiCh[ch].ResetProgram();
	}
}


void IMidiPlugin::ApplyPitchWheelDepth(int32 &value, int8 pwd)
//------------------------------------------------------------
{
	if(pwd != 0)
	{
		value = (value * ((MIDIEvents::pitchBendMax - MIDIEvents::pitchBendCentre + 1) / 64)) / pwd;
	} else
	{
		value = 0;
	}
}


void IMidiPlugin::MidiCC(uint8 nMidiCh, MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX /*trackChannel*/)
//------------------------------------------------------------------------------------------------------------------
{
	//Error checking
	LimitMax(nController, MIDIEvents::MIDICC_end);
	LimitMax(nParam, uint8(127));

	if(m_SndFile.m_playBehaviour[kMIDICCBugEmulation])
		MidiSend(MIDIEvents::Event(MIDIEvents::evControllerChange, nMidiCh, nParam, static_cast<uint8>(nController)));	// param and controller are swapped (old broken implementation)
	else
		MidiSend(MIDIEvents::CC(nController, nMidiCh, nParam));
}


// Bend MIDI pitch for given MIDI channel using fine tracker param (one unit = 1/64th of a note step)
void IMidiPlugin::MidiPitchBend(uint8 nMidiCh, int32 increment, int8 pwd)
//-----------------------------------------------------------------------
{
	if(m_SndFile.m_playBehaviour[kOldMIDIPitchBends])
	{
		// OpenMPT Legacy: Old pitch slides never were really accurate, but setting the PWD to 13 in plugins would give the closest results.
		increment = (increment * 0x800 * 13) / (0xFF * pwd);
		increment = EncodePitchBendParam(increment);
	} else
	{
		increment = EncodePitchBendParam(increment);
		ApplyPitchWheelDepth(increment, pwd);
	}

	int32 newPitchBendPos = (increment + m_MidiCh[nMidiCh].midiPitchBendPos) & vstPitchBendMask;
	Limit(newPitchBendPos, EncodePitchBendParam(MIDIEvents::pitchBendMin), EncodePitchBendParam(MIDIEvents::pitchBendMax));

	MidiPitchBend(nMidiCh, newPitchBendPos);
}


// Set MIDI pitch for given MIDI channel using fixed point pitch bend value (converted back to 0-16383 MIDI range)
void IMidiPlugin::MidiPitchBend(uint8 nMidiCh, int32 newPitchBendPos)
//-------------------------------------------------------------------
{
	ASSERT(EncodePitchBendParam(MIDIEvents::pitchBendMin) <= newPitchBendPos && newPitchBendPos <= EncodePitchBendParam(MIDIEvents::pitchBendMax));
	m_MidiCh[nMidiCh].midiPitchBendPos = newPitchBendPos;
	MidiSend(MIDIEvents::PitchBend(nMidiCh, DecodePitchBendParam(newPitchBendPos)));
}


// Apply vibrato effect through pitch wheel commands on a given MIDI channel.
void IMidiPlugin::MidiVibrato(uint8 nMidiCh, int32 depth, int8 pwd)
//-----------------------------------------------------------------
{
	depth = EncodePitchBendParam(depth);
	if(depth != 0 || (m_MidiCh[nMidiCh].midiPitchBendPos & vstVibratoFlag))
	{
		ApplyPitchWheelDepth(depth, pwd);

		// Temporarily add vibrato offset to current pitch
		int32 newPitchBendPos = (depth + m_MidiCh[nMidiCh].midiPitchBendPos) & vstPitchBendMask;
		Limit(newPitchBendPos, EncodePitchBendParam(MIDIEvents::pitchBendMin), EncodePitchBendParam(MIDIEvents::pitchBendMax));

		MidiSend(MIDIEvents::PitchBend(nMidiCh, DecodePitchBendParam(newPitchBendPos)));
	}

	// Update vibrato status
	if(depth != 0)
	{
		m_MidiCh[nMidiCh].midiPitchBendPos |= vstVibratoFlag;
	} else
	{
		m_MidiCh[nMidiCh].midiPitchBendPos &= ~vstVibratoFlag;
	}
}


void IMidiPlugin::MidiCommand(uint8 nMidiCh, uint8 nMidiProg, uint16 wMidiBank, uint16 note, uint16 vol, CHANNELINDEX trackChannel)
//---------------------------------------------------------------------------------------------------------------------------------
{
	PlugInstrChannel &channel = m_MidiCh[nMidiCh];

	bool bankChanged = (channel.currentBank != --wMidiBank) && (wMidiBank < 0x4000);
	bool progChanged = (channel.currentProgram != --nMidiProg) && (nMidiProg < 0x80);
	//get vol in [0,128[
	uint8 volume = static_cast<uint8>(std::min(vol / 2, 127));

	// Bank change
	if(wMidiBank < 0x4000 && bankChanged)
	{
		uint8 high = static_cast<uint8>(wMidiBank >> 7);
		uint8 low = static_cast<uint8>(wMidiBank & 0x7F);

		if((channel.currentBank >> 7) != high)
		{
			// High byte changed
			MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_BankSelect_Coarse, nMidiCh, high));
		}
		// Low byte
		//GetSoundFile()->ProcessMIDIMacro(trackChannel, false, GetSoundFile()->m_MidiCfg.szMidiGlb[MIDIOUT_BANKSEL], 0);
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_BankSelect_Fine, nMidiCh, low));

		channel.currentBank = wMidiBank;
	}

	// Program change
	// According to the MIDI specs, a bank change alone doesn't have to change the active program - it will only change the bank of subsequent program changes.
	// Thus we send program changes also if only the bank has changed.
	if(nMidiProg < 0x80 && (progChanged || bankChanged))
	{
		channel.currentProgram = nMidiProg;
		//GetSoundFile()->ProcessMIDIMacro(trackChannel, false, GetSoundFile()->m_MidiCfg.szMidiGlb[MIDIOUT_PROGRAM], 0);
		MidiSend(MIDIEvents::ProgramChange(nMidiCh, nMidiProg));
	}


	// Specific Note Off
	if(note > NOTE_MAX_SPECIAL)
	{
		uint8 i = static_cast<uint8>(note - NOTE_MAX_SPECIAL - NOTE_MIN);
		if(channel.noteOnMap[i][trackChannel])
		{
			channel.noteOnMap[i][trackChannel]--;
			MidiSend(MIDIEvents::NoteOff(nMidiCh, i, 0));
		}
	}

	// "Hard core" All Sounds Off on this midi and tracker channel
	// This one doesn't check the note mask - just one note off per note.
	// Also less likely to cause a VST event buffer overflow.
	else if(note == NOTE_NOTECUT)	// ^^
	{
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllNotesOff, nMidiCh, 0));
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllSoundOff, nMidiCh, 0));

		// Turn off all notes
		for(uint8 i = 0; i < CountOf(channel.noteOnMap); i++)
		{
			channel.noteOnMap[i][trackChannel] = 0;
			MidiSend(MIDIEvents::NoteOff(nMidiCh, i, volume));
		}

	}

	// All "active" notes off on this midi and tracker channel
	// using note mask.
	else if(note == NOTE_KEYOFF || note == NOTE_FADE) // ==, ~~
	{
		for(uint8 i = 0; i < CountOf(channel.noteOnMap); i++)
		{
			// Some VSTis need a note off for each instance of a note on, e.g. fabfilter.
			while(channel.noteOnMap[i][trackChannel])
			{
				MidiSend(MIDIEvents::NoteOff(nMidiCh, i, volume));
				channel.noteOnMap[i][trackChannel]--;
			}
		}
	}

	// Note On
	else if(ModCommand::IsNote(static_cast<ModCommand::NOTE>(note)))
	{
		note -= NOTE_MIN;

		// Reset pitch bend on each new note, tracker style.
		// This is done if the pitch wheel has been moved or there was a vibrato on the previous row (in which case the "vstVibratoFlag" bit of the pitch bend memory is set)
		if(m_MidiCh[nMidiCh].midiPitchBendPos != EncodePitchBendParam(MIDIEvents::pitchBendCentre))
		{
			MidiPitchBend(nMidiCh, EncodePitchBendParam(MIDIEvents::pitchBendCentre));
		}

		// count instances of active notes.
		// This is to send a note off for each instance of a note, for plugs like Fabfilter.
		// Problem: if a note dies out naturally and we never send a note off, this counter
		// will block at max until note off. Is this a problem?
		// Safe to assume we won't need more than 16 note offs max on a given note?
		if(channel.noteOnMap[note][trackChannel] < 17)
			channel.noteOnMap[note][trackChannel]++;

		MidiSend(MIDIEvents::NoteOn(nMidiCh, static_cast<uint8>(note), volume));
	}
}


bool IMidiPlugin::IsNotePlaying(uint32 note, uint32 midiChn, uint32 trackerChn)
//-------------------------------------------------------------------------
{
	note -= NOTE_MIN;
	return (m_MidiCh[midiChn].noteOnMap[note][trackerChn] != 0);
}


std::string SNDMIXPLUGIN::GetParamName(PlugParamIndex index) const
//----------------------------------------------------------------
{
	if(pMixPlugin != nullptr)
	{
		return pMixPlugin->GetFormattedParamName(index).GetString();
	} else
	{
		return std::string();
	}
}


OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS