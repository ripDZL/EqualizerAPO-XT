/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "VSTPreviewEndpoint.h"

#include "AbstractAPOInfo.h"

namespace
{
const wchar_t* renderEndpointPrefix = L"{0.0.0.00000000}.";
const wchar_t* captureEndpointPrefix = L"{0.0.1.00000000}.";

bool startsWith(const std::wstring& value, const wchar_t* prefix)
{
	return value.rfind(prefix, 0) == 0;
}
}

VSTPreviewEndpoint vstPreviewEndpointFromDeviceGuid(bool input, const std::wstring& deviceGuid)
{
	if (deviceGuid.empty())
		return {};

	if (startsWith(deviceGuid, captureEndpointPrefix))
		return { VSTPreviewEndpointFlow::Capture, deviceGuid };
	if (startsWith(deviceGuid, renderEndpointPrefix))
		return { VSTPreviewEndpointFlow::Render, deviceGuid };

	return {
		input ? VSTPreviewEndpointFlow::Capture : VSTPreviewEndpointFlow::Render,
		std::wstring(input ? captureEndpointPrefix : renderEndpointPrefix) + deviceGuid
	};
}

VSTPreviewEndpoint vstPreviewEndpointForSelectedDevice(const std::shared_ptr<AbstractAPOInfo>& selectedDevice)
{
	if (selectedDevice == nullptr)
		return {};
	return vstPreviewEndpointFromDeviceGuid(selectedDevice->isInput(), selectedDevice->getDeviceGuid());
}
