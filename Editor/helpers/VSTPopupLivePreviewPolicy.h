/*
    This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#pragma once

#include <algorithm>
#include <cwctype>
#include <string>

namespace VSTPopupLivePreviewPolicy
{
enum class FeedPath
{
	None,
	SelectedEndpoint,
	PanelPreview
};

// Kept separate from the widget classes so the native-panel preview policy
// can be regression-tested without constructing a VST or a QWidget.
inline bool isBertomDenoiserClassic(const std::wstring& libraryPath)
{
	std::wstring normalized = libraryPath;
	std::transform(normalized.begin(), normalized.end(), normalized.begin(),
		[](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
	// The supplied VST3 bundle has this stable module name. Do not broaden
	// this to every "denoiser": normal native panels retain the feed.
	return normalized.find(L"bertom_denoiserclassic") != std::wstring::npos;
}

inline bool shouldRun(bool requested, bool embedded, bool nativePanelOpen,
	const std::wstring& libraryPath)
{
	if (!requested)
		return false;
	if (embedded)
		return true;
	// The popup dialog and preview worker share one VST controller. Bertom
	// Denoiser Classic terminates the Editor when process() overlaps that
	// session, so keep its native panel safe while restoring other panels.
	return nativePanelOpen && !isBertomDenoiserClassic(libraryPath);
}

// A selected endpoint preserves the fork's microphone/device analyzer feed.
// Otherwise, use the upstream panel feeder, which also monitors plug-in
// generated signal. The paths must never run together on one editor instance.
inline FeedPath selectFeedPath(bool requested, bool hasSelectedEndpoint,
	bool embedded, bool nativePanelOpen, const std::wstring& libraryPath)
{
	if (!shouldRun(requested, embedded, nativePanelOpen, libraryPath))
		return FeedPath::None;
	return hasSelectedEndpoint ? FeedPath::SelectedEndpoint : FeedPath::PanelPreview;
}
}
