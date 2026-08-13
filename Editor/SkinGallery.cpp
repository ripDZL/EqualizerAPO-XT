#include "SkinGallery.h"
#include "diagnostics/ToolbarPixelProbe.h"
#include "widgets/MainToolbarKit.h"
#include "SubwooferRouting/Preset.h"
#include "SubwooferRouting/StateCodec.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <memory>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QEnterEvent>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPixmap>
#include <QPointer>
#include <QRadioButton>
#include <QToolButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <QTreeView>
#include <QUrl>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "Editor/FilterTable.h"
#include "Editor/SkinManager.h"
#include "Editor/guis/CopyFilterGUI.h"
#include "Editor/helpers/GUIHelper.h"
#include "Editor/skins/ISkin.h"
#include "Editor/skins/Skins.h"
#include "Editor/widgets/AddCardRow.h"
#include "Editor/widgets/EqGraphView.h"
#include "Editor/widgets/SegmentedControl.h"
#include "Editor/analysis/AnalysisMetric.h"
#include "Editor/analysis/AnalysisResponse.h"
#include "filters/BiQuad.h"
#include "Editor/skins/shared/SkinFileIcons.h"
#include "Editor/widgets/FilterCardRow.h"
#include "Editor/widgets/CommandRowFrame.h"
#include "Editor/widgets/FilterInsertSeam.h"
#include "Editor/widgets/FilterPickerView.h"
#include "Editor/widgets/SkinComboBox.h"
#include "Editor/widgets/TitleBar.h"
#include "Editor/widgets/UpdateToast.h"
#include "Editor/widgets/subwooferrouting/SubwooferRoutingEditorDialog.h"
#include "Editor/widgets/cards/SubwooferRoutingCardEditor.h"
#include "Editor/widgets/routing/IRoutingRenderer.h"

namespace
{
struct GalleryRow
{
	QString name;
	QString line;
};

// Representative rows: a parametric filter, a shelf filter (three knobs in
// the legacy BiQuad GUI hosted by the card body), a peaking filter at 0 dB
// (the bipolar gain knob at its neutral detent), the preamp card (the
// bare knob + value scrub pair - the row that shows whether a skin seats
// custom widgets directly on its surface), the reference-card rows and an
// empty Copy row (the routing editor's empty state).
//
// The reference rows (Include / Convolution / MultiConvolution / VST) render
// against synthetic target files written next to a synthetic config file
// (buildReferenceFiles), so the resolved cards show their healthy named-entity
// state with a deterministic impulse-response readout. include_missing keeps
// the broken-reference transition (MISSING + Locate) in every
// skin's judged set. The VST library is intentionally unresolvable; the card
// renders its missing/not-loaded state, which doubles as the recovery-entry
// showcase. The two MultiConvolution rows cover the mapping-form card (the
// per-skin routing view over a 4-channel BRIR, both ears mapped) and the
// freshly inserted empty state; the empty one also guards the Insert path,
// where a bare "MultiConvolution:" template must still resolve to the card
// body and not fall back to an empty row. The comment and stage rows judge
// the in-place note editor and the two-lane stage card. The two device rows
// split the card's grammar over the synthetic endpoints (galleryDevices):
// "device" shows an engaged playback switch and an engaged capture well next
// to an idle endpoint (the routed/at-rest contrast every skin styles), while
// "device_all" shows the all-devices master engaged over powered-down
// endpoint chips.
// The inline-State fixture is generated through the core instead of pasting
// JSON, so the gallery always renders exactly what the codec would emit.
QString subwooferRoutingPresetRowLine()
{
	const subroute::PresetCreateResult preset =
		subroute::createBuiltInPreset(subroute::kIssue246FrontRear41PresetId);
	if (!preset.succeeded())
		return QStringLiteral("SubwooferRouting:");
	const subroute::StateEncodeResult encoded =
		subroute::encodeStateCanonical(*preset.state);
	if (!encoded.succeeded())
		return QStringLiteral("SubwooferRouting:");
	return QStringLiteral("SubwooferRouting: State ")
		+ QString::fromUtf8(encoded.text->data(),
			static_cast<int>(encoded.text->size()));
}

void appendVstBusRows(QList<GalleryRow>& rows)
{
	const QString quotedLibrary = QStringLiteral("VSTPlugin: Library \"%1\"");
	const QString vst3 = qEnvironmentVariable("EAPO_GALLERY_VST3_PLUGIN");
	if (!vst3.isEmpty() && QFileInfo::exists(vst3))
	{
		rows.append({QStringLiteral("vst3_bus_accepted"),
			quotedLibrary.arg(vst3) + QStringLiteral(" Input Stereo Output Stereo")});
		rows.append({QStringLiteral("vst3_bus_auto"), quotedLibrary.arg(vst3)});
		rows.append({QStringLiteral("vst3_bus_rejected"),
			quotedLibrary.arg(vst3) + QStringLiteral(" Input Stereo Output 7.1")});
	}
	const QString upmixer = qEnvironmentVariable("EAPO_GALLERY_VST3_UPMIXER");
	if (!upmixer.isEmpty() && QFileInfo::exists(upmixer))
		rows.append({QStringLiteral("vst3_bus_upmix"),
			quotedLibrary.arg(upmixer) + QStringLiteral(" Input Stereo Output 7.1")});
	const QString vst2 = qEnvironmentVariable("EAPO_GALLERY_VST2_PLUGIN");
	if (!vst2.isEmpty() && QFileInfo::exists(vst2))
		rows.append({QStringLiteral("vst2_bus_ignored"),
			quotedLibrary.arg(vst2) + QStringLiteral(" Input Stereo Output 7.1")});
}

QList<GalleryRow> galleryRows()
{
	QList<GalleryRow> rows = {
		{ QStringLiteral("filter"), QStringLiteral("Filter 1: ON PK Fc 1000 Hz Gain 6 dB Q 0.71") },
		{ QStringLiteral("shelf"), QStringLiteral("Filter 2: ON HSC Fc 8000 Hz Gain -2.5 dB Q 0.71") },
		{ QStringLiteral("gain0db"), QStringLiteral("Filter 3: ON PK Fc 1000 Hz Gain 0 dB Q 1") },
		{ QStringLiteral("preamp"), QStringLiteral("Preamp: -6 dB") },
		{ QStringLiteral("include"), QStringLiteral("Include: example.txt") },
		{ QStringLiteral("include_nested"), QStringLiteral("Include: Surround\\example.txt") },
		{ QStringLiteral("include_missing"), QStringLiteral("Include: missing.txt") },
		{ QStringLiteral("vst"), QStringLiteral("VSTPlugin: Library example.dll") },
		{ QStringLiteral("device"), QStringLiteral("Device: Speakers Example Audio; Microphone Example Audio") },
		{ QStringLiteral("device_all"), QStringLiteral("Device: all") },
		{ QStringLiteral("channel"), QStringLiteral("Channel: L R") },
		{ QStringLiteral("comment"), QStringLiteral("# Living room preset - tuned by ear") },
		{ QStringLiteral("stage"), QStringLiteral("Stage: pre-mix post-mix") },
		{ QStringLiteral("copy_empty"), QStringLiteral("Copy:") },
		{ QStringLiteral("copy"), QStringLiteral("Copy: VC=0.5*L+0.5*R R=L") },
		{ QStringLiteral("convolution"), QStringLiteral("Convolution: example.wav") },
		{ QStringLiteral("multiconvolution"), QStringLiteral("MultiConvolution: L=0+1 R=2+3 brir.wav") },
		{ QStringLiteral("multiconvolution_empty"), QStringLiteral("MultiConvolution:") },
		// The clean-install first impression: the graphic EQ card is the first
		// thing a fresh install shows, and the two raw-text shapes (a bare
		// note line and a programmatic If command) are the rows that once
		// rendered as nothing at all.
		{ QStringLiteral("graphiceq"), QStringLiteral("GraphicEQ: 25 -4.5; 100 -2; 1000 0; 8000 3.5; 16000 1") },
		{ QStringLiteral("text"), QStringLiteral("plain note line without a command") },
		{ QStringLiteral("iftext"), QStringLiteral("If: inputChannelCount == 2") },
		// The custom-coefficient escape hatch: the IIR card states the order
		// and both coefficient rows (a 2nd-order Butterworth low-pass at fs/4,
		// a0 = 1). Appended last so the row numbers of every earlier scene
		// stay stable against the shot baseline.
		{ QStringLiteral("iir"), QStringLiteral("Filter: ON IIR Order 2 Coefficients 0.2929 0.5858 0.2929 1.0 -0.0 0.1716") },
		// A dynamic line (inline `expression` gain): the Preamp card opens in
		// dynamic mode - powered-down knob, the expression as a token in the
		// value position - instead of parsing the text as 0.0 and destroying
		// the expression on the first knob turn. Appended last (mid-list
		// insertion renumbers every following scene).
		{ QStringLiteral("dynpreamp"), QStringLiteral("Preamp: `bass + 3` dB") },
		// The all-pass card. It has no gain and its magnitude is flat, so the
		// card has to say in words what the filter does; these shots are how
		// that reads in each skin. Written as a bandwidth on purpose - the
		// spelling the editors used to lose - and appended last, because
		// inserting mid-list renumbers every following scene against the
		// stored baseline.
		{ QStringLiteral("allpass"), QStringLiteral("Filter 4: ON AP Fc 900 Hz BW Oct 1") },
		// A notch: the other gain-less biquad. The legacy row hides the gain
		// block for every type that has no gain and lets the remaining blocks
		// stretch into the space, so this row and the all-pass above it look
		// the same - which is the evidence that the all-pass row's spacing is
		// the .ui's own behaviour and not something this campaign introduced.
		{ QStringLiteral("notch"), QStringLiteral("Filter 5: ON NO Fc 800 Hz") },
		// Independent built-in phase and sparse-FIR cards. Velvet appears in
		// both time modes, plus a malformed line so the in-card repair state is
		// judged rather than dropping to raw text.
		{ QStringLiteral("hilbert"), QStringLiteral("Hilbert: Shift=SL,SR Align=L,R Direction=-90") },
		{ QStringLiteral("velvet_dynamic"), QStringLiteral("Velvet: Mode=Dynamic Amount=85% Length=27.5625ms Density=1088.435/s Evolution=5s Transition=250ms Decay=-60dB Variation=2050083136") },
		{ QStringLiteral("velvet_static"), QStringLiteral("Velvet: Mode=Static Amount=100% Length=27.5625ms Density=1088.435/s Evolution=5s Transition=250ms Decay=-60dB Variation=2050083136") },
		{ QStringLiteral("velvet_invalid"), QStringLiteral("Velvet: Mode=Dynamic Length=not-a-time") },
		// The Subwoofer Routing card in its two load-bearing shapes: the built-in
		// #246 preset as an inline State (built through the core so the JSON is
		// always the canonical bytes the engine sees), and a linked profile
		// whose file is missing, which is the warning state every skin must
		// carry without dropping the card. Appended last (mid-list insertion
		// renumbers every following scene against the stored baseline).
		{ QStringLiteral("subwooferrouting"), subwooferRoutingPresetRowLine() },
		{ QStringLiteral("subwooferrouting_missing"), QStringLiteral("SubwooferRouting: Profile \"missing.swxt.json\"") }
	};
	appendVstBusRows(rows);
	return rows;
}

// Synthetic audio endpoints for the Device rows. Offscreen runners have no
// audio devices and the Device card only grows chips for enumerated
// endpoints, so without these the card renders as a lone master chip and the
// per-skin switch grammar is never judged. Three playback endpoints (one
// engaged, one idle, one without the APO - the blank hidden behind the
// reveal toggle) and one engaged capture endpoint cover the state family.
class GalleryAPOInfo : public AbstractAPOInfo
{
public:
	GalleryAPOInfo(const std::wstring& connection, const std::wstring& name, bool input, bool installed,
		unsigned channelCount = 2, unsigned long channelMask = 0x3)
		: connection(connection), name(name), input(input), installed(installed),
		channelCount(channelCount), channelMask(channelMask)
	{
	}

	std::wstring getConnectionName() const override
	{
		return connection;
	}

	std::wstring getDeviceName() const override
	{
		return name;
	}

	std::wstring getDeviceGuid() const override
	{
		return L"";
	}

	// The card pre-selects a chip when the row's pattern matches this string
	// (DeviceCommand::matches) and serializes selections back as these exact
	// strings joined with "; " - keep them plain words so the gallery line
	// round-trips byte-identically.
	std::wstring getDeviceString() const override
	{
		return connection + L" " + name;
	}

	unsigned getChannelCount() const override
	{
		return channelCount;
	}

	unsigned getSampleRate() const override
	{
		return 48000;
	}

	unsigned long getChannelMask() const override
	{
		return channelMask;
	}

	bool isInput() const override
	{
		return input;
	}

	bool isInstalled() const override
	{
		return installed;
	}

	bool canBeUpgraded() const override
	{
		return false;
	}

	bool hasChanges() const override
	{
		return false;
	}

	bool isExperimental() const override
	{
		return false;
	}

	bool isEnhancementsDisabled() const override
	{
		return false;
	}

	bool isDefaultDevice() const override
	{
		return false;
	}

	bool isDisabled() const override
	{
		return false;
	}

	bool isUnplugged() const override
	{
		return false;
	}

	void install() override
	{
	}

	void uninstall() override
	{
	}

	void reinstall() override
	{
	}

private:
	std::wstring connection;
	std::wstring name;
	bool input;
	bool installed;
	unsigned channelCount;
	unsigned long channelMask;
};

void galleryDevices(QList<std::shared_ptr<AbstractAPOInfo>>& outputs, QList<std::shared_ptr<AbstractAPOInfo>>& inputs)
{
	outputs.append(std::make_shared<GalleryAPOInfo>(L"Speakers", L"Example Audio", false, true));
	outputs.append(std::make_shared<GalleryAPOInfo>(L"Headphones", L"Example Audio", false, true));
	outputs.append(std::make_shared<GalleryAPOInfo>(L"Digital Output", L"Example Audio", false, false));
	inputs.append(std::make_shared<GalleryAPOInfo>(L"Microphone", L"Example Audio", true, true));
}

// A canonical 16-bit PCM WAV of silence: enough for libsndfile to report the
// deterministic length / rate / channel readout the convolution cards print.
bool writeWavFile(const QString& path, quint16 channels, quint32 sampleRate, quint32 frames)
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly))
		return false;
	QDataStream out(&file);
	out.setByteOrder(QDataStream::LittleEndian);
	const quint16 bitsPerSample = 16;
	const quint16 blockAlign = channels * bitsPerSample / 8;
	const quint32 dataSize = frames * blockAlign;
	out.writeRawData("RIFF", 4);
	out << quint32(36 + dataSize);
	out.writeRawData("WAVE", 4);
	out.writeRawData("fmt ", 4);
	out << quint32(16) << quint16(1) << channels << sampleRate
		<< quint32(sampleRate * blockAlign) << blockAlign << bitsPerSample;
	out.writeRawData("data", 4);
	out << dataSize;
	const QByteArray silence(dataSize, '\0');
	return out.writeRawData(silence.constData(), silence.size()) == silence.size();
}

// Synthetic reference targets for the gallery rows, next to a synthetic
// config file so relative references resolve: example.txt (Include),
// example.wav (Convolution, 100 ms mono) and brir.wav Ûž:êÚ$z{-®éÜj×—° –6öç7B&ööÂF&²ÒF&´–æFW‚ÓÒ° –6öç7B7G&–æræÖRÒ7G&–ætÆ—FW&Â‚"SòS""’æ&r‡6¶–âÓæ–B‚’À –F&²ò7G&–ætÆ—FW&Â‚&F&²"’¢7G&–ætÆ—FW&Â‚&Æ–v‡B"’“°  ’òòg&W6‚&÷w2VæFW"F†—26¶–âÂ–âF†RÆ—fR7v—F6‚÷&FW"‡FV"F÷và ’òò&Vf÷&RF†R7G–ÆW6†VWB7vÂ&V'V–ÆBgFW"’à —F&ÆRÓæ6ÆV%&÷w2‚“° •6¶–äÖævW#£¦–ç7Fæ6R‚’ÓæÇ•6¶–â‡6¶–âÓæ–B‚’ÂF&²“° —F&ÆRÓçWFFTwV—2‚“° •Æ–6F–öã£§&ö6W74WfVçG2‚“° •6÷&TÆ–6F–öã£§6VæE÷7FVDWfVçG2†çVÆÇG"ÂWfVçC£¤FVfW'&VDFVÆWFR“° •Æ–6F–öã£§&ö6W74WfVçG2‚“°  –6öç7B–çB&÷t6÷VçBÒ–çB‡F&ÆRÓæFö7VÖVçD—FV×2‚’æ6÷VçB‚’“° ––b‡&÷t6÷VçBÒÆ–æW2ç6—¦R‚’ —° —v&æ–ær‚$6&DÖ÷fUFW7C¢W2W‡V7FVBVÆÆB&÷w2Âf÷VæBVB"À —&–çF&ÆR†æÖR’Â7FF–5ö67CÆÆöærÆöæsâ†Æ–æW2ç6—¦R‚’’Â&÷t6÷VçB“° –f–ÇW&W2²³° –6öçF–çVS° —Ð  ’òòöæR6&BÖ÷fVBF÷vâöæR&÷rÂF†Vâ&6²W¢GvòF–ÖVB6öÖÖ—G2ö` ’òòF†RG&rÖÖ÷fRF‚W"66VæRÂæBF†RFö7VÖVçBÆVfW2F†R66VæP ’òò–â—G2÷&–v–æÂ÷&FW"à –6öç7B–çBg&öÒÒ&÷t6÷VçBò#° –f÷"†–çB72Ò²72Â#²72²² —° –6öç7BÆ—7CÅ7G&–æsâ&Vf÷&RÒF&ÆRÓævWDÆ–æW2‚“° –6öç7B–çB6÷W&6U&÷rÒ72ÓÒòg&öÒ¢g&öÒ²° –6öç7B–çBF&vWE&÷rÒ72ÓÒòg&öÒ²¢g&öÓ° –6öç7B–çBG&÷&÷rÒ72ÓÒòg&öÒ²"¢g&öÓ° ”f–ÇFW%F&ÆS£¤—FVÒ¢Ö÷fVBÒF&ÆRÓæFö7VÖVçD—FV×2‚’æB‡6÷W&6U&÷r“°  •VÆ6VEF–ÖW"F–ÖW#° —F–ÖW"ç7F'B‚“° —F&ÆRÓæÖ÷fU&÷w2‡²Ö÷fVBÒÂG&÷&÷r“° •Æ–6F–öã£§&ö6W74WfVçG2‚“° –6öç7B–çCcBVÆ6VBÒF–ÖW"æVÆ6VB‚“° –Ö÷fW2²³°  •Æ—7CÅ7G&–æsâW‡V7FVBÒ&Vf÷&S° –W‡V7FVBæÖ÷fR‡6÷W&6U&÷rÂF&vWE&÷r“° ––b‡F&ÆRÓævWDÆ–æW2‚’ÒW‡V7FVB —° —v&æ–ær‚$6&DÖ÷fUFW7C¢W2Ö÷fRVB&öGV6VBw&öærFö7VÖVçB÷&FW""À —&–çF&ÆR†æÖR’Â72²“° –f–ÇW&W2²³° —Ð ’òòF†RÖ÷fVBÆ–æR×W7B7F’F†R†öæÇ’’6VÆV7F–öâÂB—G2æWp ’òò&÷rÒ'’÷6—F–öâÂæ÷Bö–çFW"Â6ò&÷F‚F†R6÷’×7Æ–6Ræ@ ’òòF†R—FVÒ×&W6W'f–ær–×ÆVÖVçFF–öç2öbF†RÖ÷fR72à –6öç7B6WCÄf–ÇFW%F&ÆS£¤—FVÒ£âb6VÆV7FVD—FV×2ÒF&ÆRÓævWE6VÆV7FVD—FV×2‚“° ––b‡6VÆV7FVD—FV×2ç6—¦R‚’Ò —ÇÂF&ÆRÓæFö7VÖVçD—FV×2‚’æ–æFW„öb‚§6VÆV7FVD—FV×2æ6&Vv–â‚’’ÒF&vWE&÷r —° —v&æ–ær‚$6&DÖ÷fUFW7C¢W2Ö÷fRVBF–Bæ÷BÆVfRF†RÖ÷fVB&÷r6VÆV7FVB"À —&–çF&ÆR†æÖR’Â72²“° –f–ÇW&W2²³° —Ð –6öç7B–çB&÷uv–FvWG2Ò–çB‡F&ÆRÓæf–æD6†–ÆG&VãÄf–ÇFW$6&E&÷r£â€ •7G&–ær‚’ÂC£¤f–æDF—&V7D6†–ÆG&VäöæÇ’’æ6÷VçB‚’“° ––b‡&÷uv–FvWG2Ò&÷t6÷VçB —° —v&æ–ær‚$6&DÖ÷fUFW7C¢W2Ö÷fRVBÆVgBVB&÷rv–FvWG2f÷"VB&÷w2"À —&–çF&ÆR†æÖR’Â72²Â&÷uv–FvWG2Â&÷t6÷VçB“° –f–ÇW&W2²³° —Ð  ––b†VÆ6VBâÆ–Ö—D×2 —° —v&æ–ær‚$6&DÖ÷fUFW7C¢W2Ö÷fRVBFöö²VÆÆB×2†Æ–Ö—BVB×2’"À —&–çF&ÆR†æÖR’Â72²Â7FF–5ö67CÆÆöærÆöæsâ†VÆ6VB’ÂÆ–Ö—D×2“° –f–ÇW&W2²³° —Ð –VÇ6R–b‡v&æ–æt×2âbbVÆ6VBâv&æ–æt×2 —° —v&æ–ær‚$6&DÖ÷fUFW7C¢W2Ö÷fRVBFöö²VÆÆB×2‡v&æ–ærVB×3²†&BÆ–Ö—BVB×2’"À —&–çF&ÆR†æÖR’Â72²Â7FF–5ö67CÆÆöærÆöæsâ†VÆ6VB’Âv&æ–æt×2ÂÆ–Ö—D×2“° —Ð ––b†VÆ6VBâv÷'7D×2 —° —v÷'7D×2ÒVÆ6VC° —v÷'7DæÖRÒæÖS° —Ð —v&æ–ær‚$6&DÖ÷fUFW7C¢W2Ö÷fRVC¢VÆÆB×2‡&÷w2VBÂv–FvWG2VÆÆB’"À —&–çF&ÆR†æÖR’Â72²Â7FF–5ö67CÆÆöærÆöæsâ†VÆ6VB’Â&÷t6÷VçBÀ —7FF–5ö67CÆÆöærÆöæsâ…Æ–6F–öã£¦ÆÅv–FvWG2‚’ç6—¦R‚’’“° —Ð —Ð —Ð  —v&æ–ær‚$6&DÖ÷fUFW7C¢VBÖ÷fW2÷fW"VÆÆB&÷w2Âv÷'7BVÆÆB×2‚W2’Âv&æ–ærVB×2ÂÆ–Ö—BVB×2Âf–ÇW&W2VB"À –Ö÷fW2Â7FF–5ö67CÆÆöærÆöæsâ†Æ–æW2ç6—¦R‚’’Â7FF–5ö67CÆÆöærÆöæsâ‡v÷'7D×2’À —&–çF&ÆR‡v÷'7DæÖR’Âv&æ–æt×2ÂÆ–Ö—D×2Âf–ÇW&W2“°  ’òò6ÖRæò×FV&F÷vâW†—B2'Vâ‚’à –6öç7B–çB7FGW2Òf–ÇW&W2ÓÒò¢° —7FC£¦ffÇW6‚†çVÆÇG"“° —7FC£¥ôW†—B‡7FGW2“°§Ð ¦–çB'Vä6&E6VÆV7F–öåFW7B†6öç7B7G&–ætÆ—7Bb&wVÖVçG2§° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢7F'F–ær"“° •67&VVâ¢67&VVâÒwV”Æ–6F–öã£§&–Ö'•67&VVâ‚“° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢F&vWBÆFf÷&ÓÒW27G–ÆSÒW2G#ÒRã&bÆö6ÆSÒW2"À —&–çF&ÆR…wV”Æ–6F–öã£§ÆFf÷&ÔæÖR‚’’Â&–çF&ÆR‡Óç7G–ÆR‚’Óæö&¦V7DæÖR‚’’À —67&VVâÒçVÆÇG"ò67&VVâÓæFWf–6U—†VÅ&F–ò‚’¢ãÂ&–çF&ÆR…Æö6ÆR‚’ææÖR‚’’“°  –6öç7B–çBfÆt–æFW‚Ò&wVÖVçG2æ–æFW„öb…7G&–ætÆ—FW&Â‚"ÒÖ6&B×6VÆV7F–öâ×FW7B"’“° •F—"÷WGWDF—#° –&ööÂ6GW&U&WVW7FVBÒfÇ6S° ––b†fÆt–æFW‚ãÒbbfÆt–æFW‚²Â&wVÖVçG2ç6—¦R‚ ’bb&wVÖVçG2æB†fÆt–æFW‚²’ç7F'G5v—F‚…7G&–ætÆ—FW&Â‚"ÒÒ"’’ —° –÷WGWDF—"ÒF—"†&wVÖVçG2æB†fÆt–æFW‚²’“° –6GW&U&WVW7FVBÒG'VS° ––b‚÷WGWDF—"æÖ·F‚…7G&–ætÆ—FW&Â‚"â"’’ —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢6ææ÷B7&VFR÷WGWBF—&V7F÷'’W2"À —&–çF&ÆR†÷WGWDF—"æ'6öÇWFUF‚‚’’“° —&WGW&â#° —Ð —Ð  •FV×÷&'”F—"67&F6ƒ° ––b‚67&F6‚æ—5fÆ–B‚’ —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢6ææ÷B7&VFR67&F6‚F—&V7F÷'’"“° —&WGW&â#° —Ð —WFVçb‚$Tõõ4´”åôtÄÄU%’"Â#"“° –6öç7B7G&–ær6öæf–uF‚Ò'V–ÆE&VfW&Væ6Tf–ÆW2…F—"‡67&F6‚çF‚‚’’“° ––b†6öæf–uF‚æ—4V×G’‚’ —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢6ææ÷Bw&—FR&VfW&Væ6RF&vWBf–ÆW2"“° —&WGW&â#° —Ð  –6öç7BWFò6Æ–6²ÒµÒ…v–FvWB¢F&vWBÂ6öç7Bö–çBbÆö6Å÷2’° –6öç7Bö–çBvÆö&Å÷2ÒF&vWBÓæÖFôvÆö&Â†Æö6Å÷2“° •Ö÷W6TWfVçB&W72…WfVçC£¤Ö÷W6T'WGFöå&W72Âö–çDb†Æö6Å÷2’Âö–çDb†vÆö&Å÷2’À •C£¤ÆVgD'WGFöâÂC£¤ÆVgD'WGFöâÂC£¤æôÖöF–f–W"“° •Æ–6F–öã£§6VæDWfVçB‡F&vWBÂg&W72“° •Ö÷W6TWfVçB&VÆV6R…WfVçC£¤Ö÷W6T'WGFöå&VÆV6RÂö–çDb†Æö6Å÷2’Âö–çDb†vÆö&Å÷2’À •C£¤ÆVgD'WGFöâÂC£¤æô'WGFöâÂC£¤æôÖöF–f–W"“° •Æ–6F–öã£§6VæDWfVçB‡F&vWBÂg&VÆV6R“° •Æ–6F–öã£§&ö6W74WfVçG2‚“° —Ó° –6öç7BWFò&W74¶W’ÒµÒ…v–FvWB¢F&vWBÂ–çB¶W’’° •¶W”WfVçB&W72…WfVçC£¤¶W•&W72Â¶W’ÂC£¤æôÖöF–f–W"“° •Æ–6F–öã£§6VæDWfVçB‡F&vWBÂg&W72“° •¶W”WfVçB&VÆV6R…WfVçC£¤¶W•&VÆV6RÂ¶W’ÂC£¤æôÖöF–f–W"“° •Æ–6F–öã£§6VæDWfVçB‡F&vWBÂg&VÆV6R“° •Æ–6F–öã£§&ö6W74WfVçG2‚“° —Ó°  ––çBf–ÇW&W2Ò° ––çB66VæW2Ò° –f÷"„•6¶–â¢6¶–â¢6¶–ç3£¦ÆÂ‚’ —° –f÷"†–çBF&´–æFW‚Ò²F&´–æFW‚Â#²F&´–æFW‚²² —° –6öç7B&ööÂF&²ÒF&´–æFW‚ÓÒ° –6öç7B7G&–ærÖöFRÒF&²ò7G&–ætÆ—FW&Â‚&F&²"’¢7G&–ætÆ—FW&Â‚&Æ–v‡B"“° –6öç7B7G&–ær66VæRÒ7G&–ætÆ—FW&Â‚"SòS""’æ&r‡6¶–âÓæ–B‚’ÂÖöFR“° •6¶–äÖævW#£¦–ç7Fæ6R‚’ÓæÇ•6¶–â‡6¶–âÓæ–B‚’ÂF&²“°  •67&öÆÄ&V67&öÆÄ&V° —67&öÆÄ&Vç&W6—¦Rƒ“cÂs#“° •Æ—7CÄf–ÇFW$6&E&÷r£â&÷w2Ò'V–ÆE&÷w2‡67&öÆÄ&VÂ6öæf–uF‚Â° •7G&–ætÆ—FW&Â‚%&V×¢ÓD""’À •7G&–ætÆ—FW&Â‚%&V×¢Ó"D""’À •7G&–ætÆ—FW&Â‚%&V×¢Ó2D"" —Ò“° ”f–ÇFW%F&ÆR¢F&ÆRÒö&¦V7Eö67CÄf–ÇFW%F&ÆR£â‡67&öÆÄ&Vçv–FvWB‚’“° ––b‡F&ÆRÓÒçVÆÇG"ÇÂ&÷w2ç6—¦R‚’Ò2 —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢W2F&ÆR6öç7G'V7F–öâf–ÆVB"Â&–çF&ÆR‡66VæR’“° –f–ÇW&W2²³° –6öçF–çVS° —Ð  –6öç7BWFòf—7VÅ7FFRÒ²g&÷w5Ò†6öç7B6†"¢&÷W'G’Â–çBW‡V7FVD–æFW‚’° ––çB6÷VçBÒ° –&ööÂW‡V7FVBÒfÇ6S° –f÷"†–çB’Ò²’Â&÷w2ç6—¦R‚“²’²² —° ”6öÖÖæE&÷tg&ÖR¢g&ÖRÒ&÷w5¶•ÒÓæf–æD6†–ÆCÄ6öÖÖæE&÷tg&ÖR£â€ •7G&–ær‚’ÂC£¤f–æDF—&V7D6†–ÆG&VäöæÇ’“° –6öç7B&ööÂ7F—fRÒg&ÖRÒçVÆÇG"bbg&ÖRÓç&÷W'G’‡&÷W'G’’çFô&ööÂ‚“° ––b†7F—fR –6÷VçB²³° ––b†’ÓÒW‡V7FVD–æFW‚ –W‡V7FVBÒ7F—fS° —Ð —&WGW&âÖ¶U—"†6÷VçBÂW‡V7FVB“° —Ó°  ’òòG&—fRF†RW†7Bf–ÇFW%F&ÆRö–çFW"F‚ÂF†Vâ6ö×&RF†RÖöFVÀ ’òòç7vW"v—F‚F†RG–æÖ–2&÷W'F–W2F†R6¶–â7GVÆÇ’&VæFW'2à –6öç7Bö–çB6V6öæD†VFW"Ò&÷w5³ÒÓæÖFò‡F&ÆRÂ&÷w5³ÒÓævWD†VFW%&V7B‚’æ6VçFW"‚’“° –6Æ–6²‡F&ÆRÂ6V6öæD†VFW"“° –6öç7B&ööÂ†VFW$ÖöFVÂÒF&ÆRÓævWE6VÆV7FVD—FV×2‚’ç6—¦R‚’ÓÒ ’bbF&ÆRÓævWE6VÆV7FVD—FV×2‚’æ6öçF–ç2‡F&ÆRÓæFö7VÖVçD—FV×2‚’æBƒ’ ’bbF&ÆRÓævWDfö7W6VD—FVÒ‚’ÓÒF&ÆRÓæFö7VÖVçD—FV×2‚’æBƒ“° –6öç7B—#Æ–çBÂ&ööÃâ†VFW%6VÆV7FVBÒf—7VÅ7FFR‚'6VÆV7FVB"Â“° –6öç7B—#Æ–çBÂ&ööÃâ†VFW$fö7W6VBÒf—7VÅ7FFR‚&fö7W6VB"Â“° –6öç7B&ööÂ†VFW%f—7VÂÒ†VFW%6VÆV7FVBæf—'7BÓÒbb†VFW%6VÆV7FVBç6V6öæ@ ’bb†VFW$fö7W6VBæf—'7BÓÒbb†VFW$fö7W6VBç6V6öæC° ––b‚†VFW$ÖöFVÂÇÂ†VFW%f—7VÂ —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢W2†VFW"6Æ–6²ÖöFVÃÒVBf—7VÃÒVB6VÆV7FVDg&ÖW3ÒVBfö7W6VDg&ÖW3ÒVB"À —&–çF&ÆR‡66VæR’Â†VFW$ÖöFVÂò¢Â†VFW%f—7VÂò¢À –†VFW%6VÆV7FVBæf—'7BÂ†VFW$fö7W6VBæf—'7B“° –f–ÇW&W2²³° —Ð  ’òò¶W–&ö&Bæf–vF–öâ—2F†R6Æ÷6W7B&Vw&W76–öâFƒ¢—BW6W2F†P ’òò6ÖR7FFR7–æ6‡&öæ—¦W"'WB×W7B&WF–âF†RÆ—7Bw2W†—7F–ær'&÷p ’òò&V†f–÷"gFW"ö–çFW"6Æ–6·2&V6öÖR6&BÖv&Rà —&W74¶W’‡F&ÆRÂC£¤¶W•õW“° –6öç7B&ööÂ¶W–&ö&DÖöFVÂÒF&ÆRÓævWE6VÆV7FVD—FV×2‚’ç6—¦R‚’ÓÒ ’bbF&ÆRÓævWE6VÆV7FVD—FV×2‚’æ6öçF–ç2‡F&ÆRÓæFö7VÖVçD—FV×2‚’æBƒ’ ’bbF&ÆRÓævWDfö7W6VD—FVÒ‚’ÓÒF&ÆRÓæFö7VÖVçD—FV×2‚’æBƒ“° –6öç7B—#Æ–çBÂ&ööÃâ¶W–&ö&E6VÆV7FVBÒf—7VÅ7FFR‚'6VÆV7FVB"Â“° –6öç7B—#Æ–çBÂ&ööÃâ¶W–&ö&Dfö7W6VBÒf—7VÅ7FFR‚&fö7W6VB"Â“° –6öç7B&ööÂ¶W–&ö&Ef—7VÂÒ¶W–&ö&E6VÆV7FVBæf—'7BÓÒbb¶W–&ö&E6VÆV7FVBç6V6öæ@ ’bb¶W–&ö&Dfö7W6VBæf—'7BÓÒbb¶W–&ö&Dfö7W6VBç6V6öæC° ––b‚¶W–&ö&DÖöFVÂÇÂ¶W–&ö&Ef—7VÂ —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢W2¶W–&ö&BÖ÷fRÖöFVÃÒVBf—7VÃÒVB6VÆV7FVDg&ÖW3ÒVBfö7W6VDg&ÖW3ÒVB"À —&–çF&ÆR‡66VæR’Â¶W–&ö&DÖöFVÂò¢Â¶W–&ö&Ef—7VÂò¢À –¶W–&ö&E6VÆV7FVBæf—'7BÂ¶W–&ö&Dfö7W6VBæf—'7B“° –f–ÇW&W2²³° —Ð  ’òòÆ–æTVF—B6öç7VÖW2—G2÷vâ&W72âÆ–â6Æ–6²F†W&R×W7B7F–ÆÀ ’òòfö7W2÷6VÆV7B—G2÷væ–ær6&BæB6öÆÆ6RâöÆB×VÇF’×6VÆV7F–öâà —&÷w5³%ÒÓæVF—EFW‡B‚“° •Æ–6F–öã£§&ö6W74WfVçG2‚“° —F&ÆRÓç6VÆV7DÆÂ‚“° •Æ–6F–öã£§&ö6W74WfVçG2‚“° •Æ–æTVF—B¢&tVF—F÷"Ò&÷w5³%ÒÓæf–æD6†–ÆCÅÆ–æTVF—B£â€ •7G&–ætÆ—FW&Â‚$f–ÇFW$6&E&tVF—F÷""’“° ––b‡&tVF—F÷"ÓÒçVÆÇG"ÇÂ&tVF—F÷"Óæ—5f—6–&ÆR‚’ —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢W2&rVF—F÷"—2Væf–Æ&ÆR"Â&–çF&ÆR‡66VæR’“° –f–ÇW&W2²³° —Ð –VÇ6P —° –6Æ–6²‡&tVF—F÷"Â&tVF—F÷"Óç&V7B‚’æ6VçFW"‚’“° –6öç7B&ööÂVF—F÷$ÖöFVÂÒF&ÆRÓævWE6VÆV7FVD—FV×2‚’ç6—¦R‚’ÓÒ ’bbF&ÆRÓævWE6VÆV7FVD—FV×2‚’æ6öçF–ç2‡F&ÆRÓæFö7VÖVçD—FV×2‚’æBƒ"’ ’bbF&ÆRÓævWDfö7W6VD—FVÒ‚’ÓÒF&ÆRÓæFö7VÖVçD—FV×2‚’æBƒ"“° –6öç7B—#Æ–çBÂ&ööÃâVF—F÷%6VÆV7FVBÒf—7VÅ7FFR‚'6VÆV7FVB"Â"“° –6öç7B—#Æ–çBÂ&ööÃâVF—F÷$fö7W6VBÒf—7VÅ7FFR‚&fö7W6VB"Â"“° –6öç7B&ööÂVF—F÷%f—7VÂÒVF—F÷%6VÆV7FVBæf—'7BÓÒbbVF—F÷%6VÆV7FVBç6V6öæ@ ’bbVF—F÷$fö7W6VBæf—'7BÓÒbbVF—F÷$fö7W6VBç6V6öæC° ––b‚VF—F÷$ÖöFVÂÇÂVF—F÷%f—7VÂ —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢W2VF—F÷"6Æ–6²ÖöFVÃÒVBf—7VÃÒVB6VÆV7FVDg&ÖW3ÒVBfö7W6VDg&ÖW3ÒVB"À —&–çF&ÆR‡66VæR’ÂVF—F÷$ÖöFVÂò¢ÂVF—F÷%f—7VÂò¢À –VF—F÷%6VÆV7FVBæf—'7BÂVF—F÷$fö7W6VBæf—'7B“° –f–ÇW&W2²³° —Ð —Ð  ––b†6GW&U&WVW7FVB —° –6öç7B7G&–ær6†÷BÒ÷WGWDF—"æf–ÆUF‚…7G&–ætÆ—FW&Â‚"SòS%ö6&B×6VÆV7F–öâçær" ’æ&r‡6¶–âÓæ–B‚’ÂÖöFR’“° ––b‚F&ÆRÓæw&"‚’ç6fR‡6†÷BÂ%är"’ —° —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢W26÷VÆBæ÷Bw&—FRW2"À —&–çF&ÆR‡66VæR’Â&–çF&ÆR‡6†÷B’“° –f–ÇW&W2²³° —Ð —Ð  —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢W26ö×ÆWFR"Â&–çF&ÆR‡66VæR’“° —66VæW2²³° —Ð —Ð  —v&æ–ær‚$6&E6VÆV7F–öåFW7C¢VB66VæW2Âf–ÇW&W2VB"Â66VæW2Âf–ÇW&W2“° –6öç7B–çB7FGW2Òf–ÇW&W2ÓÒò¢° —7FC£¦ffÇW6‚†çVÆÇG"“° —7FC£¥ôW†—B‡7FGW2“°§Ð ¦–çB'Vâ†6öç7B7G&–ætÆ—7Bb&wVÖVçG2§° –6öç7B–çBfÆt–æFW‚Ò&wVÖVçG2æ–æFW„öb…7G&–ætÆ—FW&Â‚"Ò×6¶–âÖvÆÆW'’"’“° ––b†fÆt–æFW‚ÂÇÂfÆt–æFW‚²ãÒ&wVÖVçG2ç6—¦R‚’ —° —v&æ–ær‚%W6vS¢VF—F÷"Ò×6¶–âÖvÆÆW'’Æ÷WDF—#â²Ò×6¶–âÖvÆÆW'’×6¶–ç2–BÆ–BÂââåÒ"“° —&WGW&â#° —Ð  •F—"÷WDF—"†&wVÖVçG2æB†fÆt–æFW‚²’“° ––b‚÷WDF—"æÖ·F‚…7G&–ætÆ—FW&Â‚"â"’’ —° —v&æ–ær‚%6¶–ävÆÆW'“¢6ææ÷B7&VFR÷WGWBF—&V7F÷'’W2"Â&–çF&ÆR†÷WDF—"æ'6öÇWFUF‚‚’’“° —&WGW&â#° —Ð  •7G&–ætÆ—7B6¶–ä–G3° –6öç7B–çB6¶–ç4–æFW‚Ò&wVÖVçG2æ–æFW„öb…7G&–ætÆ—FW&Â‚"Ò×6¶–âÖvÆÆW'’×6¶–ç2"’“° ––b‡6¶–ç4–æFW‚ãÒbb6¶–ç4–æFW‚²Â&wVÖVçG2ç6—¦R‚’ —° —6¶–ä–G2Ò&wVÖVçG2æB‡6¶–ç4–æFW‚²’ç7Æ—B…ÆF–ã6†"‚rÂr’ÂC£¥6¶—V×G•'G2“° —Ð –VÇ6P —° –f÷"„•6¶–â¢6¶–â¢6¶–ç3£¦ÆÂ‚’ —6¶–ä–G2æVæB‡6¶–âÓæ–B‚’“° —Ð  ’òòF†R&VfW&Væ6R6&G2&ö&RF&vWBf–ÆW3²F†RvÆÆW'’&÷f–FW27–çF†WF–0 ’òòöæW2æBÖ&·2—G6VÆb6òF†R6&G26¶—F†RVF–ò×6W'f–6R4Â&ö&RÀ ’òòv†–6‚†2æòÖVæ–ævgVÂç7vW"f÷"g&W6†Ç’w&—GFVâ67&F6‚f–ÆW2à —WFVçb‚$Tõõ4´”åôtÄÄU%’"Â#"“° –6öç7B7G&–ær6öæf–uF‚Ò'V–ÆE&VfW&Væ6Tf–ÆW2†÷WDF—"“° ––b†6öæf–uF‚æ—4V×G’‚’ —° —v&æ–ær‚%6¶–ävÆÆW'“¢6ææ÷Bw&—FR&VfW&Væ6RF&vWBf–ÆW2VæFW"W2"Â&–çF&ÆR†÷WDF—"æ'6öÇWFUF‚‚’’“° —&WGW&â#° —Ð  ––çBf–ÇW&W2Ò° ––b‡Vçf—&öæÖVçEf&–&ÆT—56WB‚$TõôtÄÄU%•ôÄTt5’"’ —° –f–ÇW&W2³Ò&VæFW$†W&—FvR†÷WDF—"Â6öæf–uF‚Â6¶–ä–G2“° –6öç7B–çB7FGW2Òf–ÇW&W2ÓÒò¢° —7FC£¦ffÇW6‚†çVÆÇG"“° —7FC£¥ôW†—B‡7FGW2“° —Ð –f÷"†6öç7B7G&–ærb6¶–ä–B¢6¶–ä–G2 —° –f–ÇW&W2³Ò&VæFW%6¶–â†÷WDF—"Â6¶–ä–BçG&–ÖÖVB‚’Â6öæf–uF‚ÂG'VR“° –f–ÇW&W2³Ò&VæFW%6¶–â†÷WDF—"Â6¶–ä–BçG&–ÖÖVB‚’Â6öæf–uF‚ÂfÇ6R“° —Ð  ’òò6VÆbÖ6†V6²F†R6†÷B6÷VçB6ò6–ÆVçFÇ’G&÷VB6¶–âÂ&÷r÷"7FFRf–Ç0 ’òòF†R'VâWfVâv†VâWfW'’GFV×FVBw&"7V66VVFVBâvÆÆW'•&÷w2‚’G&—fW2F†P ’òò&÷rFW&ÒÂ6òFF–ærvÆÆW'’&÷rWFFW2F†—2W‡V7FF–öâWFöÖF–6ÆÇ ’òòæBæòW‡FW&æÂ†'V–ÆBç–ÖÂ’6÷VçBæVVG2Fò&RF÷V6†VBà –6öç7B–çBW‡G&6†÷G2Òf—†VE66Væ&–õ6†÷D6÷VçB‚“° –6öç7B–çBW%6¶–äÖöFRÒ7FF–5ö67CÆ–çCâ†vÆÆW'•&÷w2‚’ç6—¦R‚’’¢µ7FFW5W%&÷r²W‡G&6†÷G3° –6öç7B–çBW‡V7FVBÒ7FF–5ö67CÆ–çCâ‡6¶–ä–G2ç6—¦R‚’’¢"¢W%6¶–äÖöFS° –6öç7B–çB7GVÂÒ7FF–5ö67CÆ–çCâ†÷WDF—"æVçG'”Æ—7B…7G&–ætÆ—7Gµ7G&–ætÆ—FW&Â‚"¢çær"—ÒÂF—#£¤f–ÆW2’ç6—¦R‚’“° ––b†7GVÂÒW‡V7FVB —° —v&æ–ær‚%6¶–ävÆÆW'“¢W‡V7FVBVB6†÷G2‚VB6¶–ç2‚"ÖöFW2‚‚VB&÷w2‚VB²VBW‡G&2’’Âw&÷FRVB"À –W‡V7FVBÂ7FF–5ö67CÆ–çCâ‡6¶–ä–G2ç6—¦R‚’’Â7FF–5ö67CÆ–çCâ†vÆÆW'•&÷w2‚’ç6—¦R‚’’Âµ7FFW5W%&÷rÂW‡G&6†÷G2Â7GVÂ“° –f–ÇW&W2²³° —Ð –f÷"†6öç7B7G&–ærb6¶–ä–B¢6¶–ä–G2 —° –f÷"†6öç7B7G&–ærbÖöFR¢²7G&–ætÆ—FW&Â‚&F&²"’Â7G&–ætÆ—FW&Â‚&Æ–v‡B"’Ò —° –f÷"†6öç7BvÆÆW'•66Væ&–òb66Væ&–ò¢vÆÆW'•66Væ&–÷2‚’ —° –f÷"†6öç7B7G&–ærb7FFR¢66Væ&–òç7FFW2 —° –6öç7B7G&–ærf–ÆTæÖRÒ7G&–ætÆ—FW&Â‚"SòS%òS5òSBçær" ’æ&r‡6¶–ä–BçG&–ÖÖVB‚’ÂÖöFRÂ66Væ&–òæ–BÂ7FFR“° ––b‚÷WDF—"æW†—7G2†f–ÆTæÖR’ —° —v&æ–ær‚%6¶–ävÆÆW'“¢&Vv—7FW&VB66Væ&–ò÷WGWB—2Ö—76–æs¢W2"À —&–çF&ÆR†f–ÆTæÖR’“° –f–ÇW&W2²³° —Ð —Ð —Ð —Ð —Ð  ’òòÒ×6¶–âÖvÆÆW'’—2†VFÆW72öæR×6†÷C¢'’F†—2ö–çBWfW'’67&VVç6†÷B†0 ’òò&VVâ&VæFW&VBæBfÇW6†VBFòF—6²â&WGW&æ–æræ÷&ÖÆÇ’v÷VÆBVçv–æB–çFòF†P ’òòÆ–6F–öâòvÆö&ÂFV&F÷vâÂv†–6‚öâF†Röfg67&VVâÆFf÷&ÒæWfW  ’òòf–æ—6†W2ÒÆVgF÷fW"&6¶w&÷VæB&W6÷W&6R¶VW2F†R&ö6W72Æ—fRÂ6òF†P ’òò&VæFW'2ÆÂ7V66VVB'WBF†R&ö6W72†æw2öâW†—BæBç’G&—f–ær67&—@ ’òò†2FòF–ÖR÷WBæB¶–ÆÂ—Bâæ÷F†–ær—2ÆVgBFòW'6—7BÂ6òfÇW6‚F†P ’òòF–væ÷7F–27G&VÒæBW†—B–ÖÖVF–FVÇ’v—F‚F†Rf–ÇW&R7FGW2–ç7FVBà –6öç7B–çB7FGW2Òf–ÇW&W2ÓÒò¢° —7FC£¦ffÇW6‚†çVÆÇG"“° —7FC£¥ôW†—B‡7FGW2“°§Ð§Ð