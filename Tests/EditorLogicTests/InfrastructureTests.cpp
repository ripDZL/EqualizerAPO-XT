#include "EditorLogicTestSupport.h"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include <QDir>
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QLocale>
#include <QSettings>
#include <QTemporaryDir>

#include "Benchmark/BatchPlan.h"
#include "Editor/skins/SkinPaint.h"
#include "Editor/skins/SkinSupport.h"
#include "Editor/skins/CustomThemeStore.h"
#include "Editor/skins/SkinThemeData.h"
#include "Editor/widgets/EditableValueText.h"
#include "Editor/widgets/cards/FileReferenceController.h"
#include "helpers/OwnedBackgroundTask.h"
#include "helpers/UpdateElevationPolicy.h"

void testOwnedBackgroundTaskJoinsAndStartsOnlyOnce()
{
	OwnedBackgroundTask task;
	std::promise<void> enteredPromise;
	std::future<void> entered = enteredPromise.get_future();
	std::promise<void> releasePromise;
	std::shared_future<void> release = releasePromise.get_future().share();
	std::atomic<bool> completed{ false };

	expectTrue(task.startOnce([&]() {
		enteredPromise.set_value();
		release.wait();
		completed.store(true);
	}), "owned background task starts its worker");
	requireTrue(entered.wait_for(std::chrono::seconds(5)) == std::future_status::ready,
		"owned background task enters its worker");
	expectFalse(task.startOnce([]() {}), "owned background task rejects a second start");

	std::future<void> joined = std::async(std::launch::async, [&]() {
		task.join();
	});
	expectTrue(joined.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout,
		"join waits while the owned worker is active");
	releasePromise.set_value();
	requireTrue(joined.wait_for(std::chrono::seconds(5)) == std::future_status::ready,
		"join completes after the owned worker exits");
	joined.get();
	expectTrue(completed.load(), "join observes completion of the owned worker");
}

void testUpdateElevationPolicyUsesOnePromptForEditorUpdates()
{
	using UpdateElevationPolicy::ApplyMode;

	expectTrue(
		UpdateElevationPolicy::chooseApplyMode(true, false) == ApplyMode::LaunchElevatedCoordinator,
		"an unelevated Editor delegates a staged update to one elevated coordinator");
	expectTrue(
		UpdateElevationPolicy::chooseApplyMode(true, true) == ApplyMode::ApplyInCurrentProcess,
		"the elevated coordinator starts the updater without another elevation");
	expectFalse(UpdateElevationPolicy::hookMustSelfElevate(true),
		"update hooks inheriting the elevated updater token do not request UAC again");

	expectTrue(
		UpdateElevationPolicy::chooseApplyMode(false, false) == ApplyMode::None,
		"an Editor without a staged update does not request elevation");
	expectTrue(UpdateElevationPolicy::hookMustSelfElevate(false),
		"standalone install and uninstall hooks still elevate when required");
}

// The roster is the one list of which skins exist, so what is worth pinning is
// that it is complete and that everything derived from it lands on a real skin.
// Missing a skin used to be silent: resolveId() falls back to Studio, so a skin
// that was registered but forgotten in one of eighteen places was drawn as Studio
// with no error anywhere.
void testTheSkinRosterIsTheOneList()
{
	const QVector<SkinThemeData::SkinEntry>& roster = SkinThemeData::roster();
	requireTrue(roster.size() >= 15, "the roster carries the built-in skins and token variants");
	expectTrue(roster.first().id == QStringLiteral("studio"),
		"studio is first, which is what an unknown id falls back to and what the default is");

	QSet<QString> seen;
	for (const SkinThemeData::SkinEntry& skin : roster)
	{
		expectFalse(skin.id.isEmpty(), "every entry has an id");
		expectFalse(skin.qssBaseName.isEmpty(), "every entry names its style sheet");
		expectTrue(skin.tokens != nullptr, "every entry carries its token table");
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
	for (int index = 0; index < ids.size(); index++)
		expectTrue(ids[index] == roster[index].id, "ids() preserves the roster order, which is the display order");
	for (const QString& variant : { QStringLiteral("midnight"), QStringLiteral("arctic"),
		QStringLiteral("ember"), QStringLiteral("violet"), QStringLiteral("solar"),
		QStringLiteral("obsidian"), QStringLiteral("aurora"), QStringLiteral("forge"),
		QStringLiteral("nebula"), QStringLiteral("noir") })
	{
		expectTrue(ids.contains(variant),
			QStringLiteral("%1 is a selectable token-variant theme").arg(variant));
	}
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

	expectTrue(SkinThemeData::tokens(QStringLiteral("obsidian"), true).graphRadius == 12,
		"Obsidian Glass adds rounder glass graph chrome");
	expectTrue(SkinThemeData::tokens(QStringLiteral("aurora"), true).borderRadius == 18,
		"Aurora Veil uses a softer dark-mode radius");
	expectTrue(SkinThemeData::tokens(QStringLiteral("forge"), true).graphRadius == 6,
		"Copper Forge adds small-radius metal graph chrome");
	expectTrue(SkinThemeData::tokens(QStringLiteral("nebula"), true).cardRailWidth == 5,
		"Neon Nebula thickens Matrix rails");
	expectTrue(SkinThemeData::tokens(QStringLiteral("noir"), true).zebraStripe,
		"Noir Chrome keeps precision zebra striping");

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

// The alpha form a sheet needs to hold a token at partial opacity. QSS has no
// variables and its rgba() takes three numbers, so without this a sheet had to
// write the palette value out by hand and a token change stopped reaching it.
void testTokenSubstitutionOffersAnAlphaForm()
{
	const SkinTokens tokens = SkinThemeData::tokens(QStringLiteral("studio"), true);
	const QColor accent(tokens.accent);
	requireTrue(accent.isValid(), "the studio accent token parses as a colour");

	const QString resolved = SkinThemeData::substituteTokens(
		QStringLiteral("QWidget { background: rgba(@ACCENT_RGB@, 0.30); color: @ACCENT@; }"), tokens);

	expectTrue(resolved.contains(QStringLiteral("rgba(%1, %2, %3, 0.30)")
			.arg(accent.red()).arg(accent.green()).arg(accent.blue())),
		"the alpha form expands to the token's three channels, so rgba() gets numbers");
	expectTrue(resolved.contains(QStringLiteral("color: ") + tokens.accent),
		"and the plain form still expands to the hex value");
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
	const QString sourceDirectory = repoRoot.filePath(QStringLiteral("Editor/skins"));
	const QStringList skinIds = SkinThemeData::ids();
	for (const QString& skinId : skinIds)
	{
		for (bool dark : { false, true })
		{
			const SkinThemeData::ResolvedStyleSheet sheet =
				SkinThemeData::styleSheet(skinId, dark, sourceDirectory);
			expectTrue(sheet.loaded,
				QStringLiteral("loads %1 source sheet").arg(sheet.resourcePath));
			expectFalse(sheet.usedStudioFallback,
				QStringLiteral("%1 uses its own source sheet").arg(skinId));
			expectTrue(sheet.resolvedId == skinId,
				QStringLiteral("%1 stays canonical while resolving its sheet").arg(skinId));
			expectTrue(sheet.resourcePath == SkinThemeData::qssResource(skinId, dark),
				QStringLiteral("%1 resolves the expected sheet").arg(skinId));
			expectFalse(sheet.qss.isEmpty(),
				QStringLiteral("%1 produces a non-empty app stylesheet").arg(sheet.resourcePath));
			expectTrue(sheet.qss.contains(QStringLiteral("QComboBox::down-arrow")),
				QStringLiteral("%1 includes the shared combo arrow override").arg(sheet.resourcePath));
			expectTrue(sheet.qss.contains(QStringLiteral("QFileDialog QToolButton")),
				QStringLiteral("%1 includes the shared file dialog override").arg(sheet.resourcePath));
			expectTrue(sheet.unresolvedTokens.isEmpty(),
				QStringLiteral("%1 leaves no unresolved @TOKEN@ sentinel: %2")
					.arg(sheet.resourcePath, sheet.unresolvedTokens.join(QStringLiteral(", "))));
		}
	}

	const QStringList unresolved = SkinThemeData::unresolvedTokenPlaceholders(
		QStringLiteral("@ACCENT@ @CUSTOM_2@ @ACCENT@"));
	expectEqual(unresolved, QStringList({ QStringLiteral("@ACCENT@"), QStringLiteral("@CUSTOM_2@") }),
		"unresolved token reporting preserves first-seen order without duplicates");

	const QString alphaResolved = SkinThemeData::substituteTokens(
		QStringLiteral("@ACCENT_A30@ @SURFACE_SUNKEN_A05@ @SHADOW_A55@ @HIGHLIGHT_A95@ @ACCENT_A101@"),
		SkinThemeData::tokens(QStringLiteral("studio"), false));
	expectTrue(alphaResolved.contains(QStringLiteral("rgba(47, 107, 255, 0.30)")),
		"alpha token resolves an opaque token colour into rgba");
	expectTrue(alphaResolved.contains(QStringLiteral("rgba(246, 247, 251, 0.05)")),
		"alpha token handles token names with underscores");
	expectTrue(alphaResolved.contains(QStringLiteral("rgba(0, 0, 0, 0.55)"))
		&& alphaResolved.contains(QStringLiteral("rgba(255, 255, 255, 0.95)")),
		"alpha token handles fixed shadow/highlight material effects");
	expectTrue(SkinThemeData::unresolvedTokenPlaceholders(alphaResolved).contains(QStringLiteral("@ACCENT_A101@")),
		"invalid alpha tokens stay visible to unresolved-token reporting");
}

void testThemePreviewStyleSheetUsesCustomTokens()
{
	QDir repoRoot(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath());
	requireTrue(repoRoot.cdUp(), "theme-preview test reaches the tests directory");
	requireTrue(repoRoot.cdUp(), "theme-preview test reaches the repository root");
	const QString sourceDirectory = repoRoot.filePath(QStringLiteral("Editor/skins"));

	SkinTokens tokens = SkinThemeData::tokens(QStringLiteral("midnight"), true);
	tokens.accent = QStringLiteral("#123456");
	finishTokens(tokens);

	const SkinThemeData::ResolvedStyleSheet sheet =
		SkinThemeData::styleSheetForTokens(QStringLiteral("midnight"), true, tokens, sourceDirectory);
	expectTrue(sheet.loaded, "theme preview loads the base skin source sheet");
	expectTrue(sheet.resolvedId == QStringLiteral("midnight"),
		"theme preview keeps the selected variant id");
	expectTrue(sheet.resourcePath == SkinThemeData::qssResource(QStringLiteral("midnight"), true),
		"theme preview uses the variant's QSS grammar");
	expectTrue(sheet.qss.contains(QStringLiteral("#123456")),
		"theme preview substitutes edited token values");
	expectTrue(sheet.unresolvedTokens.isEmpty(),
		"theme preview leaves no unresolved @TOKEN@ sentinels");
}

void testCustomThemeStoreRoundTripsTokensAndJson()
{
	QTemporaryDir dir;
	requireTrue(dir.isValid(), "custom theme store test has a temporary directory");
	QSettings settings(dir.filePath(QStringLiteral("themes.ini")), QSettings::IniFormat);

	CustomThemeStore::Theme theme;
	theme.name = QStringLiteral("Night Lab");
	theme.id = CustomThemeStore::uniqueIdForName(settings, theme.name);
	theme.baseTheme = QStringLiteral("midnight");
	theme.dark = true;
	theme.colors.insert(QStringLiteral("background"), QStringLiteral("#010203"));
	theme.colors.insert(QStringLiteral("accent"), QStringLiteral("#123456"));
	theme.colors.insert(QStringLiteral("danger"), QStringLiteral("#fedcba"));

	expectTrue(CustomThemeStore::saveTheme(settings, theme),
		"custom theme saves to settings");
	expectTrue(CustomThemeStore::isCustomThemeId(theme.skinId()),
		"custom theme skin ids carry the custom prefix");
	expectEqual(CustomThemeStore::storageId(theme.skinId()), theme.id,
		"custom theme storage ids strip the custom prefix");

	CustomThemeStore::Theme loaded;
	expectTrue(CustomThemeStore::findTheme(settings, theme.skinId(), &loaded),
		"custom theme loads by custom skin id");
	expectFalse(CustomThemeStore::findTheme(settings, theme.id, nullptr),
		"bare storage ids do not hijack built-in skin resolution");
	expectEqual(loaded.name, theme.name, "custom theme name round-trips");
	expectEqual(loaded.baseTheme, QStringLiteral("midnight"), "custom theme base skin round-trips");
	expectTrue(loaded.dark, "custom theme dark mode round-trips");

	const SkinTokens tokens = CustomThemeStore::tokensForTheme(loaded);
	expectEqual(tokens.background, QStringLiteral("#010203"),
		"custom theme overrides background token");
	expectEqual(tokens.accent, QStringLiteral("#123456"),
		"custom theme overrides accent token");
	expectEqual(tokens.danger, QStringLiteral("#FEDCBA"),
		"custom theme normalizes imported colour values");
	expectTrue(tokens.dark, "custom theme tokens preserve dark mode");

	const QJsonObject exported = CustomThemeStore::toJsonObject(loaded);
	CustomThemeStore::Theme parsed;
	QString error;
	expectTrue(CustomThemeStore::fromJsonObject(exported, &parsed, &error),
		"exported custom theme JSON imports again");
	expectEqual(parsed.name, loaded.name, "custom theme JSON carries the name");
	expectEqual(parsed.baseTheme, loaded.baseTheme, "custom theme JSON carries the base skin");
	expectEqual(parsed.colors.value(QStringLiteral("accent")), QStringLiteral("#123456"),
		"custom theme JSON carries edited colors");

	QJsonObject legacyColors;
	legacyColors.insert(QStringLiteral("accent"), QStringLiteral("#abcdef"));
	QJsonObject legacyExport;
	legacyExport.insert(QStringLiteral("baseTheme"), QStringLiteral("solar"));
	legacyExport.insert(QStringLiteral("dark"), false);
	legacyExport.insert(QStringLiteral("colors"), legacyColors);
	CustomThemeStore::Theme legacyTheme;
	expectTrue(CustomThemeStore::fromJsonObject(legacyExport, &legacyTheme, &error),
		"transient Theme Lab v1 JSON without name/id still imports");
	expectTrue(legacyTheme.id.isEmpty(), "legacy Theme Lab JSON does not invent an id");
	expectFalse(legacyTheme.dark, "legacy Theme Lab JSON carries light mode");
	expectEqual(CustomThemeStore::tokensForTheme(legacyTheme).accent, QStringLiteral("#ABCDEF"),
		"legacy Theme Lab JSON colors normalize into tokens");
	expectEqual(CustomThemeStore::uniqueIdForName(settings, QStringLiteral("Studio")), QStringLiteral("studio-2"),
		"custom theme ids reserve built-in skin ids");
	CustomThemeStore::Theme builtInCollision = theme;
	builtInCollision.id = QStringLiteral("studio");
	expectFalse(CustomThemeStore::saveTheme(settings, builtInCollision),
		"custom theme store rejects direct built-in id collisions");

	expectTrue(CustomThemeStore::removeTheme(settings, theme.skinId()),
		"custom theme can be deleted");
	expectFalse(CustomThemeStore::findTheme(settings, theme.skinId(), nullptr),
		"deleted custom theme is no longer found");
}

void testSkinMaterialEffectHelpersStayFixedBlackAndWhite()
{
	const QColor shadow = skinMaterialShadow(55);
	expectEqual(shadow.red(), 0, "material shadow keeps fixed black red channel");
	expectEqual(shadow.green(), 0, "material shadow keeps fixed black green channel");
	expectEqual(shadow.blue(), 0, "material shadow keeps fixed black blue channel");
	expectEqual(shadow.alpha(), 55, "material shadow carries caller alpha");

	const QColor highlight = skinMaterialHighlight(95);
	expectEqual(highlight.red(), 255, "material highlight keeps fixed white red channel");
	expectEqual(highlight.green(), 255, "material highlight keeps fixed white green channel");
	expectEqual(highlight.blue(), 255, "material highlight keeps fixed white blue channel");
	expectEqual(highlight.alpha(), 95, "material highlight carries caller alpha");
	expectEqual(skinMaterialHighlight().alpha(), 255, "material highlight defaults opaque");
}

void testSkinScopeGutterLayoutKeepsIfBranchesOnMemberRails()
{
	SkinTokens tokens;
	tokens.channelGroupIndent = 20;
	tokens.rowHeight = 48;
	const QSize rowSize(200, 52);

	const SkinScopeGutterLayout head = skinScopeGutterLayout(
		QStringLiteral("if"), QStringLiteral("if"), 2, 1, tokens, rowSize);
	expectTrue(head.shouldPaint, "If head rows always paint their starter rail");
	expectTrue(head.headRow, "If head rows are classified as heads");
	expectEqual(head.indentUnits, 2, "If head uses its semantic indent");
	expectEqual(head.ifLevels, 1, "If head exposes one logic rail");
	expectEqual(head.channelLevels, 1, "If head preserves outer channel rails");
	expectEqual(head.ownLevel, 2, "If head starter rail sits one band past its live rail");
	expectEqual(head.cardLeft, 48, "If head card edge follows its semantic indent");
	expectEqual(head.bandCenter(2), 58, "integer rail centers stay on the existing gutter grid");

	const SkinScopeGutterLayout member = skinScopeGutterLayout(
		QStringLiteral("biquad"), QStringLiteral("filter"), 3, 2, tokens, rowSize);
	expectTrue(member.shouldPaint, "members inside an If block paint inherited rails");
	expectFalse(member.ifFamily, "ordinary rows are not If-family rows");
	expectEqual(member.indentUnits, 3, "member rows keep their current depth");
	expectEqual(member.ifLevels, 2, "members keep every inherited logic rail");
	expectEqual(member.channelLevels, 1, "members preserve channel rails outside inherited If rails");
	expectEqual(member.ownLevel, 2, "member row's own rail is the innermost inherited logic rail");

	const SkinScopeGutterLayout branch = skinScopeGutterLayout(
		QStringLiteral("if"), QStringLiteral("else"), 2, 2, tokens, rowSize);
	expectTrue(branch.branchRow, "Else rows are branch rows");
	expectTrue(branch.branchOrTail, "Else rows use the branch-or-tail rail rule");
	expectEqual(branch.indentUnits, 3, "Else rows mount on member rails");
	expectEqual(branch.ifLevels, 1, "Else rows reserve the current branch rail separately");
	expectEqual(branch.channelLevels, 1, "Else rows keep channel rails outside inherited If rails");
	expectEqual(branch.ownLevel, 2, "Else branch marker lands on the active branch rail");
	expectEqual(branch.cardLeft, 68, "Else card edge follows the mounted member rail");

	const SkinScopeGutterLayout tail = skinScopeGutterLayout(
		QStringLiteral("if"), QStringLiteral("endif"), 2, 2, tokens, rowSize);
	expectTrue(tail.tailRow, "EndIf rows are tail rows");
	expectEqual(tail.indentUnits, branch.indentUnits, "EndIf rows mount with branches");
	expectEqual(tail.ownLevel, branch.ownLevel, "EndIf cap lands on the same rail as the branch marker");
	expectEqual(tail.junctionY, 28, "integer junction uses the shared row-center math");

	const SkinScopeGutterLayout flat = skinScopeGutterLayout(
		QStringLiteral("biquad"), QStringLiteral("filter"), 0, 0, tokens, rowSize);
	expectFalse(flat.shouldPaint, "flat non-scope rows leave the shared gutter painter in charge");
}

void testSkinAnalysisGraphLayoutKeepsAxisMathShared()
{
	const SkinAnalysisGraphLayout layout = skinAnalysisGraphLayout(
		QRect(0, 0, 100, 80), QRectF(10.0, 5.0, 80.0, 50.0), 70.0, 1.25);

	expectEqual(layout.plotLeft(), 10, "analysis layout exposes integer plot left");
	expectEqual(layout.plotRight(), 90, "analysis layout exposes integer plot right");
	expectEqual(layout.plotTop(), 5, "analysis layout exposes integer plot top");
	expectEqual(layout.plotBottom(), 55, "analysis layout exposes integer plot bottom");
	expectEqual(layout.zeroRow(), 70, "analysis layout preserves the unclamped zero row for edge tests");
	expectTrue(qAbs(layout.zeroClamped - 55.0) < 0.001, "analysis layout clamps zero to the plot edge");
	expectTrue(qAbs(layout.hover - 1.0) < 0.001, "analysis layout clamps hover to the animation range");
	expectTrue(layout.truncatedXAxisLabelRect(42.9, 2, 48, 11) == QRect(18, 57, 48, 11),
		"truncated x-axis label rect preserves Studio/Rack integer placement");
	expectTrue(layout.roundedXAxisLabelRect(42.9, 2, 48, 12) == QRect(19, 57, 48, 12),
		"rounded x-axis label rect preserves Soft edge placement");

	const SkinAxisLabelRect left = layout.clampedRoundedXAxisLabelRect(3.0, 2, 48, 12, 8);
	expectEqual(left.rect.left(), 8, "clamped x-axis labels stay inside the left edge inset");
	expectTrue((left.alignment & Qt::AlignLeft) != 0, "left-clamped x-axis labels switch to left alignment");
	const SkinAxisLabelRect right = layout.clampedRoundedXAxisLabelRect(98.0, 2, 48, 12, 8);
	expectEqual(right.rect.right(), 91, "clamped x-axis labels stay inside the right edge inset");
	expectTrue((right.alignment & Qt::AlignRight) != 0, "right-clamped x-axis labels switch to right alignment");

	expectTrue(layout.centeredRectClampedToX(4, 7, 20, 5, 2, 40) == QRect(2, 7, 20, 5),
		"centered cells clamp their left edge to the minimum x");
	expectTrue(layout.centeredRectClampedToX(80, 7, 20, 5, 2, 40) == QRect(40, 7, 20, 5),
		"centered cells clamp their left edge to the maximum x");
	expectTrue(layout.footerRectF(3.0, 18.0) == QRectF(10.0, 58.0, 80.0, 18.0),
		"footer rects are anchored under the plot");

	struct TestGridLine
	{
		double pos = 0.0;
		bool major = false;
	};
	const QVector<TestGridLine> lines = {
		{ 10.0, false },
		{ 22.0, true },
		{ 40.0, false }
	};
	expectTrue(qAbs(skinMinimumAdjacentGridGap(lines, 1000.0) - 12.0) < 0.001,
		"grid gap helper finds the closest adjacent rules");
	expectEqual(skinFirstMajorGridIndex(lines), 1, "grid major helper finds the first major rule");
	expectEqual(skinLabelStrideForGap(5.0, 16.0), 4, "grid stride helper thins crowded labels");
	expectEqual(skinLabelStrideForGap(0.3, 16.0), 1, "grid stride helper keeps a safe fallback for degenerate gaps");
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
}
