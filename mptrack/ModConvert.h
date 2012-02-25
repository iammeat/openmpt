/*
 * ModConvert.h
 * ------------
 * Purpose: Converting between various module formats.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

// Warning types
enum enmWarnings
{
	wInstrumentsToSamples = 0,
	wResizedPatterns,
	wSampleBidiLoops,
	wSampleSustainLoops,
	wSampleAutoVibrato,
	wMODSampleFrequency,
	wBrokenNoteMap,
	wInstrumentSustainLoops,
	wInstrumentTuning,
	wMODGlobalVars,
	wMOD31Samples,
	wRestartPos,
	wChannelVolSurround,
	wChannelPanning,
	wPatternSignatures,
	wLinearSlides,
	wTrimmedEnvelopes,
	wReleaseNode,
	wEditHistory,
	wMixmode,
	wCompatibilityMode,
	wPitchToTempoLock,
	wNumWarnings
};
