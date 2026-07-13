/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Jonas Thedering

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "stdafx.h"
#include <wincrypt.h>
#include "StringHelper.h"
#include "VSTPluginLibrary.h"
#include "VSTPluginInstance.h"
#include "VSTPluginInstanceInternal.h"

using namespace std;
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace
{
constexpr uint32 vst3HostStateMagic = 0x54533345; // E3ST
constexpr uint32 vst3HostStateVersion = 1;

#pragma pack(push, 1)
struct VST3HostStateHeader
{
	uint32 magic = vst3HostStateMagic;
	uint32 version = vst3HostStateVersion;
	uint32 componentSize = 0;
	uint32 controllerSize = 0;
	uint32 parameterCount = 0;
};

struct VST3HostParameterState
{
	ParamID id = 0;
	ParamValue value = 0.0;
};
#pragma pack(pop)

bool decodeBase64(const wstring& encoded, vector<char>& data)
{
	DWORD size = 0;
	if (encoded.empty() || CryptStringToBinaryW(encoded.c_str(), 0, CRYPT_STRING_BASE64, NULL, &size, NULL, NULL) == FALSE)
		return false;
	data.resize(size);
	return size > 0 && CryptStringToBinaryW(encoded.c_str(), 0, CRYPT_STRING_BASE64,
		(BYTE*)data.data(), &size, NULL, NULL) == TRUE;
}

wstring encodeBase64(const vector<char>& data)
{
	if (data.empty() || data.size() > MAXDWORD)
		return L"";
	DWORD length = 0;
	CryptBinaryToStringW((BYTE*)data.data(), (DWORD)data.size(),
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &length);
	vector<wchar_t> encoded(length);
	if (length == 0 || CryptBinaryToStringW((BYTE*)data.data(), (DWORD)data.size(),
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, encoded.data(), &length) == FALSE)
		return L"";
	return encoded.data();
}

vector<char> combineVST3State(const vector<char>& component, const vector<char>& controller,
	const vector<VST3HostParameterState>& parameters)
{
	if (controller.empty())
		return component;
	if (component.size() > UINT32_MAX || controller.size() > UINT32_MAX || parameters.size() > UINT32_MAX)
		return component;
	const size_t parameterBytes = parameters.size() * sizeof(VST3HostParameterState);
	if (component.size() > SIZE_MAX - sizeof(VST3HostStateHeader) - controller.size()
		|| component.size() + sizeof(VST3HostStateHeader) + controller.size() > SIZE_MAX - parameterBytes)
		return component;
	VST3HostStateHeader header;
	header.componentSize = (uint32)component.size();
	header.controllerSize = (uint32)controller.size();
	header.parameterCount = (uint32)parameters.size();
	vector<char> combined(sizeof(header) + component.size() + controller.size() + parameterBytes);
	memcpy(combined.data(), &header, sizeof(header));
	if (!component.empty())
		memcpy(combined.data() + sizeof(header), component.data(), component.size());
	memcpy(combined.data() + sizeof(header) + component.size(), controller.data(), controller.size());
	if (!parameters.empty())
		memcpy(combined.data() + sizeof(header) + component.size() + controller.size(),
			parameters.data(), parameterBytes);
	return combined;
}

void splitVST3State(const vector<char>& combined, vector<char>& component, vector<char>& controller,
	vector<VST3HostParameterState>& parameters)
{
	component = combined;
	controller.clear();
	parameters.clear();
	if (combined.size() < sizeof(VST3HostStateHeader))
		return;
	VST3HostStateHeader header;
	memcpy(&header, combined.data(), sizeof(header));
	const uint64 parameterBytes = (uint64)header.parameterCount * sizeof(VST3HostParameterState);
	const uint64 payloadSize = (uint64)header.componentSize + header.controllerSize + parameterBytes;
	if (header.magic != vst3HostStateMagic || header.version != vst3HostStateVersion
		|| payloadSize != (uint64)(combined.size() - sizeof(header)))
		return;
	component.assign(combined.begin() + sizeof(header), combined.begin() + sizeof(header) + header.componentSize);
	const size_t controllerStart = sizeof(header) + header.componentSize;
	controller.assign(combined.begin() + controllerStart, combined.begin() + controllerStart + header.controllerSize);
	parameters.resize(header.parameterCount);
	if (!parameters.empty())
		memcpy(parameters.data(), combined.data() + controllerStart + header.controllerSize, (size_t)parameterBytes);
}
}

void VSTPluginInstance::writeToEffect(const std::wstring& chunkData, const std::unordered_map<std::wstring, float>& paramMap)
{
	if (library->isVST3())
	{
		if (chunkData != L"")
		{
			vector<char> data;
			if (decodeBase64(chunkData, data))
			{
				vector<char> componentState;
				vector<char> controllerState;
				vector<VST3HostParameterState> parameters;
				splitVST3State(data, componentState, controllerState, parameters);
				if (!componentState.empty())
				{
					VST3MemoryStream* stream = new VST3MemoryStream(componentState);
					if (vst3Component != NULL)
						vst3Component->setState(stream);
					stream->seek(0, IBStream::kIBSeekSet);
					if (vst3Controller != NULL)
						vst3Controller->setComponentState(stream);
					stream->release();
				}
				if (vst3Controller != NULL && !controllerState.empty())
				{
					VST3MemoryStream* stream = new VST3MemoryStream(controllerState);
					vst3Controller->setState(stream);
					stream->release();
				}
				bool parameterQueued = false;
				for (const VST3HostParameterState& parameter : parameters)
				{
					if (vst3Controller != NULL)
						vst3Controller->setParamNormalized(parameter.id, parameter.value);
					queueVST3ParameterEdit(parameter.id, parameter.value);
					parameterQueued = true;
				}
				if (parameterQueued)
					flushVST3ParameterChanges();
			}
		}
		else if (vst3Controller != NULL)
		{
			bool parameterQueued = false;
			for (auto it : paramMap)
			{
				for (int32 i = 0; i < vst3Controller->getParameterCount(); i++)
				{
					ParameterInfo info;
					if (vst3Controller->getParameterInfo(i, info) == kResultOk
						&& it.first == wstring((wchar_t*)info.title))
					{
						vst3Controller->setParamNormalized(info.id, it.second);
						queueVST3ParameterEdit(info.id, it.second);
						parameterQueued = true;
						break;
					}
				}
			}
			if (parameterQueued)
				flushVST3ParameterChanges();
		}
		return;
	}

	if (effect == NULL)
		return;

	if (effect->flags & VST_EFFECT_FLAG_CHUNKS)
	{
		if (chunkData != L"")
		{
			DWORD bufSize = 0;
			CryptStringToBinaryW(chunkData.c_str(), 0, CRYPT_STRING_BASE64, NULL, &bufSize, NULL, NULL);
			BYTE* buf = new BYTE[bufSize];
			if (CryptStringToBinaryW(chunkData.c_str(), 0, CRYPT_STRING_BASE64, buf, &bufSize, NULL, NULL) == TRUE)
				effect->control(effect, VST_EFFECT_OPCODE_SET_CHUNK_DATA, 1, bufSize, buf, 0.0f);
			delete[] buf;
		}
	}
	else
	{
		for (int i = 0; i < effect->num_params; i++)
		{
			char buf[256];
			effect->control(effect, VST_EFFECT_OPCODE_PARAM_GET_NAME, i, 0, buf, 0.0f);
			buf[255] = '\0'; // just to be sure
			wstring name = StringHelper::toWString(buf, CP_UTF8);
			auto it = paramMap.find(name);
			if (it != paramMap.end())
				effect->set_parameter(effect, i, it->second);
		}
	}
}

void VSTPluginInstance::readFromEffect(std::wstring& chunkData, std::unordered_map<std::wstring, float>& paramMap) const
{
	if (library->isVST3())
	{
		chunkData = L"";
		paramMap.clear();

		auto readPluginState = [](auto* plugin) {
			vector<char> data;
			if (plugin != NULL)
			{
				VST3MemoryStream* stream = new VST3MemoryStream();
				if (plugin->getState(stream) == kResultOk)
					data = stream->getData();
				stream->release();
			}
			return data;
		};
		const vector<char> componentState = readPluginState(vst3Component);
		const vector<char> controllerState = vst3ControllerInitializedSeparately
			? readPluginState(vst3Controller) : vector<char>();
		vector<VST3HostParameterState> parameters;
		if (vst3Controller != NULL)
		{
			for (int32 i = 0; i < vst3Controller->getParameterCount(); i++)
			{
				ParameterInfo info;
				if (vst3Controller->getParameterInfo(i, info) == kResultOk)
					parameters.push_back({ info.id, vst3Controller->getParamNormalized(info.id) });
			}
		}
		if (!componentState.empty() || !controllerState.empty())
			chunkData = encodeBase64(combineVST3State(componentState, controllerState, parameters));

		if (chunkData == L"" && vst3Controller != NULL)
		{
			for (int32 i = 0; i < vst3Controller->getParameterCount(); i++)
			{
				ParameterInfo info;
				if (vst3Controller->getParameterInfo(i, info) == kResultOk)
					paramMap[wstring((wchar_t*)info.title)] = (float)vst3Controller->getParamNormalized(info.id);
			}
		}
		return;
	}

	if (effect == NULL)
		return;

	chunkData = L"";
	paramMap.clear();

	if (effect->flags & VST_EFFECT_FLAG_CHUNKS)
	{
		BYTE* chunk = NULL;
		int size = (int)effect->control(effect, VST_EFFECT_OPCODE_GET_CHUNK_DATA, 1, 0, &chunk, 0.0f);
		DWORD stringLength = 0;
		CryptBinaryToStringW(chunk, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &stringLength);
		wchar_t* string = new wchar_t[stringLength];
		if (CryptBinaryToStringW(chunk, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, string, &stringLength) == TRUE)
			chunkData = string;
		delete[] string;
	}
	else
	{
		for (int i = 0; i < effect->num_params; i++)
		{
			char buf[256];
			effect->control(effect, VST_EFFECT_OPCODE_PARAM_GET_NAME, i, 0, buf, 0.0f);
			buf[255] = '\0'; // just to be sure
			float value = effect->get_parameter(effect, i);
			paramMap[StringHelper::toWString(buf, CP_UTF8)] = value;
		}
	}
}
