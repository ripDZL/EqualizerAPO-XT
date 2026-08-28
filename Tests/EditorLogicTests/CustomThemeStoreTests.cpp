/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "EditorLogicTestSupport.h"

#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>

#include "Editor/skins/CustomThemeStore.h"

void testCustomThemeStoreRoundTripsTokensAndJson()
{
	QTemporaryDir dir;
	requireTrue(dir.isValid(), QStringLiteral("custom theme store test has a temporary directory"));
	QSettings settings(dir.filePath(QStringLiteral("themes.ini")), QSettings::IniFormat);

	CustomThemeStore::Theme theme;
	theme.name = QStringLiteral("Night Lab");
	theme.id = CustomThemeStore::uniqueIdForName(settings, theme.name);
	theme.baseTheme = QStringLiteral("rack");
	theme.dark = true;
	theme.colors.insert(QStringLiteral("background"), QStringLiteral("#010203"));
	theme.colors.insert(QStringLiteral("accent"), QStringLiteral("#123456"));
	theme.colors.insert(QStringLiteral("danger"), QStringLiteral("#fedcba"));

	expectTrue(CustomThemeStore::saveTheme(settings, theme),
		QStringLiteral("custom theme saves to settings"));
	expectTrue(CustomThemeStore::isCustomThemeId(theme.skinId()),
		QStringLiteral("custom theme skin ids carry the custom prefix"));
	expectEqual(CustomThemeStore::storageId(theme.skinId()), theme.id,
		QStringLiteral("custom theme storage ids strip the custom prefix"));

	CustomThemeStore::Theme loaded;
	expectTrue(CustomThemeStore::findTheme(settings, theme.skinId(), &loaded),
		QStringLiteral("custom theme loads by custom skin id"));
	expectFalse(CustomThemeStore::findTheme(settings, theme.id, nullptr),
		QStringLiteral("bare storage ids do not hijack built-in skin resolution"));
	expectEqual(loaded.name, theme.name, QStringLiteral("custom theme name round-trips"));
	expectEqual(loaded.baseTheme, QStringLiteral("rack"), QStringLiteral("custom theme base skin round-trips"));
	expectTrue(loaded.dark, QStringLiteral("custom theme dark mode round-trips"));

	const SkinTokens tokens = CustomThemeStore::tokensForTheme(loaded);
	expectEqual(tokens.background, QStringLiteral("#010203"),
		QStringLiteral("custom theme overrides background token"));
	expectEqual(tokens.accent, QStringLiteral("#123456"),
		QStringLiteral("custom theme overrides accent token"));
	expectEqual(tokens.danger, QStringLiteral("#FEDCBA"),
		QStringLiteral("custom theme normalizes imported color values"));
	expectTrue(tokens.dark, QStringLiteral("custom theme tokens preserve dark mode"));

	const QJsonObject exported = CustomThemeStore::toJsonObject(loaded);
	CustomThemeStore::Theme parsed;
	QString error;
	expectTrue(CustomThemeStore::fromJsonObject(exported, &parsed, &error),
		QStringLiteral("exported custom theme JSON imports again"));
	expectEqual(parsed.name, loaded.name, QStringLiteral("custom theme JSON carries the name"));
	expectEqual(parsed.baseTheme, loaded.baseTheme, QStringLiteral("custom theme JSON carries the base skin"));
	expectEqual(parsed.colors.value(QStringLiteral("accent")), QStringLiteral("#123456"),
		QStringLiteral("custom theme JSON carries edited colors"));

	QJsonObject legacyColors;
	legacyColors.insert(QStringLiteral("accent"), QStringLiteral("#abcdef"));
	QJsonObject legacyExport;
	legacyExport.insert(QStringLiteral("baseTheme"), QStringLiteral("soft"));
	legacyExport.insert(QStringLiteral("dark"), false);
	legacyExport.insert(QStringLiteral("colors"), legacyColors);
	CustomThemeStore::Theme legacyTheme;
	expectTrue(CustomThemeStore::fromJsonObject(legacyExport, &legacyTheme, &error),
		QStringLiteral("transient Theme Lab v1 JSON without name/id still imports"));
	expectTrue(legacyTheme.id.isEmpty(), QStringLiteral("legacy Theme Lab JSON does not invent an id"));
	expectFalse(legacyTheme.dark, QStringLiteral("legacy Theme Lab JSON carries light mode"));
	expectEqual(CustomThemeStore::tokensForTheme(legacyTheme).accent, QStringLiteral("#ABCDEF"),
		QStringLiteral("legacy Theme Lab JSON colors normalize into tokens"));
	expectEqual(CustomThemeStore::uniqueIdForName(settings, QStringLiteral("Studio")), QStringLiteral("studio-2"),
		QStringLiteral("custom theme ids reserve built-in skin ids"));
	CustomThemeStore::Theme builtInCollision = theme;
	builtInCollision.id = QStringLiteral("studio");
	expectFalse(CustomThemeStore::saveTheme(settings, builtInCollision),
		QStringLiteral("custom theme store rejects direct built-in id collisions"));

	expectTrue(CustomThemeStore::removeTheme(settings, theme.skinId()),
		QStringLiteral("custom theme can be deleted"));
	expectFalse(CustomThemeStore::findTheme(settings, theme.skinId(), nullptr),
		QStringLiteral("deleted custom theme is no longer found"));
}
