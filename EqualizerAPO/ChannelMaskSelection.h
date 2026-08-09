/*
	This file is part of EqualizerAPO-XT.

	The LockForProcess channel-mask choice is kept separate from the COM
	adapter so capture selection remains directly testable. Initialize() fills
	EngineSetup before FilterEngine has been initialized, so this must take the
	setup's capture flag rather than query the engine's previous state.
*/

#pragma once

inline unsigned resolveApoChannelMask(bool capture,
	unsigned inputChannelCount, unsigned inputChannelMask,
	unsigned outputChannelCount, unsigned outputChannelMask)
{
	if (capture)
	{
		if (inputChannelMask == 0 && inputChannelCount == outputChannelCount)
			return outputChannelMask;
		return inputChannelMask;
	}

	if (outputChannelMask == 0 && inputChannelCount == outputChannelCount)
		return inputChannelMask;
	return outputChannelMask;
}
