/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "SkinDisplayNames.h"

#include <QCoreApplication>
#include <QHash>

namespace SkinDisplayNames
{
QString displayName(const QString& id)
{
	static const QHash<QString, const char*> names = {
		{ QStringLiteral("studio"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Studio Glass") },
		{ QStringLiteral("minimal"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Precision Minimal") },
		{ QStringLiteral("soft"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Soft Lab") },
		{ QStringLiteral("rack"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Hardware Rack") },
		{ QStringLiteral("matrix"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Signal Matrix") }
	};
	const auto it = names.constFind(id);
	if (it == names.constEnd())
		return id;
	return QCoreApplication::translate("SkinDisplayNames", it.value());
}
}
