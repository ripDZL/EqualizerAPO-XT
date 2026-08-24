#include "EditorLogicTestSupport.h"

#include <new>
#include <stdexcept>

#include <QDir>
#include <QColor>
#include <QPalette>
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QTemporaryDir>

#include "Benchmark/BatchPlan.h"
#include "Editor/helpers/AnalysisWorkerRecovery.h"
#include "Editor/skins/SkinThemeData.h"
#include "Editor/widgets/EditableValueText.h"
#include "Editor/widgets/cards/FileReferenceController.h"
#include "runtime/memory/AlignedMemory.h"
// The roster is the one list of which skins exist, so what is worth pinning is
// that it is complete and that everything derived from it lands on a real skin.
// Missing a skin used to be silent: resolveId() falls back to Studio, so a skin
// that was registered but forgotten in one of eighteen places was drawn as Studio
// with no error anywhere.
void testTheSkinRosterIsTheOneList()
{
	const QVector<SkinThemeData::SkinEntry>& roster = SkinThemeData::roster();
	const QStringList expectedIds = {
		QStringLiteral("studio"), QStringLiteral("minimal"), QStringLiteral("soft"),
		QStringLiteral("rack"), QStringLiteral("matrix"), QStringLiteral("midnight"),
		QStringLiteral("arctic"), QStringLiteral("ember"), QStringLiteral("violet"),
		QStringLiteral("solar"), QStringLiteral("obsidian"), QStringLiteral("aurora"),
		QStringLiteral("forge"), QStringLiteral("nebula"), QStringLiteral("noir"),
		QStringLiteral("legacy-slate"), QStringLiteral("legacy-blue"),
		QStringLiteral("legacy-forest"), QStringLiteral("legacy-bronze"),
		QStringLiteral("legacy-plum")
	};
	requireTrue(roster.size() == expectedIds.size(),
		"the roster carries exactly the base skins and the three scoped variant sets");
	expectTrue(roster.first().id == QStringLiteral("studio"),
		"studio is first, which is what an unknown id falls back to and what the default is");

	QSet<QString> seen;
	for (const SkinThemeData::SkinEntry& skin : roster)
	{
		expectFalse(skin.id.isEmpty(), "every entry has an id");
		expectFalse(skin.qssBaseName.isEmpty(), "every entry names its style sheet");
		expectFalse(skin.paintBaseId.isEmpty(), "every entry names its paint grammar");
		expectTrue(skin.tokens != nullptr, "every entry carries its token table");
		expectTrue(SkinThemeData::entry(skin.paintBaseId).id == skin.paintBaseId,
			"every paint grammar is a registered skin id");
		expectTrue(SkinThemeData::entry(skin.paintBaseId).paintBaseId == skin.paintBaseId,
			"every paint grammar is a self-rooting base skin");
		expectFalse(seen.contains(skin.id),
			QStringLiteral("%1 appears once").arg(skin.id));
		seen.insert(skin.id);

		expectTrue(SkinThemeData::resolveId(skin.id) == skin.id,
			QStringLiteral("%1 is its own canonical id").arg(skin.id));
		expectTrue(SkinThemeData::entry(skin.id).id == skin.id,
			QStringLiteral("%1 resolves back to its own entry").arg(skin.id));
	}

	const QStringList ids = SkinThemeData::ids();
	requireTrue(ids.size() == roster.size(), "ids() has one entry per roster skin, in the same order");
	expectTrue(ids == expectedIds,
		"ids() preserves the exact scoped roster and its display order");
	expectTrue(SkinThemeData::entry(QStringLiteral("midnight")).qssBaseName == QStringLiteral("studio"),
		"Midnight Console rides the Studio QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("midnight")).paintBaseId == QStringLiteral("studio"),
		"Midnight Console rides the Studio paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("arctic")).qssBaseName == QStringLiteral("soft"),
		"Arctic Bloom rides the Soft QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("arctic")).paintBaseId == QStringLiteral("soft"),
		"Arctic Bloom rides the Soft paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("ember")).qssBaseName == QStringLiteral("rack"),
		"Ember Rack rides the Rack QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("ember")).paintBaseId == QStringLiteral("rack"),
		"Ember Rack rides the Rack paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("violet")).qssBaseName == QStringLiteral("matrix"),
		"Violet Pulse rides the Matrix QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("violet")).paintBaseId == QStringLiteral("matrix"),
		"Violet Pulse rides the Matrix paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("solar")).qssBaseName == QStringLiteral("precision"),
		"Solar Paper rides the Minimal QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("solar")).paintBaseId == QStringLiteral("minimal"),
		"Solar Paper rides the Minimal paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("obsidian")).qssBaseName == QStringLiteral("studio"),
		"Obsidian Glass rides the Studio QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("obsidian")).paintBaseId == QStringLiteral("studio"),
		"Obsidian Glass rides the Studio paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("aurora")).qssBaseName == QStringLiteral("soft"),
		"Aurora Veil rides the Soft QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("aurora")).paintBaseId == QStringLiteral("soft"),
		"Aurora Veil rides the Soft paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("forge")).qssBaseName == QStringLiteral("rack"),
		"Copper Forge rides the Rack QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("forge")).paintBaseId == QStringLiteral("rack"),
		"Copper Forge rides the Rack paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("nebula")).qssBaseName == QStringLiteral("matrix"),
		"Neon Nebula rides the Matrix QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("nebula")).paintBaseId == QStringLiteral("matrix"),
		"Neon Nebula rides the Matrix paint grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("noir")).qssBaseName == QStringLiteral("precision"),
		"Noir Chrome rides the Minimal QSS grammar");
	expectTrue(SkinThemeData::entry(QStringLiteral("noir")).paintBaseId == QStringLiteral("minimal"),
		"Noir Chrome rides the Minimal paint grammar");
	for (const QString& variant : { QStringLiteral("legacy-slate"), QStringLiteral("legacy-blue"),
		QStringLiteral("legacy-forest"), QStringLiteral("legacy-bronze"), QStringLiteral("legacy-plum") })
	{
		expectTrue(SkinThemeData::entry(variant).qssBaseName == QStringLiteral("precision"),
			QStringLiteral("%1 rides the Precision QSS grammar").arg(variant));
		expectTrue(SkinThemeData::entry(variant).paintBaseId == QStringLiteral("minimal"),
			QStringLiteral("%1 rides the Minimal paint grammar").arg(variant));
	}
	expectTrue(SkinThemeData::tokens(QStringLiteral("obsidian"), true).graphRadius == 12,
		"Obsidian Glass keeps its larger graph radius");
	expectTrue(SkinThemeData::tokens(QStringLiteral("aurora"), true).borderRadius == 18,
		"Aurora Veil keeps its softer radius");
	expectTrue(SkinThemeData::tokens(QStringLiteral("forge"), true).graphRadius == 6,
		"Copper Forge keeps its compact graph radius");
	expectTrue(SkinThemeData::tokens(QStringLiteral("nebula"), true).cardRailWidth == 5,
		"Neon Nebula keeps its matrix card rail");
	expectTrue(SkinThemeData::tokens(QStringLiteral("noir"), true).zebraStripe,
		"Noir Chrome keeps its minimal zebra stripes");
	expectTrue(SkinThemeData::tokens(QStringLiteral("legacy-bronze"), true).fontFamily == QStringLiteral("Segoe UI"),
		"Legacy Bronze keeps the native legacy-row font");
	expectTrue(SkinThemeData::tokens(QStringLiteral("legacy-bronze"), true).accent == QStringLiteral("#C58B48"),
		"Legacy Bronze carries the requested bronze accent");

	// The two stored aliases from earlier releases, which are the only ids that
	// cannot be derived from the roster.
	expectTrue(SkinThemeData::resolveId(QStringLiteral("glassy")) == QStringLiteral("studio"),
		"the glassy alias still resolves, because it is what old registries hold");
	expectTrue(SkinThemeData::resolveId(QStringLiteral("industrial")) == QStringLiteral("rack"),
		"and so does industrial");
	expectTrue(SkinThemeData::resolveId(QStringLiteral("no such skin")) == roster.first().id,
		"an unknown id falls back to the first entry rather than producing no skin at all");
}

void testSkinTokensCarryExplicitMode()
{
	const QStringList skinIds = SkinThemeData::ids();
	for (const QString& skinId : skinIds)
	{
		expectTrue(SkinThemeData::tokens(skinId, true).dark,
			QStringLiteral("%1 dark tokens carry dark=true").arg(skinId));
		expectFalse(SkinThemeData::tokens(skinId, false).dark,
			QStringLiteral("%1 light tokens carry dark=false").arg(skinId));
	}
}

// A skin can be stylistically different without sacrificing ordinary text
// readability. These checks cover every built-in token table in both modes,
// including the Legacy Rows palettes that do not load a modern QSS sheet.
void testEveryBuiltInThemePassesTheReadabilityContract()
{
	for (const QString& skinId : SkinThemeData::ids())
	{
		const SkinTokens light = SkinThemeData::tokens(skinId, false);
		const SkinTokens dark = SkinThemeData::tokens(skinId, true);
		expectTrue(SkinThemeData::modesAreDistinct(light, dark),
			QStringLiteral("%1 has visibly different light and dark window grounds").arg(skinId));

		for (const SkinTokens& tokens : { light, dark })
		{
			const QVector<SkinThemeData::ReadabilityCheck> checks =
				SkinThemeData::readabilityChecks(tokens);
			requireTrue(!checks.isEmpty(),
				QStringLiteral("%1 exposes a non-empty readability audit").arg(skinId));
			for (const SkinThemeData::ReadabilityCheck& check : checks)
			{
				expectTrue(check.passes(),
					QStringLiteral("%1 %2 %3 is %4:1 (needs %5:1)")
						.arg(skinId, tokens.dark ? QStringLiteral("dark") : QStringLiteral("light"),
							check.label)
						.arg(check.ratio, 0, 'f', 2)
						.arg(check.minimum, 0, 'f', 2));
			}
			expectTrue(SkinThemeData::passesReadability(tokens),
				QStringLiteral("%1 %2 passes the aggregate readability contract")
					.arg(skinId, tokens.dark ? QStringLiteral("dark") : QStringLiteral("light")));
		}
	}
}

void testTooltipContractFollowsThemeTokens()
{
	const SkinTokens tokens = SkinThemeData::tokens(QStringLiteral("legacy-plum"), true);
	const QString rule = SkinThemeData::tooltipOverride(tokens);
	expectTrue(rule.contains(tokens.card),
		"tooltip override uses the active card colour instead of a platform default");
	expectTrue(rule.contains(tokens.text),
		"tooltip override uses the active text colour instead of a platform default");
	expectTrue(rule.contains(tokens.border),
		"tooltip override carries the active border colour");

	const QPalette palette = SkinThemeData::palette(tokens, true);
	expectTrue(palette.color(QPalette::ToolTipBase) == QColor(tokens.card),
		"tooltip palette base follows the active card token");
	expectTrue(palette.color(QPalette::ToolTipText) == QColor(tokens.text),
		"tooltip palette ink follows the active text token");
	expectTrue(palette.color(QPalette::HighlightedText) == QColor(SkinThemeData::selectionText(tokens)),
		"selected text palette follows the readable token-derived selection ink");
}

void testThemeLabCanRepairCustomTextContrast()
{
	SkinTokens custom = SkinThemeData::tokens(QStringLiteral("studio"), false);
	custom.text = QStringLiteral("#FFFFFF");
	custom.mutedText = QStringLiteral("#FFFFFF");
	expectFalse(SkinThemeData::passesReadability(custom),
		"the fixture starts with white text on a light custom theme");

	SkinThemeData::repairTextReadability(custom);
	expectTrue(SkinThemeData::passesReadability(custom),
		"Theme Lab's text repair restores the full normal-text contrast contract");
}

// The alpha form a sheet needs to hold a token at partial opacity. QSS has no
// variables and its rgba() takes three numbers, so without this a sheet had to
// write the palette value out by hand and a token change stopped reaching it.
void testTokenSubstitutionOffersAnAlphaForm()
{
	const SkinTokens tokens = SkinThemeData::tokens(QStringLiteral("studio"), true);
	const QColor accent(tokens.accent);
	requireTrue(accent.isValid(), "the studio accent token parses as a colour");

	const QString resolved = SkinThemeData::substituteTokens(
		QStringLiteral("QWidget { background: rgba(@ACCENT_RGB@, 0.30); color: @SELECTION_TEXT@; }"), tokens);

	expectTrue(resolved.contains(QStringLiteral("rgba(%1, %2, %3, 0.30)")
			.arg(accent.red()).arg(accent.green()).arg(accent.blue())),
		"the alpha form expands to the token's three channels, so rgba() gets numbers");
	expectTrue(resolved.contains(QStringLiteral("color: ") + SkinThemeData::selectionText(tokens)),
		"and the selected-text form expands to the readable derived ink");
	expectFalse(resolved.contains(QStringLiteral("_RGB")),
		"the longer sentinel is replaced before the shorter one, or the hex value would be left with _RGB stuck to it");

	// A font family is not a colour and has no channels; asking for its alpha form
	// has to leave the sheet alone rather than produce something that would make
	// the whole rule invalid.
	const QString fontSheet = SkinThemeData::substituteTokens(
		QStringLiteral("QWidget { font-family: \"@FONT@\"; }"), tokens);
	expectTrue(fontSheet.contains(tokens.fontFamily), "the font token still resolves");
}

void testEverySkinSheetResolvesAllThemeTokens()
{
	QDir repoRoot(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath());
	requireTrue(repoRoot.cdUp(), "skin-token test reaches the tests directory");
	requireTrue(repoRoot.cdUp(), "skin-token test reaches the repository root");
	const QStringList skinIds = SkinThemeData::ids();
	const QRegularExpression unresolved(QStringLiteral("@[A-Z_0-9]+@"));
	for (const QString& skinId : skinIds)
	{
		for (bool dark : { false, true })
		{
			const QString resource = SkinThemeData::qssResource(skinId, dark);
			const QString sourcePath = repoRoot.filePath(
				QStringLiteral("Editor/skins/%1/qss/%2")
					.arg(SkinThemeData::entry(skinId).paintBaseId, QFileInfo(resource).fileName()));
			QFile file(sourcePath);
			expectTrue(file.open(QIODevice::ReadOnly | QIODevice::Text),
				QStringLiteral("loads %1 source sheet").arg(resource));
			const QString resolved = SkinThemeData::substituteTokens(
				QString::fromUtf8(file.readAll()), SkinThemeData::tokens(skinId, dark));
			expectFalse(unresolved.match(resolved).hasMatch(),
				QStringLiteral("%1 leaves no unresolved @TOKEN@ sentinel").arg(resource));
		}
	}
}

void testEditableValueTextUsesDisplayedDecimalFormatFirst()
{
	const QLocale german(QLocale::German, QLocale::Germany);
	double value = 0.0;
	expectTrue(parseEditableValueText(QStringLiteral("12.345"), german, &value),
		"dot-decimal text parses under a grouping-dot locale");
	expectTrue(qAbs(value - 12.345) < 0.000001,
		"dot-decimal text keeps the displayed C-locale meaning");
	expectTrue(parseEditableValueText(QStringLiteral("12,5"), german, &value),
		"system-locale decimal text remains accepted as a fallback");
	expectTrue(qAbs(value - 12.5) < 0.000001,
		"system-locale fallback keeps its decimal meaning");
}

void testBenchmarkBatchPlanUsesOnlyComparableFullBatches()
{
	const BenchmarkBatchPlan partial = planBenchmarkBatches(1000, 480);
	expectEqual(partial.processedFrames, 960u,
		"benchmark processes only full fixed-size batches");
	expectEqual(partial.trimmedFrames, 40u,
		"benchmark reports the excluded partial tail");

	const BenchmarkBatchPlan exact = planBenchmarkBatches(960, 480);
	expectEqual(exact.processedFrames, 960u,
		"batch-aligned benchmark lengths remain unchanged");
	expectEqual(exact.trimmedFrames, 0u,
		"batch-aligned benchmark lengths trim nothing");
}

void testFileReferenceControllerOwnsPathState()
{
	QTemporaryDir temp;
	requireTrue(temp.isValid(), "file-reference test creates a temporary root");
	QDir root(temp.path());
	requireTrue(root.mkpath(QStringLiteral("config/irs")),
		"file-reference test creates the config dependency directory");
	requireTrue(root.mkpath(QStringLiteral("outside")),
		"file-reference test creates the external directory");
	const QString configPath = root.filePath(QStringLiteral("config/config.txt"));
	const QString impulsePath = root.filePath(QStringLiteral("config/irs/room.wav"));
	QFile impulse(impulsePath);
	requireTrue(impulse.open(QIODevice::WriteOnly),
		"file-reference test creates the resolved dependency");
	impulse.close();

	FileReferenceController reference(
		QStringLiteral("convolution"), QStringLiteral("irs/room.wav"));
	reference.resolveAgainstConfig(configPath);
	ReferenceCardState state = reference.describe(QStringLiteral("No file selected"));
	expectFalse(state.missing, "controller resolves a config-relative reference");
	expectEqual(state.name, QStringLiteral("room.wav"),
		"controller derives the reference display name");
	expectEqual(state.directory, QDir::toNativeSeparators(QStringLiteral("irs")),
		"controller preserves the as-written relative directory");
	expectPath(state.fullPath, impulsePath);

	reference.setWrittenPath(QStringLiteral("irs/missing.wav"));
	reference.resolveAgainstConfig(configPath);
	state = reference.describe(QStringLiteral("No file selected"));
	expectTrue(state.missing, "controller owns and reports an edited missing path");
	expectTrue(state.fullPath.isEmpty(), "missing references do not expose a clickable path");

	const QString baseDirectory = root.filePath(QStringLiteral("config/deep"));
	const QString farSelection = root.filePath(QStringLiteral("outside/plugin.dll"));
	expectPath(FileReferenceController::displayPathForBaseDirectory(
		baseDirectory, farSelection), farSelection);

	const QString vst3Bundle = root.filePath(QStringLiteral("outside/Portable.vst3"));
	const QString vst3Module = vst3Bundle + QStringLiteral("/Contents/x86_64-win/Portable.vst3");
	requireTrue(root.mkpath(QFileInfo(vst3Module).absolutePath()),
		"file-reference test creates the VST3 bundle fixture");
	QFile module(vst3Module);
	requireTrue(module.open(QIODevice::WriteOnly),
		"file-reference test creates the VST3 module fixture");
	module.close();
	expectTrue(FileReferenceController::isVST3BundleDirectory(vst3Bundle),
		"VST3 bundle directory is an eligible direct VST selection");
	expectFalse(FileReferenceController::isVST3BundleDirectory(vst3Module),
		"VST3 module file must not masquerade as a bundle selection");
	expectFalse(FileReferenceController::isVST3BundleDirectory(root.filePath(QStringLiteral("outside"))),
		"ordinary directories are not VST3 bundle selections");

	FileReferenceController vstReference(
		QStringLiteral("vst"), QStringLiteral("kept-plugin.dll"));
	vstReference.setResolvedPath(farSelection);
	expectFalse(vstReference.setVST3BundleSelection(
		root.filePath(QStringLiteral("outside")), root.filePath(QStringLiteral("outside"))),
		"invalid VST3 selection must be rejected before controller state changes");
	expectEqual(vstReference.writtenPath(), QStringLiteral("kept-plugin.dll"),
		"rejected VST3 selection preserves the saved library path");
	expectPath(vstReference.resolvedPath(), farSelection);
	expectTrue(vstReference.setVST3BundleSelection(
		vst3Bundle, root.filePath(QStringLiteral("outside"))),
		"valid VST3 selection updates the controller");
	expectEqual(vstReference.writtenPath(), QStringLiteral("Portable.vst3"),
		"valid VST3 selection uses the configured base directory");
	expectPath(vstReference.resolvedPath(), vst3Bundle);
}

void testAnalysisWorkerRecovery()
{
	bool failurePublished = false;
	try
	{
		const bool succeeded = AnalysisWorkerRecovery::run(
			[] { throw std::bad_alloc(); },
			[&](const char*) { failurePublished = true; });
		expectFalse(succeeded, "failed analysis iteration reported success");
	}
	catch (...)
	{
		harness.fail("analysis iteration exception escaped and would stop the worker");
	}
	expectTrue(failurePublished, "analysis failure was not published");

	bool nextIterationRan = false;
	const bool recovered = AnalysisWorkerRecovery::run(
		[&] { nextIterationRan = true; },
		[](const char*) {});
	expectTrue(recovered, "next analysis iteration did not recover");
	expectTrue(nextIterationRan, "analysis worker did not accept work after a failure");
}

void testMemoryHelperConstructReleasesStorageWhenConstructorThrows()
{
	struct ThrowingConstructor
	{
		ThrowingConstructor()
		{
			throw std::runtime_error("constructor failure");
		}
	};

	// The counters live in AlignedMemory itself; this binary links Common.lib
	// whole-archive and therefore cannot substitute its own alloc()/free().
	AlignedMemory::resetAllocationCountsForTesting();
	bool threw = false;
	try
	{
		AlignedMemory::construct<ThrowingConstructor>();
	}
	catch (const std::runtime_error&)
	{
		threw = true;
	}

	expectTrue(threw, "construct propagates a constructor exception");
	expectEqual((int)AlignedMemory::allocationCountForTesting(), 1, "construct allocates storage once");
	expectEqual((int)AlignedMemory::freeCountForTesting(), 1, "construct releases storage when construction fails");
}
