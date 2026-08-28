/*
    This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "EditorLogicTestSupport.h"

#include "Editor/helpers/VSTPopupLivePreviewPolicy.h"

void testVSTPopupLivePreviewPolicy()
{
	using VSTPopupLivePreviewPolicy::FeedPath;
	using VSTPopupLivePreviewPolicy::selectFeedPath;
    using VSTPopupLivePreviewPolicy::shouldRun;

    expectFalse(shouldRun(false, true, true, L"C:\\Plugins\\Normal.vst3"),
        QStringLiteral("a disabled live analyzer feed never starts"));
	expectTrue(shouldRun(true, true, false,
		L"C:\\Program Files\\Common Files\\VST3\\Bertom_DenoiserClassic.vst3"),
		QStringLiteral("embedded Denoiser panels retain live preview"));
	expectTrue(shouldRun(true, false, true, L"C:\\Plugins\\FabFilter Pro-Q 4.vst3"),
		QStringLiteral("normal native panels retain live preview"));
	expectTrue(shouldRun(true, false, true, L"C:\\Plugins\\BL-Denoiser.dll"),
		QStringLiteral("the Bertom exception does not suppress unrelated native panels"));
	expectFalse(shouldRun(true, false, true,
		L"C:\\Program Files\\Common Files\\VST3\\Bertom_DenoiserClassic.vst3"),
		QStringLiteral("Bertom Denoiser Classic native panels remain protected"));
	expectFalse(shouldRun(true, false, false, L"C:\\Plugins\\Normal.vst3"),
		QStringLiteral("a closed native panel does not run live preview"));
	expectTrue(selectFeedPath(true, false, false, true, L"C:\\Plugins\\Normal.vst3")
		== FeedPath::PanelPreview,
		QStringLiteral("a normal native panel uses the upstream monitor when no endpoint is selected"));
	expectTrue(selectFeedPath(true, true, false, true, L"C:\\Plugins\\FabFilter Pro-Q 4.vst3")
		== FeedPath::SelectedEndpoint,
		QStringLiteral("a selected microphone or device replaces the upstream monitor"));
	expectTrue(selectFeedPath(true, true, false, true,
		L"C:\\Program Files\\Common Files\\VST3\\Bertom_DenoiserClassic.vst3")
		== FeedPath::None,
		QStringLiteral("Bertom native panels do not start either preview engine"));
}
