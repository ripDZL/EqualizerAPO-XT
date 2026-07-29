/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.

	Selected-device resolution for the Editor-side VST live analyzer feed.
*/

#pragma once

#include <memory>
#include <string>

class AbstractAPOInfo;

enum class VSTPreviewEndpointFlow
{
	None,
	Render,
	Capture
};

struct VSTPreviewEndpoint
{
	VSTPreviewEndpointFlow flow = VSTPreviewEndpointFlow::None;
	std::wstring deviceId;

	bool isValid() const
	{
		return flow != VSTPreviewEndpointFlow::None && !deviceId.empty();
	}

	bool operator==(const VSTPreviewEndpoint& other) const
	{
		return flow == other.flow && deviceId == other.deviceId;
	}

	bool operator!=(const VSTPreviewEndpoint& other) const
	{
		return !(*this == other);
	}
};

VSTPreviewEndpoint vstPreviewEndpointFromDeviceGuid(bool input, const std::wstring& deviceGuid);
VSTPreviewEndpoint vstPreviewEndpointForSelectedDevice(const std::shared_ptr<AbstractAPOInfo>& selectedDevice);
