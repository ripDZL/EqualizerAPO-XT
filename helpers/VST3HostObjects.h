#pragma once

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/vst/ivstattributes.h"
#include "pluginterfaces/vst/ivstmessage.h"

namespace VST3HostObjects
{
using namespace Steinberg;
using namespace Steinberg::Vst;

class AttributeList final : public IAttributeList
{
public:
	tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
	{
		QUERY_INTERFACE(iid, obj, FUnknown::iid, IAttributeList)
		QUERY_INTERFACE(iid, obj, IAttributeList::iid, IAttributeList)
		*obj = nullptr;
		return kNoInterface;
	}
	uint32 PLUGIN_API addRef() override { return ++refCount; }
	uint32 PLUGIN_API release() override
	{
		uint32 result = --refCount;
		if (result == 0)
			delete this;
		return result;
	}

	tresult PLUGIN_API setInt(AttrID id, int64 value) override
	{
		if (id == nullptr)
			return kInvalidArgument;
		std::lock_guard<std::mutex> lock(dataMutex);
		intValues[id] = value;
		return kResultOk;
	}
	tresult PLUGIN_API getInt(AttrID id, int64& value) override
	{
		std::lock_guard<std::mutex> lock(dataMutex);
		auto it = id != nullptr ? intValues.find(id) : intValues.end();
		if (it == intValues.end())
			return kResultFalse;
		value = it->second;
		return kResultOk;
	}
	tresult PLUGIN_API setFloat(AttrID id, double value) override
	{
		if (id == nullptr)
			return kInvalidArgument;
		std::lock_guard<std::mutex> lock(dataMutex);
		floatValues[id] = value;
		return kResultOk;
	}
	tresult PLUGIN_API getFloat(AttrID id, double& value) override
	{
		std::lock_guard<std::mutex> lock(dataMutex);
		auto it = id != nullptr ? floatValues.find(id) : floatValues.end();
		if (it == floatValues.end())
			return kResultFalse;
		value = it->second;
		return kResultOk;
	}
	tresult PLUGIN_API setString(AttrID id, const TChar* value) override
	{
		if (id == nullptr || value == nullptr)
			return kInvalidArgument;
		std::lock_guard<std::mutex> lock(dataMutex);
		std::vector<TChar>& stored = stringValues[id];
		stored.assign(value, value + stringLength(value) + 1);
		return kResultOk;
	}
	tresult PLUGIN_API getString(AttrID id, TChar* value, uint32 sizeInBytes) override
	{
		if (value == nullptr || sizeInBytes < sizeof(TChar))
			return kInvalidArgument;
		std::lock_guard<std::mutex> lock(dataMutex);
		auto it = id != nullptr ? stringValues.find(id) : stringValues.end();
		if (it == stringValues.end() || sizeInBytes < it->second.size() * sizeof(TChar))
			return kResultFalse;
		memcpy(value, it->second.data(), it->second.size() * sizeof(TChar));
		return kResultOk;
	}
	tresult PLUGIN_API setBinary(AttrID id, const void* value, uint32 sizeInBytes) override
	{
		if (id == nullptr || (value == nullptr && sizeInBytes != 0))
			return kInvalidArgument;
		std::lock_guard<std::mutex> lock(dataMutex);
		const char* bytes = static_cast<const char*>(value);
		binaryValues[id] = sizeInBytes == 0 ? std::vector<char>() : std::vector<char>(bytes, bytes + sizeInBytes);
		return kResultOk;
	}
	tresult PLUGIN_API getBinary(AttrID id, const void*& value, uint32& sizeInBytes) override
	{
		std::lock_guard<std::mutex> lock(dataMutex);
		auto it = id != nullptr ? binaryValues.find(id) : binaryValues.end();
		if (it == binaryValues.end())
			return kResultFalse;
		value = it->second.empty() ? nullptr : it->second.data();
		sizeInBytes = (uint32)it->second.size();
		return kResultOk;
	}

private:
	static size_t stringLength(const TChar* value)
	{
		size_t length = 0;
		while (value[length] != 0)
			length++;
		return length;
	}

	std::atomic<uint32> refCount{ 1 };
	std::mutex dataMutex;
	std::unordered_map<std::string, int64> intValues;
	std::unordered_map<std::string, double> floatValues;
	std::unordered_map<std::string, std::vector<TChar>> stringValues;
	std::unordered_map<std::string, std::vector<char>> binaryValues;
};

class Message final : public IMessage
{
public:
	Message() : attributes(new AttributeList()) {}

	tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
	{
		QUERY_INTERFACE(iid, obj, FUnknown::iid, IMessage)
		QUERY_INTERFACE(iid, obj, IMessage::iid, IMessage)
		*obj = nullptr;
		return kNoInterface;
	}
	uint32 PLUGIN_API addRef() override { return ++refCount; }
	uint32 PLUGIN_API release() override
	{
		uint32 result = --refCount;
		if (result == 0)
			delete this;
		return result;
	}
	FIDString PLUGIN_API getMessageID() override { return messageId.empty() ? nullptr : messageId.c_str(); }
	void PLUGIN_API setMessageID(FIDString id) override { messageId = id != nullptr ? id : ""; }
	IAttributeList* PLUGIN_API getAttributes() override { return attributes; }

private:
	~Message() { attributes->release(); }

	std::atomic<uint32> refCount{ 1 };
	std::string messageId;
	AttributeList* attributes;
};

inline tresult createInstance(TUID cid, TUID iid, void** obj)
{
	if (obj == nullptr)
		return kInvalidArgument;
	*obj = nullptr;
	if (FUnknownPrivate::iidEqual(cid, IMessage::iid)
		&& (FUnknownPrivate::iidEqual(iid, IMessage::iid) || FUnknownPrivate::iidEqual(iid, FUnknown::iid)))
	{
		*obj = static_cast<IMessage*>(new Message());
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual(cid, IAttributeList::iid)
		&& (FUnknownPrivate::iidEqual(iid, IAttributeList::iid) || FUnknownPrivate::iidEqual(iid, FUnknown::iid)))
	{
		*obj = static_cast<IAttributeList*>(new AttributeList());
		return kResultOk;
	}
	return kNoInterface;
}
}
