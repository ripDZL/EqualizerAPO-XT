/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "CustomThemeStore.h"

#include <QColor>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>
#include <QVector>

#include "Editor/skins/shared/SkinSupport.h"
#include "Editor/skins/SkinThemeData.h"

namespace
{
const QString kArrayKey = QStringLiteral("interface/customThemes");
const QString kCustomPrefix = QStringLiteral("custom:");

struct ColorField
{
	const char* key = nullptr;
	QString SkinTokens::* field = nullptr;
};

const QVector<ColorField>& editableColorFields()
{
	static const QVector<ColorField> fields = {
		{ "background", &SkinTokens::background },
		{ "surface", &SkinTokens::surface },
		{ "card", &SkinTokens::card },
		{ "cardHover", &SkinTokens::cardHover },
		{ "cardSelected", &SkinTokens::cardSelected },
		{ "text", &SkinTokens::text },
		{ "mutedText", &SkinTokens::mutedText },
		{ "border", &SkinTokens::border },
		{ "graph", &SkinTokens::graph },
		{ "graphGridMinor", &SkinTokens::graphGridMinor },
		{ "accent", &SkinTokens::accent },
		{ "accent2", &SkinTokens::accent2 },
		{ "success", &SkinTokens::success },
		{ "warning", &SkinTokens::warning },
		{ "danger", &SkinTokens::danger }
	};
	return fields;
}

QString normalizeColor(const QString& value)
{
	const QColor color(value.trimmed());
	if (!color.isValid())
		return QString();
	return color.name(QColor::HexRgb).toUpper();
}

QSet<QString> existingIds(QSettings& settings)
{
	QSet<QString> result;
	for (const QString& id : SkinThemeData::ids())
		result.insert(id);
	for (const CustomThemeStore::Theme& theme : CustomThemeStore::themes(settings))
		result.insert(theme.id);
	return result;
}

QString colorsToJson(const QMap<QString, QString>& colors)
{
	QJsonObject object;
	for (auto it = colors.cbegin(); it != colors.cend(); ++it)
		object.insert(it.key(), it.value());
	return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QMap<QString, QString> colorsFromJson(const QString& json)
{
	QMap<QString, QString> result;
	QJsonParseError error;
	const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError || !document.isObject())
		return result;

	const QJsonObject object = document.object();
	for (auto it = object.constBegin(); it != object.constEnd(); ++it)
	{
		const QString normalized = normalizeColor(it.value().toString());
		if (!normalized.isEmpty())
			result.insert(it.key(), normalized);
	}
	return result;
}
}

namespace CustomThemeStore
{
QString Theme::skinId() const
{
	return kCustomPrefix + id;
}

QString customPrefix()
{
	return kCustomPrefix;
}

bool isCustomThemeId(const QString& id)
{
	return id.startsWith(kCustomPrefix);
}

QString storageId(const QString& id)
{
	return isCustomThemeId(id) ? id.mid(kCustomPrefix.size()) : id;
}

QString suggestedId(const QString& name)
{
	QString id = name.trimmed().toLower();
	id.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")), QStringLiteral("-"));
	id.replace(QRegularExpression(QStringLiteral("(^-+|-+$)")), QString());
	if (id.isEmpty())
		id = QStringLiteral("theme");
	return id.left(48);
}

QString uniqueIdForName(QSettings& settings, const QString& name)
{
	const QString base = suggestedId(name);
	const QSet<QString> used = existingIds(settings);
	if (!used.contains(base))
		return base;

	for (int suffix = 2; suffix < 10000; ++suffix)
	{
		const QString candidate = QStringLiteral("%1-%2").arg(base).arg(suffix);
		if (!used.contains(candidate))
			return candidate;
	}
	return QStringLiteral("%1-%2").arg(base).arg(QDateTime::currentMSecsSinceEpoch());
}

QList<Theme> themes(QSettings& settings)
{
	QList<Theme> result;
	const int size = settings.beginReadArray(kArrayKey);
	for (int index = 0; index < size; ++index)
	{
		settings.setArrayIndex(index);
		Theme theme;
		theme.id = storageId(settings.value(QStringLiteral("id")).toString()).trimmed();
		theme.name = settings.value(QStringLiteral("name")).toString().trimmed();
		theme.baseTheme = settings.value(QStringLiteral("baseTheme"), QStringLiteral("studio")).toString();
		theme.dark = settings.value(QStringLiteral("dark"), true).toBool();
		theme.colors = colorsFromJson(settings.value(QStringLiteral("colorsJson")).toString());
		if (!theme.id.isEmpty() && !theme.name.isEmpty())
			result.append(theme);
	}
	settings.endArray();
	return result;
}

bool findTheme(QSettings& settings, const QString& id, Theme* out)
{
	if (!isCustomThemeId(id))
		return false;
	const QString wanted = storageId(id);
	for (const Theme& theme : themes(settings))
	{
		if (theme.id == wanted)
		{
			if (out != nullptr)
				*out = theme;
			return true;
		}
	}
	return false;
}

bool saveTheme(QSettings& settings, const Theme& theme)
{
	Theme saved = theme;
	saved.id = storageId(saved.id).trimmed();
	saved.name = saved.name.trimmed();
	saved.baseTheme = SkinThemeData::resolveId(saved.baseTheme);
	if (saved.id.isEmpty() || saved.name.isEmpty() || SkinThemeData::ids().contains(saved.id))
		return false;

	QList<Theme> list = themes(settings);
	bool replaced = false;
	for (Theme& existing : list)
	{
		if (existing.id == saved.id)
		{
			existing = saved;
			replaced = true;
			break;
		}
	}
	if (!replaced)
		list.append(saved);

	settings.beginWriteArray(kArrayKey);
	for (int index = 0; index < list.size(); ++index)
	{
		settings.setArrayIndex(index);
		settings.setValue(QStringLiteral("id"), list[index].id);
		settings.setValue(QStringLiteral("name"), list[index].name);
		settings.setValue(QStringLiteral("baseTheme"), list[index].baseTheme);
		settings.setValue(QStringLiteral("dark"), list[index].dark);
		settings.setValue(QStringLiteral("colorsJson"), colorsToJson(list[index].colors));
	}
	settings.endArray();
	settings.sync();
	return true;
}

bool removeTheme(QSettings& settings, const QString& id)
{
	const QString removedId = storageId(id);
	QList<Theme> list = themes(settings);
	int removed = 0;
	for (int index = list.size() - 1; index >= 0; --index)
	{
		if (list[index].id == removedId)
		{
			list.removeAt(index);
			removed++;
		}
	}
	if (removed == 0)
		return false;

	settings.remove(kArrayKey);
	settings.beginWriteArray(kArrayKey);
	for (int index = 0; index < list.size(); ++index)
	{
		settings.setArrayIndex(index);
		settings.setValue(QStringLiteral("id"), list[index].id);
		settings.setValue(QStringLiteral("name"), list[index].name);
		settings.setValue(QStringLiteral("baseTheme"), list[index].baseTheme);
		settings.setValue(QStringLiteral("dark"), list[index].dark);
		settings.setValue(QStringLiteral("colorsJson"), colorsToJson(list[index].colors));
	}
	settings.endArray();
	settings.sync();
	return true;
}

SkinTokens tokensForTheme(const Theme& theme)
{
	SkinTokens tokens = SkinThemeData::tokens(theme.baseTheme, theme.dark);
	tokens.dark = theme.dark;
	for (const ColorField& spec : editableColorFields())
	{
		const auto it = theme.colors.constFind(QString::fromLatin1(spec.key));
		if (it == theme.colors.constEnd())
			continue;
		const QString normalized = normalizeColor(*it);
		if (!normalized.isEmpty())
			tokens.*(spec.field) = normalized;
	}
	finishTokens(tokens);
	return tokens;
}

QJsonObject toJsonObject(const Theme& theme)
{
	QJsonObject colors;
	for (auto it = theme.colors.cbegin(); it != theme.colors.cend(); ++it)
		colors.insert(it.key(), it.value());

	QJsonObject root;
	if (!theme.id.isEmpty())
		root.insert(QStringLiteral("id"), storageId(theme.id));
	root.insert(QStringLiteral("name"), theme.name);
	root.insert(QStringLiteral("baseTheme"), SkinThemeData::resolveId(theme.baseTheme));
	root.insert(QStringLiteral("dark"), theme.dark);
	root.insert(QStringLiteral("colors"), colors);
	return root;
}

bool fromJsonObject(const QJsonObject& object, Theme* theme, QString* error)
{
	if (theme == nullptr)
		return false;
	if (!object.value(QStringLiteral("colors")).isObject())
	{
		if (error != nullptr)
			*error = QStringLiteral("Theme JSON must contain a colors object.");
		return false;
	}

	Theme parsed;
	parsed.id = storageId(object.value(QStringLiteral("id")).toString()).trimmed();
	parsed.name = object.value(QStringLiteral("name")).toString().trimmed();
	parsed.baseTheme = SkinThemeData::resolveId(
		object.value(QStringLiteral("baseTheme")).toString(QStringLiteral("studio")));
	parsed.dark = object.value(QStringLiteral("dark")).toBool(true);

	const QJsonObject colors = object.value(QStringLiteral("colors")).toObject();
	for (auto it = colors.constBegin(); it != colors.constEnd(); ++it)
	{
		const QString normalized = normalizeColor(it.value().toString());
		if (!normalized.isEmpty())
			parsed.colors.insert(it.key(), normalized);
	}
	if (parsed.colors.isEmpty())
	{
		if (error != nullptr)
			*error = QStringLiteral("Theme JSON does not contain any valid colors.");
		return false;
	}

	*theme = parsed;
	return true;
}
}
