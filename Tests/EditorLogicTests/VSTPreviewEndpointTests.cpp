/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2026  EqualizerAPO-XT contributors

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "EditorLogicTestSupport.h"

#include "Editor/helpers/VSTPreviewEndpoint.h"

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
}
