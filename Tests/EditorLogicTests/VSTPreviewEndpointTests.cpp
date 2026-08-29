/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2026  EqualizerAPO-XT contributors

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "EditorLogicTestSupport.h"

#include <memory>
#include <vector>

#include "devices/AbstractAPOInfo.h"
#include "Editor/helpers/VSTPreviewEndpoint.h"

namespace
{
class PreviewEndpointTestAPOInfo : public AbstractAPOInfo
{
public:
	PreviewEndpointTestAPOInfo(bool input, const std::wstring& guid, const std::wstring& deviceString = L"")
		: input(input), guid(guid), deviceString(deviceString.empty() ? guid : deviceString)
	{
	}

	std::wstring getConnectionName() const override { return L""; }
	std::wstring getDeviceName() const override { return L""; }
	std::wstring getDeviceGuid() const override { return guid; }
	std::wstring getDeviceString() const override { return deviceString; }
	unsigned getChannelCount() const override { return 0; }
	unsigned getSampleRate() const override { return 0; }
	unsigned long getChannelMask() const override { return 0; }
	bool isInput() const override { return input; }
	bool isInstalled() const override { return true; }
	bool canBeUpgraded() const override { return false; }
	bool hasChanges() const override { return false; }
	bool isEnhancementsDisabled() const override { return false; }
	bool isDefaultDevice() const override { return false; }
	bool isDisabled() const override { return false; }
	bool isUnplugged() const override { return false; }
	void install() override {}
	void uninstall() override {}
	void reinstall() override {}

private:
	bool input;
	std::wstring guid;
	std::wstring deviceString;
};
}

void testVSTPreviewEndpointSelection()
{
	const std::wstring rawGuid = L"{dddddddd-1111-2222-3333-444444444444}";
	const VSTPreviewEndpoint inputEndpoint = vstPreviewEndpointFromDeviceGuid(true, rawGuid);
	expectTrue(inputEndpoint.flow == VSTPreviewEndpointFlow::Capture,
		QStringLiteral("raw input GUID resolves to a capture endpoint"));
	expectTrue(inputEndpoint.deviceId == L"{0.0.1.00000000}." + rawGuid,
		QStringLiteral("raw input GUID gets the capture endpoint prefix"));

	const VSTPreviewEndpoint outputEndpoint = vstPreviewEndpointFromDeviceGuid(false, rawGuid);
	expectTrue(outputEndpoint.flow == VSTPreviewEndpointFlow::Render,
		QStringLiteral("raw output GUID resolves to a render endpoint"));
	expectTrue(outputEndpoint.deviceId == L"{0.0.0.00000000}." + rawGuid,
		QStringLiteral("raw output GUID gets the render endpoint prefix"));

	const std::wstring fullCaptureId = L"{0.0.1.00000000}.{eeeeeeee-1111-2222-3333-444444444444}";
	const VSTPreviewEndpoint preserved = vstPreviewEndpointFromDeviceGuid(true, fullCaptureId);
	expectTrue(preserved.flow == VSTPreviewEndpointFlow::Capture,
		QStringLiteral("full capture endpoint id resolves as capture"));
	expectTrue(preserved.deviceId == fullCaptureId,
		QStringLiteral("full endpoint id is not prefixed again"));

	expectFalse(vstPreviewEndpointForSelectedDevice(nullptr).isValid(),
		QStringLiteral("no selected device leaves preview capture on defaults"));
	expectFalse(vstPreviewEndpointFromDeviceGuid(true, L"").isValid(),
		QStringLiteral("empty endpoint GUID leaves preview capture on defaults"));

	const std::wstring outputGuid = L"{aaaaaaaa-1111-2222-3333-444444444444}";
	const std::wstring inputGuid = L"{cf6bfa75-5dda-420a-b014-5f34758e6be6}";
	const auto selectedSpeakers = std::make_shared<PreviewEndpointTestAPOInfo>(
		false, outputGuid, L"Speakers High Definition Audio {" + outputGuid.substr(1));
	const auto behringerIn1 = std::make_shared<PreviewEndpointTestAPOInfo>(
		true, inputGuid, L"IN 1 BEHRINGER UMC 204HD 192k " + inputGuid);
	const std::vector<std::shared_ptr<AbstractAPOInfo>> outputs{ selectedSpeakers };
	const std::vector<std::shared_ptr<AbstractAPOInfo>> inputs{ behringerIn1 };

	const std::vector<std::wstring> micScopedVstLines{
		L"Preamp: -6 dB",
		L"Device: IN 1 BEHRINGER UMC 204HD 192k " + inputGuid,
		L"VSTPlugin: Library \"TDR Nova.dll\""
	};
	const VSTPreviewEndpoint micScopedEndpoint = vstPreviewEndpointForRow(
		micScopedVstLines, 2, outputs, inputs, selectedSpeakers);
	expectTrue(micScopedEndpoint.flow == VSTPreviewEndpointFlow::Capture,
		QStringLiteral("VST rows inherit the nearest matching Device row as capture preview context"));
	expectTrue(micScopedEndpoint.deviceId == L"{0.0.1.00000000}." + inputGuid,
		QStringLiteral("VST row Device context resolves to the matching microphone endpoint"));

	const std::vector<std::wstring> unscopedVstLines{
		L"VSTPlugin: Library \"TDR Nova.dll\""
	};
	expectTrue(vstPreviewEndpointForRow(unscopedVstLines, 0, outputs, inputs, selectedSpeakers)
		== vstPreviewEndpointForSelectedDevice(selectedSpeakers),
		QStringLiteral("unscoped VST rows keep using the selected Editor device"));

	const std::vector<std::wstring> allScopedVstLines{
		L"Device: all",
		L"VSTPlugin: Library \"TDR Nova.dll\""
	};
	expectTrue(vstPreviewEndpointForRow(allScopedVstLines, 1, outputs, inputs, selectedSpeakers)
		== vstPreviewEndpointForSelectedDevice(selectedSpeakers),
		QStringLiteral("Device: all keeps the selected Editor device as the preview context"));
}
