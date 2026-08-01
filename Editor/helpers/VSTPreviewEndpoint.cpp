/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "VSTPreviewEndpoint.h"

#include "AbstractAPOInfo.h"
#include "filters/DeviceCommand.h"
#include "helpers/StringHelper.h"

namespace
{
const wchar_t* renderEndpointPrefix = L"{0.0.0.00000000}.";
const wchar_t* captureEndpointPrefix = L"{0.0.1.00000000}.";

bool startsWith(const std::wstring& value, const wchar_t* prefix)
{
	return value.rfind(prefix, 0) == 0;
}

bool parseConfigCommand(const std::wstring& line, std::wstring& command, std::wstring& parameters)
{
	const std::wstring trimmed = StringHelper::trim(line);
	if (trimmed.empty() || trimmed[0] == L'#')
		return false;

	const size_t separator = trimmed.find(L':');
	if (separator == std::wstring::npos)
		return false;

	command = StringHelper::trim(trimmed.substr(0, separator));
	parameters = trimmed.substr(separator + 1);
	return true;
}

bool isAllDevicesCommand(const DeviceCommand& command)
{
	return command.patterns.size() == 1
		&& command.patterns[0].size() == 1
		&& StringHelper::toLowerCase(command.patterns[0][0]) == L"all";
}

void appendMatchingDevices(std::vector<std::shared_ptr<AbstractAPOInfo>>& matches,
	const std::vector<std::shared_ptr<AbstractAPOInfo>>& devices, const DeviceCommand& command)
{
	for (const std::shared_ptr<AbstractAPOInfo>& device : devices)
	{
		if (device != nullptr && command.matches(device->getDeviceString()))
			matches.push_back(device);
	}
}

std::shared_ptr<AbstractAPOInfo> resolveDeviceCommand(
	const DeviceCommand& command,
	const std::vector<std::shared_ptr<AbstractAPOInfo>>& outputDevices,
	const std::vector<std::shared_ptr<AbstractAPOInfo>>& inputDevices,
	const std::shared_ptr<AbstractAPOInfo>& selectedDevice)
{
	if (isAllDevicesCommand(command))
		return selectedDevice;

	if (selectedDevice != nullptr && command.matches(selectedDevice->getDeviceString()))
		return selectedDevice;

	std::vector<std::shared_ptr<AbstractAPOInfo>> matches;
	appendMatchingDevices(matches, outputDevices, command);
	appendMatchingDevices(matches, inputDevices, command);

	if (matches.size() == 1)
		return matches[0];

	return nullptr;
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

std::shared_ptr<AbstractAPOInfo> vstPreviewDeviceForRow(
	const std::vector<std::wstring>& lines,
	size_t rowIndex,
	const std::vector<std::shared_ptr<AbstractAPOInfo>>& outputDevices,
	const std::vector<std::shared_ptr<AbstractAPOInfo>>& inputDevices,
	const std::shared_ptr<AbstractAPOInfo>& selectedDevice)
{
	std::shared_ptr<AbstractAPOInfo> previewDevice = selectedDevice;
	const size_t end = std::min(rowIndex, lines.size());
	for (size_t i = 0; i < end; i++)
	{
		std::wstring commandName;
		std::wstring parameters;
		if (!parseConfigCommand(lines[i], commandName, parameters))
			continue;

		DeviceCommand deviceCommand;
		if (DeviceCommand::parse(commandName, parameters, deviceCommand))
		{
			previewDevice = resolveDeviceCommand(deviceCommand,
				outputDevices, inputDevices, selectedDevice);
		}
	}

	return previewDevice;
}

VSTPreviewEndpoint vstPreviewEndpointForRow(
	const std::vector<std::wstring>& lines,
	size_t rowIndex,
	const std::vector<std::shared_ptr<AbstractAPOInfo>>& outputDevices,
	const std::vector<std::shared_ptr<AbstractAPOInfo>>& inputDevices,
	const std::shared_ptr<AbstractAPOInfo>& selectedDevice)
{
	return vstPreviewEndpointForSelectedDevice(vstPreviewDeviceForRow(
		lines, rowIndex, outputDevices, inputDevices, selectedDevice));
}
