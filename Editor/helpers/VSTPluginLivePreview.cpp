/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.

	See VSTPluginLivePreview.h for the design boundary.
*/

#include "VSTPluginLivePreview.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>

#include "helpers/ComPtr.h"
#include "helpers/VSTPluginInstance.h"

namespace
{
constexpr REFERENCE_TIME previewBufferDuration = 1000000; // 100 ms
constexpr int maxBufferedSeconds = 2;

bool isInputFlow(EDataFlow flow)
{
	return flow == eCapture;
}

EDataFlow flowForPreviewEndpoint(VSTPreviewEndpointFlow flow)
{
	return flow == VSTPreviewEndpointFlow::Capture ? eCapture : eRender;
}

struct CapturedFormat
{
	int sampleRate = 48000;
	int channelCount = 2;
	int bitsPerSample = 32;
	int bytesPerSample = 4;
	int blockAlign = 8;
	bool floatingPoint = true;
	bool pcm = false;
};

WORD extensibleSubFormatTag(const GUID& guid)
{
	static constexpr GUID tagBase = { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
	if (guid.Data2 != tagBase.Data2 || guid.Data3 != tagBase.Data3)
		return 0;
	for (int i = 0; i < 8; i++)
	{
		if (guid.Data4[i] != tagBase.Data4[i])
			return 0;
	}
	return guid.Data1 <= 0xffff ? static_cast<WORD>(guid.Data1) : 0;
}

CapturedFormat parseFormat(const WAVEFORMATEX& waveFormat)
{
	CapturedFormat result;
	result.sampleRate = static_cast<int>(waveFormat.nSamplesPerSec);
	result.channelCount = std::max<int>(1, waveFormat.nChannels);
	result.bitsPerSample = std::max<int>(8, waveFormat.wBitsPerSample);
	result.bytesPerSample = std::max<int>(1, result.bitsPerSample / 8);
	result.blockAlign = std::max<int>(result.bytesPerSample * result.channelCount, waveFormat.nBlockAlign);

	WORD tag = waveFormat.wFormatTag;
	if (waveFormat.wFormatTag == WAVE_FORMAT_EXTENSIBLE
		&& waveFormat.cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
	{
		const auto& extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE&>(waveFormat);
		tag = extensibleSubFormatTag(extensible.SubFormat);
		result.bitsPerSample = std::max<int>(8, extensible.Format.wBitsPerSample);
		result.bytesPerSample = std::max<int>(1, result.bitsPerSample / 8);
		result.blockAlign = std::max<int>(result.bytesPerSample * result.channelCount, extensible.Format.nBlockAlign);
	}

	result.floatingPoint = tag == WAVE_FORMAT_IEEE_FLOAT;
	result.pcm = tag == WAVE_FORMAT_PCM;
	return result;
}

float pcm24ToFloat(const BYTE* sample)
{
	int32_t value = static_cast<int32_t>(sample[0])
		| (static_cast<int32_t>(sample[1]) << 8)
		| (static_cast<int32_t>(sample[2]) << 16);
	if ((value & 0x00800000) != 0)
		value |= static_cast<int32_t>(0xff000000);
	return static_cast<float>(value / 8388608.0);
}

float readSample(const BYTE* frame, int channel, const CapturedFormat& format)
{
	const BYTE* sample = frame + channel * format.bytesPerSample;
	if (format.floatingPoint)
	{
		if (format.bitsPerSample == 64)
			return static_cast<float>(*reinterpret_cast<const double*>(sample));
		return *reinterpret_cast<const float*>(sample);
	}
	if (!format.pcm)
		return 0.0f;

	switch (format.bitsPerSample)
	{
	case 8:
		return (static_cast<int>(*sample) - 128) / 128.0f;
	case 16:
		return *reinterpret_cast<const int16_t*>(sample) / 32768.0f;
	case 24:
		return pcm24ToFloat(sample);
	case 32:
		return *reinterpret_cast<const int32_t*>(sample) / 2147483648.0f;
	default:
		return 0.0f;
	}
}
}

class VSTPluginLivePreview::WasapiCapture
{
public:
	WasapiCapture(EDataFlow flow, ERole role)
		: flow(flow), role(role)
	{
	}

	explicit WasapiCapture(const VSTPreviewEndpoint& endpoint)
		: flow(flowForPreviewEndpoint(endpoint.flow)),
		role(eConsole),
		endpointDeviceId(endpoint.deviceId)
	{
	}

	~WasapiCapture()
	{
		stop();
	}

	bool start()
	{
		if (worker.joinable())
			return isReady();

		stopRequested.store(false, std::memory_order_release);
		{
			std::lock_guard<std::mutex> lock(mutex);
			readyState = false;
			failedState = false;
			bufferedSamples.clear();
		}

		worker = std::thread([this] { run(); });

		std::unique_lock<std::mutex> lock(mutex);
		readyCondition.wait_for(lock, std::chrono::milliseconds(800), [this] {
			return readyState || failedState;
		});
		return readyState && !failedState;
	}

	void stop()
	{
		stopRequested.store(true, std::memory_order_release);
		if (worker.joinable())
			worker.join();

		std::lock_guard<std::mutex> lock(mutex);
		readyState = false;
		failedState = false;
		bufferedSamples.clear();
	}

	int sampleRate() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return format.sampleRate;
	}

	bool isReady() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return readyState && !failedState;
	}

	bool popAdd(float** output, int outputChannels, int frames)
	{
		return popAddImpl(output, outputChannels, frames);
	}

	bool popAdd(double** output, int outputChannels, int frames)
	{
		return popAddImpl(output, outputChannels, frames);
	}

private:
	template<typename Sample>
	bool popAddImpl(Sample** output, int outputChannels, int frames)
	{
		if (outputChannels <= 0 || frames <= 0)
			return false;

		std::lock_guard<std::mutex> lock(mutex);
		const int sourceChannels = std::max(1, format.channelCount);
		bool consumedAny = false;
		for (int frame = 0; frame < frames; frame++)
		{
			const bool hasSourceFrame = bufferedSamples.size() >= static_cast<size_t>(sourceChannels);
			consumedAny = consumedAny || hasSourceFrame;
			for (int channel = 0; channel < outputChannels; channel++)
			{
				float value = 0.0f;
				if (hasSourceFrame)
				{
					if (outputChannels == 1 && sourceChannels > 1)
					{
						for (int source = 0; source < sourceChannels; source++)
							value += bufferedSamples[source];
						value /= static_cast<float>(sourceChannels);
					}
					else if (channel < sourceChannels)
					{
						value = bufferedSamples[channel];
					}
				}
				output[channel][frame] += static_cast<Sample>(value);
			}
			if (hasSourceFrame)
			{
				for (int source = 0; source < sourceChannels; source++)
					bufferedSamples.pop_front();
			}
		}
		return consumedAny;
	}

	void signalFailure()
	{
		std::lock_guard<std::mutex> lock(mutex);
		failedState = true;
		readyCondition.notify_all();
	}

	void signalReady(const CapturedFormat& capturedFormat)
	{
		std::lock_guard<std::mutex> lock(mutex);
		format = capturedFormat;
		readyState = true;
		readyCondition.notify_all();
	}

	void append(const BYTE* data, UINT32 frameCount, DWORD flags, const CapturedFormat& capturedFormat)
	{
		if (frameCount == 0)
			return;

		std::lock_guard<std::mutex> lock(mutex);
		const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
		const int channels = std::max(1, capturedFormat.channelCount);
		const size_t maxSamples = static_cast<size_t>(std::max(1, capturedFormat.sampleRate))
			* static_cast<size_t>(channels) * maxBufferedSeconds;
		for (UINT32 frame = 0; frame < frameCount; frame++)
		{
			const BYTE* frameData = silent ? nullptr : data + static_cast<size_t>(frame) * capturedFormat.blockAlign;
			for (int channel = 0; channel < channels; channel++)
			{
				bufferedSamples.push_back(silent
					? 0.0f : readSample(frameData, channel, capturedFormat));
			}
		}
		while (bufferedSamples.size() > maxSamples)
			bufferedSamples.pop_front();
	}

	void run()
	{
		winutil::ComApartment com(COINIT_MULTITHREADED);
		if (!com.isUsable())
		{
			signalFailure();
			return;
		}

		winutil::ComPtr<IMMDeviceEnumerator> enumerator;
		HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
			IID_PPV_ARGS(enumerator.put()));
		if (FAILED(hr))
		{
			signalFailure();
			return;
		}

		winutil::ComPtr<IMMDevice> device;
		if (!endpointDeviceId.empty())
			hr = enumerator->GetDevice(endpointDeviceId.c_str(), device.put());
		else
			hr = enumerator->GetDefaultAudioEndpoint(flow, role, device.put());
		if (FAILED(hr))
		{
			signalFailure();
			return;
		}

		winutil::ComPtr<IAudioClient> audioClient;
		hr = device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr,
			reinterpret_cast<void**>(audioClient.put()));
		if (FAILED(hr))
		{
			signalFailure();
			return;
		}

		winutil::CoTaskMem<WAVEFORMATEX> mixFormat;
		hr = audioClient->GetMixFormat(mixFormat.put());
		if (FAILED(hr) || !mixFormat)
		{
			signalFailure();
			return;
		}
		const CapturedFormat capturedFormat = parseFormat(*mixFormat.get());

		const DWORD streamFlags = isInputFlow(flow) ? 0 : AUDCLNT_STREAMFLAGS_LOOPBACK;
		hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags,
			previewBufferDuration, 0, mixFormat.get(), nullptr);
		if (FAILED(hr))
		{
			signalFailure();
			return;
		}

		winutil::ComPtr<IAudioCaptureClient> captureClient;
		hr = audioClient->GetService(__uuidof(IAudioCaptureClient),
			reinterpret_cast<void**>(captureClient.put()));
		if (FAILED(hr))
		{
			signalFailure();
			return;
		}

		hr = audioClient->Start();
		if (FAILED(hr))
		{
			signalFailure();
			return;
		}
		signalReady(capturedFormat);

		while (!stopRequested.load(std::memory_order_acquire))
		{
			UINT32 packetFrames = 0;
			hr = captureClient->GetNextPacketSize(&packetFrames);
			if (FAILED(hr))
				break;
			while (packetFrames > 0)
			{
				BYTE* data = nullptr;
				UINT32 frameCount = 0;
				DWORD flags = 0;
				hr = captureClient->GetBuffer(&data, &frameCount, &flags, nullptr, nullptr);
				if (FAILED(hr))
					break;
				append(data, frameCount, flags, capturedFormat);
				captureClient->ReleaseBuffer(frameCount);
				hr = captureClient->GetNextPacketSize(&packetFrames);
				if (FAILED(hr))
					break;
			}
			Sleep(5);
		}

		audioClient->Stop();
	}

	const EDataFlow flow;
	const ERole role;
	const std::wstring endpointDeviceId;
	mutable std::mutex mutex;
	std::condition_variable readyCondition;
	std::thread worker;
	std::atomic<bool> stopRequested{ false };
	CapturedFormat format;
	bool readyState = false;
	bool failedState = false;
	std::deque<float> bufferedSamples;
};

VSTPluginLivePreview::VSTPluginLivePreview()
	: selectedEndpointCapture(nullptr),
	inputCapture(std::make_unique<WasapiCapture>(eCapture, eConsole)),
	communicationsInputCapture(std::make_unique<WasapiCapture>(eCapture, eCommunications)),
	playbackCapture(std::make_unique<WasapiCapture>(eRender, eConsole))
{
}

VSTPluginLivePreview::~VSTPluginLivePreview()
{
	stop();
}

void VSTPluginLivePreview::setEnabled(bool value)
{
	enabled = value;
	if (!enabled)
		stop();
}

bool VSTPluginLivePreview::isEnabled() const
{
	return enabled;
}

void VSTPluginLivePreview::update(VSTPluginInstance* targetEffect, bool panelVisible, const VSTPreviewEndpoint& previewEndpoint)
{
	if (!enabled || !panelVisible || targetEffect == nullptr)
	{
		stop();
		return;
	}
	start(targetEffect, previewEndpoint);
}

void VSTPluginLivePreview::start(VSTPluginInstance* targetEffect, const VSTPreviewEndpoint& previewEndpoint)
{
	if (active && effect == targetEffect && activeEndpoint == previewEndpoint)
		return;
	stop();

	effect = targetEffect;
	activeEndpoint = previewEndpoint;
	inputChannelCount = std::max(0, effect->numInputs());
	outputChannelCount = std::max(0, effect->numOutputs());
	if (inputChannelCount == 0 && outputChannelCount == 0)
	{
		effect = nullptr;
		return;
	}
	bool selectedEndpointReady = false;
	if (activeEndpoint.isValid())
	{
		selectedEndpointCapture = std::make_unique<WasapiCapture>(activeEndpoint);
		selectedEndpointReady = selectedEndpointCapture->start();
		if (!selectedEndpointReady)
		{
			selectedEndpointCapture->stop();
			selectedEndpointCapture.reset();
		}
	}

	bool inputReady = false;
	bool communicationsInputReady = false;
	bool playbackReady = false;
	if (!selectedEndpointReady)
	{
		inputReady = inputCapture->start();
		communicationsInputReady = communicationsInputCapture->start();
		playbackReady = playbackCapture->start();
	}
	if (!selectedEndpointReady && !inputReady && !communicationsInputReady && !playbackReady)
	{
		effect = nullptr;
		return;
	}

	allocateBuffers(inputChannelCount, outputChannelCount);
	const int previewSampleRate = selectedEndpointReady ? selectedEndpointCapture->sampleRate()
		: inputReady ? inputCapture->sampleRate()
		: communicationsInputReady ? communicationsInputCapture->sampleRate()
		: playbackCapture->sampleRate();
	this->previewSampleRate = previewSampleRate;
	effect->prepareForProcessing(static_cast<float>(this->previewSampleRate), blockSize);
	effect->startProcessing();
	active = true;
	processingStopRequested.store(false, std::memory_order_release);
	processingWorker = std::thread([this] { processLoop(); });
}

void VSTPluginLivePreview::stop()
{
	processingStopRequested.store(true, std::memory_order_release);
	if (processingWorker.joinable())
		processingWorker.join();
	if (active && effect != nullptr)
		effect->stopProcessingSafely();
	active = false;
	effect = nullptr;
	activeEndpoint = {};
	if (selectedEndpointCapture != nullptr)
		selectedEndpointCapture->stop();
	selectedEndpointCapture.reset();
	inputCapture->stop();
	communicationsInputCapture->stop();
	playbackCapture->stop();
}

void VSTPluginLivePreview::allocateBuffers(int inputChannels, int outputChannels)
{
	floatInputs.assign(inputChannels, std::vector<float>(blockSize, 0.0f));
	floatOutputs.assign(outputChannels, std::vector<float>(blockSize, 0.0f));
	floatInputPtrs.resize(inputChannels);
	floatOutputPtrs.resize(outputChannels);
	for (int i = 0; i < inputChannels; i++)
		floatInputPtrs[i] = floatInputs[i].data();
	for (int i = 0; i < outputChannels; i++)
		floatOutputPtrs[i] = floatOutputs[i].data();

	doubleInputs.assign(inputChannels, std::vector<double>(blockSize, 0.0));
	doubleOutputs.assign(outputChannels, std::vector<double>(blockSize, 0.0));
	doubleInputPtrs.resize(inputChannels);
	doubleOutputPtrs.resize(outputChannels);
	for (int i = 0; i < inputChannels; i++)
		doubleInputPtrs[i] = doubleInputs[i].data();
	for (int i = 0; i < outputChannels; i++)
		doubleOutputPtrs[i] = doubleOutputs[i].data();
}

void VSTPluginLivePreview::processLoop()
{
	const double secondsPerBlock = static_cast<double>(blockSize) / std::max(1, previewSampleRate);
	const auto blockDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
		std::chrono::duration<double>(secondsPerBlock));
	auto nextBlock = std::chrono::steady_clock::now();
	while (!processingStopRequested.load(std::memory_order_acquire))
	{
		processOneBlock();
		nextBlock += blockDuration;
		const auto now = std::chrono::steady_clock::now();
		if (nextBlock < now - std::chrono::milliseconds(50))
			nextBlock = now + blockDuration;
		std::this_thread::sleep_until(nextBlock);
	}
}

void VSTPluginLivePreview::processOneBlock()
{
	if (!active || effect == nullptr)
		return;

	if (effect->canDoubleReplacing())
	{
		for (auto& input : doubleInputs)
			std::fill(input.begin(), input.end(), 0.0);
		int sourceCount = 0;
		if (selectedEndpointCapture != nullptr && selectedEndpointCapture->popAdd(doubleInputPtrs.data(), inputChannelCount, blockSize))
			sourceCount++;
		else
		{
			if (inputCapture->popAdd(doubleInputPtrs.data(), inputChannelCount, blockSize))
				sourceCount++;
			if (communicationsInputCapture->popAdd(doubleInputPtrs.data(), inputChannelCount, blockSize))
				sourceCount++;
			if (playbackCapture->popAdd(doubleInputPtrs.data(), inputChannelCount, blockSize))
				sourceCount++;
		}
		if (sourceCount > 1)
		{
			const double scale = 1.0 / sourceCount;
			for (auto& input : doubleInputs)
			{
				for (double& sample : input)
					sample *= scale;
			}
		}
		for (auto& output : doubleOutputs)
			std::fill(output.begin(), output.end(), 0.0);
		effect->processDoubleReplacing(doubleInputPtrs.data(), doubleOutputPtrs.data(), blockSize);
	}
	else
	{
		for (auto& input : floatInputs)
			std::fill(input.begin(), input.end(), 0.0f);
		int sourceCount = 0;
		if (selectedEndpointCapture != nullptr && selectedEndpointCapture->popAdd(floatInputPtrs.data(), inputChannelCount, blockSize))
			sourceCount++;
		else
		{
			if (inputCapture->popAdd(floatInputPtrs.data(), inputChannelCount, blockSize))
				sourceCount++;
			if (communicationsInputCapture->popAdd(floatInputPtrs.data(), inputChannelCount, blockSize))
				sourceCount++;
			if (playbackCapture->popAdd(floatInputPtrs.data(), inputChannelCount, blockSize))
				sourceCount++;
		}
		if (sourceCount > 1)
		{
			const float scale = 1.0f / sourceCount;
			for (auto& input : floatInputs)
			{
				for (float& sample : input)
					sample *= scale;
			}
		}
		for (auto& output : floatOutputs)
			std::fill(output.begin(), output.end(), 0.0f);
		if (effect->canReplacing())
			effect->processReplacing(floatInputPtrs.data(), floatOutputPtrs.data(), blockSize);
		else
			effect->process(floatInputPtrs.data(), floatOutputPtrs.data(), blockSize);
	}
}
