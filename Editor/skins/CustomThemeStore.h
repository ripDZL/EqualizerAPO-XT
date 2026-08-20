/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#pragma once

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QString>

#include "Editor/SkinTokens.h"

namespace CustomThemeStore
{
struct Theme
{
	QString id;
	QString name;
	QString baseTheme = QStringLiteral("studio");
	bool dark = true;
	QMap<QString, QString> colors;

	QString skinId() const;
};

QString customPrefix();
bool isCustomThemeId(const QString& id);
QString storageId(const QString& id);
QString suggestedId(const QString& name);
QString uniqueIdForName(QSettings& settings, const QString& name);

QList<Theme> themes(QSettings& settings);
bool findTheme(QSettings& settings, const QString& id, Theme* out);
bool saveTheme(QSettings& settings, const Theme& theme);
bool removeTheme(QSettings& settings, const QString& id);

SkinTokens tokensForTheme(const Theme& theme);
QJsonObject toJsonObject(const Theme& theme);
bool fromJsonObject(const QJsonObject& object, Theme* theme, QString* error = nullptr);
}
