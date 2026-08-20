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
		{ QStringLiteral("matrix"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Signal Matrix") },
		{ QStringLiteral("midnight"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Midnight Console") },
		{ QStringLiteral("arctic"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Arctic Bloom") },
		{ QStringLiteral("ember"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Ember Rack") },
		{ QStringLiteral("violet"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Violet Pulse") },
		{ QStringLiteral("solar"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Solar Paper") },
		{ QStringLiteral("obsidian"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Obsidian Glass") },
		{ QStringLiteral("aurora"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Aurora Veil") },
		{ QStringLiteral("forge"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Copper Forge") },
		{ QStringLiteral("nebula"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Neon Nebula") },
		{ QStringLiteral("noir"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Noir Chrome") },
		{ QStringLiteral("legacy-slate"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Legacy Slate") },
		{ QStringLiteral("legacy-blue"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Legacy Blue") },
		{ QStringLiteral("legacy-forest"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Legacy Forest") },
		{ QStringLiteral("legacy-bronze"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Legacy Bronze") },
		{ QStringLiteral("legacy-plum"), QT_TRANSLATE_NOOP("SkinDisplayNames", "Legacy Plum") }
	};
	const auto it = names.constFind(id);
	if (it == names.constEnd())
		return id;
	return QCoreApplication::translate("SkinDisplayNames", it.value());
}
}
