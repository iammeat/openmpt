/*
 * XMTools.cpp
 * -----------
 * Purpose: Definition of XM file structures and helper functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "XMTools.h"
#include "Sndfile.h"
#include <algorithm>


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void XMFileHeader::ConvertEndianness()
//------------------------------------
{
	version = LittleEndianW(version);
	size = LittleEndian(size);
	orders = LittleEndianW(orders);
	restartpos = LittleEndianW(restartpos);
	channels = LittleEndianW(channels);
	patterns = LittleEndianW(patterns);
	instruments = LittleEndianW(instruments);
	flags = LittleEndianW(flags);
	speed = LittleEndianW(speed);
	tempo = LittleEndianW(tempo);
}


// Convert OpenMPT's internal envelope representation to XM envelope data.
void XMInstrument::ConvertEnvelopeToXM(const InstrumentEnvelope &mptEnv, uint8 &numPoints, uint8 &flags, uint8 &sustain, uint8 &loopStart, uint8 &loopEnd, uint16 (&envData)[24])
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	numPoints = static_cast<uint8>(min(12, mptEnv.nNodes));

	// Envelope Data
	for(size_t i = 0; i < numPoints; i++)
	{
		envData[i * 2] = LittleEndianW(min(mptEnv.Ticks[i], uint16_max));
		envData[i * 2 + 1] = LittleEndianW(mptEnv.Values[i]);
	}

	// Envelope Flags
	if(mptEnv.dwFlags & ENV_ENABLED) flags |= XMInstrument::envEnabled;
	if(mptEnv.dwFlags & ENV_SUSTAIN) flags |= XMInstrument::envSustain;
	if(mptEnv.dwFlags & ENV_LOOP) flags |= XMInstrument::envLoop;

	// Envelope Loops
	sustain = static_cast<uint8>(min(12, mptEnv.nSustainStart));
	loopStart = static_cast<uint8>(min(12, mptEnv.nLoopStart));
	loopEnd = static_cast<uint8>(min(12, mptEnv.nLoopEnd));

}


// Convert OpenMPT's internal sample representation to an XMInstrument.
void XMInstrument::ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport)
//-----------------------------------------------------------------------------------
{
	MemsetZero(*this);

	// FFF is maximum in the FT2 GUI, but it can also accept other values. MilkyTracker just allows 0...4095 and 32767 ("cut")
	volFade = static_cast<uint16>(LittleEndianW(min(mptIns.nFadeOut, 32767)));

	// Convert envelopes
	ConvertEnvelopeToXM(mptIns.VolEnv, volPoints, volFlags, volSustain, volLoopStart, volLoopEnd, volEnv);
	ConvertEnvelopeToXM(mptIns.PanEnv, panPoints, panFlags, panSustain, panLoopStart, panLoopEnd, panEnv);

	// Create sample assignment table
	vector<SAMPLEINDEX> sampleList = GetSampleList(mptIns, compatibilityExport);
	for(size_t i = 0; i < CountOf(sampleMap); i++)
	{
		if(mptIns.Keyboard[i + 12] > 0)
		{
			vector<SAMPLEINDEX>::iterator sample = std::find(sampleList.begin(), sampleList.end(), mptIns.Keyboard[i + 12]);
			if(sample != sampleList.end())
			{
				// Yep, we want to export this sample.
				sampleMap[i] = static_cast<uint8>(sample - sampleList.begin());
			}
		}
	}
}


// Get a list of samples that should be written to the file.
vector<SAMPLEINDEX> XMInstrument::GetSampleList(const ModInstrument &mptIns, bool compatibilityExport) const
//----------------------------------------------------------------------------------------------------------
{
	vector<SAMPLEINDEX> sampleList;		// List of samples associated with this instrument
	vector<bool> addedToList;			// Which samples did we already add to the sample list?

	uint8 numSamples = 0;
	for(size_t i = 0; i < CountOf(sampleMap); i++)
	{
		const SAMPLEINDEX smp = mptIns.Keyboard[i + 12];
		if(smp > 0)
		{
			if(smp > addedToList.size())
			{
				addedToList.resize(smp, false);
			}

			if(!addedToList[smp - 1] && numSamples < (compatibilityExport ? 16 : 32))
			{
				// We haven't considered this sample yet.
				addedToList[smp - 1] = true;
				numSamples++;
				sampleList.push_back(smp);
			}
		}
	}
	return sampleList;
}


// Convert XM envelope data to an OpenMPT's internal envelope representation.
void XMInstrument::ConvertEnvelopeToMPT(InstrumentEnvelope &mptEnv, uint8 numPoints, uint8 flags, uint8 sustain, uint8 loopStart, uint8 loopEnd, const uint16 (&envData)[24]) const
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	mptEnv.nNodes = min(numPoints, 12);

	// Envelope Data
	for(size_t i = 0; i < 12; i++)
	{
		mptEnv.Ticks[i] = static_cast<WORD>(LittleEndianW(envData[i * 2]));
		mptEnv.Values[i] = static_cast<BYTE>(LittleEndianW(envData[i * 2 + 1]));

		if(i > 0 && mptEnv.Ticks[i] < mptEnv.Ticks[i - 1])
		{
			// libmikmod code says: "Some broken XM editing program will only save the low byte of the position
			// value. Try to compensate by adding the missing high byte" - I guess that's what this code is for.
			// Note: It appears that MPT 1.07's XI instrument saver omitted the high byte of envelope nodes.
			// This might be the source for some broken envelopes in IT and XM files.

			mptEnv.Ticks[i] &= 0xFF;
			mptEnv.Ticks[i] += mptEnv.Ticks[i - 1] & 0xFF00;
			if(mptEnv.Ticks[i] < mptEnv.Ticks[i - 1]) mptEnv.Ticks[i] += 0x100;
		}
	}
	// Sanitize envelope
	mptEnv.Ticks[0] = 0;

	// Envelope Flags
	mptEnv.dwFlags = 0;
	if(flags & XMInstrument::envEnabled && mptEnv.nNodes) mptEnv.dwFlags |= ENV_ENABLED;
	if(flags & XMInstrument::envSustain) mptEnv.dwFlags |= ENV_SUSTAIN;
	if(flags & XMInstrument::envLoop) mptEnv.dwFlags |= ENV_LOOP;

	// Envelope Loops
	if(sustain < 12)
	{
		mptEnv.nSustainStart = mptEnv.nSustainEnd = sustain;
	} else
	{
		mptEnv.dwFlags &= ~ENV_SUSTAIN;
	}

	if(loopEnd < 12 && loopEnd >= loopStart)
	{
		mptEnv.nLoopStart = loopStart;
		mptEnv.nLoopEnd = loopEnd;
	} else
	{
		mptEnv.dwFlags &= ~ENV_LOOP;
	}
}


// Convert an XMInstrument to OpenMPT's internal instrument representation.
void XMInstrument::ConvertToMPT(ModInstrument &mptIns) const
//----------------------------------------------------------
{
	mptIns.nFadeOut = volFade;

	// Convert envelopes
	ConvertEnvelopeToMPT(mptIns.VolEnv, volPoints, volFlags, volSustain, volLoopStart, volLoopEnd, volEnv);
	ConvertEnvelopeToMPT(mptIns.PanEnv, panPoints, panFlags, panSustain, panLoopStart, panLoopEnd, panEnv);

	// Create sample assignment table
	for(size_t i = 0; i < CountOf(sampleMap); i++)
	{
		mptIns.Keyboard[i + 12] = sampleMap[i];
	}
}


// Apply auto-vibrato settings from sample to file.
void XMInstrument::ApplyAutoVibratoToXM(const ModSample &mptSmp, MODTYPE fromType)
//--------------------------------------------------------------------------------
{
	vibType = mptSmp.nVibType;
	vibSweep = mptSmp.nVibSweep;
	vibDepth = mptSmp.nVibDepth;
	vibRate = mptSmp.nVibRate;

	if((vibDepth | vibRate) != 0 && !(fromType & MOD_TYPE_XM))
	{
		// Sweep is upside down in XM
		vibSweep = 255 - vibSweep;
	}
}


// Apply auto-vibrato settings from file to a sample.
void XMInstrument::ApplyAutoVibratoToMPT(ModSample &mptSmp) const
//---------------------------------------------------------------
{
	mptSmp.nVibType = vibType;
	mptSmp.nVibSweep = vibSweep;
	mptSmp.nVibDepth = vibDepth;
	mptSmp.nVibRate = vibRate;
}


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void XMInstrumentHeader::ConvertEndianness()
//------------------------------------------
{
	size = LittleEndian(size);
	sampleHeaderSize = LittleEndian(sampleHeaderSize);
	numSamples = LittleEndianW(numSamples);
}


// Write stuff to the header that's always necessary (also for empty instruments)
void XMInstrumentHeader::Finalise()
//---------------------------------
{
	size = sizeof(XMInstrumentHeader);
	if(numSamples > 0)
	{
		sampleHeaderSize = sizeof(XMSample);
	} else
	{
		size -= sizeof(XMInstrument);
		sampleHeaderSize = 0;
	}
}


// Convert OpenMPT's internal sample representation to an XMInstrumentHeader.
void XMInstrumentHeader::ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport)
//-----------------------------------------------------------------------------------------
{
	instrument.ConvertToXM(mptIns, compatibilityExport);

	StringFixer::WriteString<StringFixer::spacePadded>(name, mptIns.name);

	type = mptIns.nMidiProgram;	// If FT2 writes crap here, we can do so, too!

	numSamples = static_cast<uint16>(min(mptIns.GetSamples().size(), size_t(compatibilityExport ? 16 : 32)));
}


// Convert an XMInstrumentHeader to OpenMPT's internal instrument representation.
void XMInstrumentHeader::ConvertToMPT(ModInstrument &mptIns) const
//----------------------------------------------------------------
{
	instrument.ConvertToMPT(mptIns);

	// Create sample assignment table
	for(size_t i = 0; i < CountOf(instrument.sampleMap); i++)
	{
		if(instrument.sampleMap[i] < numSamples)
		{
			mptIns.Keyboard[i + 12] = instrument.sampleMap[i];
		} else
		{
			mptIns.Keyboard[i + 12] = 0;
		}
	}

	StringFixer::ReadString<StringFixer::spacePadded>(mptIns.name, name);

	mptIns.nMidiProgram = type;
}


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void XIInstrumentHeader::ConvertEndianness()
//------------------------------------------
{
	version = LittleEndianW(version);
	numSamples = LittleEndianW(numSamples);
}


// Convert OpenMPT's internal sample representation to an XIInstrumentHeader.
void XIInstrumentHeader::ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport)
//-----------------------------------------------------------------------------------------
{
	instrument.ConvertToXM(mptIns, compatibilityExport);

	memcpy(signature, "Extended Instrument: ", 21);
	StringFixer::WriteString<StringFixer::spacePadded>(name, mptIns.name);
	eof = 0x1A;

	memcpy(trackerName, "Created by OpenMPT  ", 20);

	version = LittleEndianW(0x102);

	numSamples = static_cast<uint16>(LittleEndianW(min(mptIns.GetSamples().size(), size_t(compatibilityExport ? 16 : 32))));
}


// Convert an XIInstrumentHeader to OpenMPT's internal instrument representation.
void XIInstrumentHeader::ConvertToMPT(ModInstrument &mptIns) const
//----------------------------------------------------------------
{
	instrument.ConvertToMPT(mptIns);

	// Fix sample assignment table
	for(size_t i = 12; i < CountOf(instrument.sampleMap) + 12; i++)
	{
		if(mptIns.Keyboard[i] >= numSamples)
		{
			mptIns.Keyboard[i] = 0;
		}
	}

	StringFixer::ReadString<StringFixer::spacePadded>(mptIns.name, name);
}


// Convert OpenMPT's internal sample representation to an XMSample.
void XMSample::ConvertToXM(const ModSample &mptSmp, MODTYPE fromType, bool compatibilityExport)
//---------------------------------------------------------------------------------------------
{
	MemsetZero(*this);

	// Volume / Panning
	vol = static_cast<uint8>(min(mptSmp.nVolume / 4, 64));
	pan = static_cast<uint8>(min(mptSmp.nPan, 255));

	// Sample Frequency
	if((fromType & (MOD_TYPE_MOD | MOD_TYPE_XM)))
	{
		finetune = mptSmp.nFineTune;
		relnote = mptSmp.RelativeTone;
	} else
	{
		int f2t = CSoundFile::FrequencyToTranspose(mptSmp.nC5Speed);
		relnote = (int8)(f2t >> 7);
		finetune = (int8)(f2t & 0x7F);
	}

	flags = 0;
	if((mptSmp.uFlags & CHN_LOOP))
	{
		flags |= (mptSmp.uFlags & CHN_PINGPONGLOOP) ? XMSample::sampleBidiLoop : XMSample::sampleLoop;
	}

	// Sample Length and Loops
	length = mptSmp.nLength;
	loopStart = mptSmp.nLoopStart;
	loopLength = mptSmp.nLoopEnd - mptSmp.nLoopStart;

	if((mptSmp.uFlags & CHN_16BIT))
	{
		flags |= XMSample::sample16Bit;
		length *= 2;
		loopStart *= 2;
		loopLength *= 2;
	}

	if((mptSmp.uFlags & CHN_STEREO) && !compatibilityExport)
	{
		flags |= XMSample::sampleStereo;
		length *= 2;
		loopStart *= 2;
		loopLength *= 2;
	}

	length = LittleEndian(length);
	loopStart = LittleEndian(loopStart);
	loopLength = LittleEndian(loopLength);
}


// Convert an XMSample to OpenMPT's internal sample representation.
void XMSample::ConvertToMPT(ModSample &mptSmp) const
//--------------------------------------------------
{
	// Volume
	mptSmp.nVolume = vol * 4;
	LimitMax(mptSmp.nVolume, WORD(256));
	mptSmp.nGlobalVol = 64;

	// Panning
	mptSmp.nPan = pan;
	mptSmp.uFlags = CHN_PANNING;

	// Sample Frequency
	mptSmp.nC5Speed = 0;
	mptSmp.nFineTune = finetune;
	mptSmp.RelativeTone = relnote;

	// Sample Length and Loops
	mptSmp.nLength = LittleEndian(length);
	mptSmp.nLoopStart = LittleEndian(loopStart);
	mptSmp.nLoopEnd = mptSmp.nLoopStart + LittleEndian(loopLength);
	mptSmp.nSustainStart = mptSmp.nSustainEnd = 0;

	if((flags & XMSample::sample16Bit))
	{
		mptSmp.nLength /= 2;
		mptSmp.nLoopStart /= 2;
		mptSmp.nLoopEnd /= 2;
	}

	if((flags & XMSample::sampleStereo))
	{
		mptSmp.nLength /= 2;
		mptSmp.nLoopStart /= 2;
		mptSmp.nLoopEnd /= 2;
	}

	if((flags & (XMSample::sampleLoop | XMSample::sampleBidiLoop)) && mptSmp.nLoopStart < mptSmp.nLength && mptSmp.nLoopEnd > mptSmp.nLoopStart)
	{
		mptSmp.uFlags |= CHN_LOOP;
		if((flags & XMSample::sampleBidiLoop))
		{
			mptSmp.uFlags |= CHN_PINGPONGLOOP;
		}
	}

	LimitMax(mptSmp.nLength, UINT(MAX_SAMPLE_LENGTH));
	LimitMax(mptSmp.nLoopStart, mptSmp.nLength);
	Limit(mptSmp.nLoopEnd, mptSmp.nLoopStart, mptSmp.nLength);

	if(mptSmp.nLoopStart > mptSmp.nLoopEnd)
	{
		mptSmp.nLoopStart = mptSmp.nLoopEnd = 0;
	}

	strcpy(mptSmp.filename, "");
}


// Retrieve the internal sample format flags for this instrument.
UINT XMSample::GetSampleFormat() const
//------------------------------------
{
	if(reserved == sampleADPCM && !(flags & (XMSample::sample16Bit | XMSample::sampleStereo)))
	{
		return RS_ADPCM4;
	}

	if(flags & XMSample::sampleStereo)
	{
		return (flags & XMSample::sample16Bit) ? RS_STPCM16D : RS_STPCM8D;
	} else
	{
		return (flags & XMSample::sample16Bit) ? RS_PCM16D : RS_PCM8D;
	}
}
