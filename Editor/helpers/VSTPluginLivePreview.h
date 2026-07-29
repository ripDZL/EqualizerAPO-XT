/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.

	Editor-side live preview feed for VST plugin panels. The audio service owns
	the real processing instances; the editor owns a separate instance for the
	visible plug-in GUI. This helper feeds copied endpoint audio into that
	visible instance so analyzer-style plug-in UIs can animate while the panel is
	open, without routing the preview audio back to the device.
*/

#pragma once

#include <memory>
#include <vector>

#include <QTimer>

#include "Editor/helpers/VSTPreviewEndpoint.h"

class VSTPluginInstance;

class VSTPluginLivePreview
{
public:
	VSTPluginLivePreview();
	~VSTPluginLivePreview();

	void setEnabled(bool enabled);
	bool isEnabled() const;
	void update(VSTPluginInstance* effect, bool panelVisible, const VSTPreviewEndpoint& previewEndpoint = {});
	void stop();

private:
	class WasapiCapture;

	void start(VSTPluginInstance* effect, const VSTPreviewEndpoint& previewEndpoint);
	void processBlock();
	void allocateBuffers(int inputChannels, int outputChannels);

	std::unique_ptr<WasapiCapture> selectedEndpointCapture;
	std::unique_ptr<WasapiCapture> inputCapture;
	std::unique_ptr<WasapiCapture> communicationsInputCapture;
	std::unique_ptr<WasapiCapture> playbackCapture;
	QTimer timer;
	VSTPluginInstance* effect = nullptr;
	VSTPreviewEndpoint activeEndpoint;
	bool enabled = true;
	bool active = false;
	int inputChannelCount = 0;
	int outputChannelCount = 0;
	static constexpr int blockSize = 512;

	std::vector<std::vector<float>> floatInputs;
	std::vector<std::vector<float>> floatOutputs;
	std::vector<float*> floatInputPtrs;
	std::vector<float*> floatOutputPtrs;
	std::vector<std::vector<double>> doubleInputs;
	std::vector<std::vector<double>> doubleOutputs;
	std::vector<double*> doubleInputPtrs;
	std::vector<double*> doubleOutputPtrs;
};
