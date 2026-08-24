/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.

	Skin theme data (see SkinThemeData.h). The skin classes' tokens()
	overrides delegate here, so this file is the single source of truth for
	skin colours.
*/

#include "SkinThemeData.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QtMath>

#include <limits>

// finishTokens; header-only, no link dependency on the skin classes.
#include "shared/SkinSupport.h"

namespace
{
double relativeLuminance(const QColor& color)
{
	if (!color.isValid())
		return 0.0;

	auto linear = [](double channel) {
		channel /= 255.0;
		return channel <= 0.04045
			? channel / 12.92
			: qPow((channel + 0.055) / 1.055, 2.4);
	};
	return 0.2126 * linear(color.red())
		+ 0.7152 * linear(color.green())
		+ 0.0722 * linear(color.blue());
}

double contrastRatioForColors(const QColor& foreground, const QColor& background)
{
	if (!foreground.isValid() || !background.isValid())
		return 0.0;
	const double lighter = qMax(relativeLuminance(foreground), relativeLuminance(background));
	const double darker = qMin(relativeLuminance(foreground), relativeLuminance(background));
	return (lighter + 0.05) / (darker + 0.05);
}

double minimumInkContrast(const QColor& ink, const SkinTokens& tokens)
{
	const QColor grounds[] = {
		QColor(tokens.background),
		QColor(tokens.surface),
		QColor(tokens.card),
		QColor(tokens.cardHover),
		QColor(tokens.cardSelected),
		QColor(tokens.surfaceSunken)
	};
	double minimum = std::numeric_limits<double>::max();
	for (const QColor& ground : grounds)
		minimum = qMin(minimum, contrastRatioForColors(ink, ground));
	return minimum;
}

QString raiseOrLowerInkForReadability(const QString& token, const SkinTokens& tokens)
{
	const QColor original(token);
	if (!original.isValid() || minimumInkContrast(original, tokens) >= 4.5)
		return token;

	QColor hsl = original.toHsl();
	int hue = 0;
	int saturation = 0;
	int lightness = 0;
	int alpha = 255;
	hsl.getHsl(&hue, &saturation, &lightness, &alpha);

	const int direction = tokens.dark ? 1 : -1;
	for (int candidateLightness = lightness;
		candidateLightness >= 0 && candidateLightness <= 255;
		candidateLightness += direction)
	{
		QColor candidate;
		candidate.setHsl(hue, saturation, candidateLightness, alpha);
		if (minimumInkContrast(candidate, tokens) >= 4.5)
			return candidate.name(QColor::HexRgb).toUpper();
	}

	// A custom token table can place dark and light grounds together, where no
	// single ink can meet the floor. Built-in palettes never take this path,
	// but keeping the original preserves the user's edit for Theme Lab to flag.
	return token;
}

void enforceTextReadability(SkinTokens& tokens)
{
	// Preserve each skin's hue and hierarchy, only moving an ink's HSL
	// lightness far enough to pass every surface it is used on. This puts the
	// contrast repair at the shared token seam so variants cannot drift.
	tokens.text = raiseOrLowerInkForReadability(tokens.text, tokens);
	tokens.mutedText = raiseOrLowerInkForReadability(tokens.mutedText, tokens);
}

QString bestInkForBackground(const QString& background, const SkinTokens& tokens)
{
	// A selection fill needs one of the palette's already-authored inks. Do not
	// smuggle a literal black/white fallback into a custom theme.
	const QString candidates[] = {
		tokens.text, tokens.background, tokens.surface, tokens.card, tokens.mutedText
	};
	QString best = tokens.text;
	double bestRatio = -1.0;
	for (const QString& candidate : candidates)
	{
		const double ratio = contrastRatioForColors(QColor(candidate), QColor(background));
		if (ratio > bestRatio)
		{
			best = candidate;
			bestRatio = ratio;
		}
	}
	return best;
}

QString repairInkForBackground(const QString& token, const QString& background)
{
	const QColor original(token);
	const QColor ground(background);
	if (!original.isValid() || !ground.isValid()
		|| contrastRatioForColors(original, ground) >= 4.5)
		return token;

	QColor hsl = original.toHsl();
	int hue = 0;
	int saturation = 0;
	int lightness = 0;
	int alpha = 255;
	hsl.getHsl(&hue, &saturation, &lightness, &alpha);
	const int direction = relativeLuminance(ground) >= 0.179 ? -1 : 1;
	for (int candidateLightness = lightness;
		candidateLightness >= 0 && candidateLightness <= 255;
		candidateLightness += direction)
	{
		QColor candidate;
		candidate.setHsl(hue, saturation, candidateLightness, alpha);
		if (contrastRatioForColors(candidate, ground) >= 4.5)
			return candidate.name(QColor::HexRgb).toUpper();
	}
	return token;
}

void appendReadabilityCheck(QVector<SkinThemeData::ReadabilityCheck>& checks,
	const QString& label, const char* foregroundToken, const QString& foreground,
	const char* backgroundToken, const QString& background)
{
	SkinThemeData::ReadabilityCheck check;
	check.label = label;
	check.foregroundToken = QString::fromLatin1(foregroundToken);
	check.backgroundToken = QString::fromLatin1(backgroundToken);
	check.ratio = SkinThemeData::contrastRatio(foreground, background);
	checks.append(check);
}

// Constitution: docs/skins/studio.md
SkinTokens studioTokens(bool dark)
{
	SkinTokens t;
	t.dark = dark;
	t.fontFamily = QStringLiteral("DM Sans");
	t.monoFontFamily = QStringLiteral("DM Mono");
	t.borderRadius = 8;
	// 36, down from 40 (2026-08 density round): a long config only ever
	// shows a handful of cards per screen, and the header floor was the
	// binding height on every collapsed row.
	t.rowHeight = 36;
	t.channelGroupIndent = 18;
	t.channelGroupStyle = SkinTokens::GradientBar;
	t.badgeStyle = SkinTokens::ColorPill;
	if (dark)
	{
		t.background = QStringLiteral("#070A12");
		t.surface = QStringLiteral("#0D1322");
		t.card = QStringLiteral("#121A2C");
		t.cardHover = QStringLiteral("#182238");
		t.cardSelected = QStringLiteral("#1E3158");
		t.text = QStringLiteral("#E8EEFB");
		t.mutedText = QStringLiteral("#91A0BA");
		t.border = QStringLiteral("#26324A");
		t.graph = QStringLiteral("#060914");
		t.graphGridMinor = QStringLiteral("#26324A");
		t.accent = QStringLiteral("#5B8CFF");
		t.accent2 = QStringLiteral("#A66CFF");
	}
	else
	{
		t.background = QStringLiteral("#EEF2F8");
		t.surface = QStringLiteral("#F8FAFE");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#F3F6FC");
		t.cardSelected = QStringLiteral("#DDE8FF");
		t.text = QStringLiteral("#182033");
		t.mutedText = QStringLiteral("#5D6A84");
		t.border = QStringLiteral("#BCC8DE");
		t.graph = QStringLiteral("#F6F7FB");
		t.graphGridMinor = QStringLiteral("#D8E0EF");
		t.accent = QStringLiteral("#2F6BFF");
		t.accent2 = QStringLiteral("#8A4DFF");
	}
	finishTokens(t);
	return t;
}

// Constitution: docs/skins/minimal.md
SkinTokens minimalTokens(bool dark)
{
	SkinTokens t;
	t.dark = dark;
	t.accent = QStringLiteral("#3B82F6");
	t.fontFamily = QStringLiteral("DM Mono");
	t.monoFontFamily = QStringLiteral("DM Mono");
	t.borderRadius = 0;
	t.rowHeight = 32;
	t.channelGroupIndent = 16;
	t.channelGroupStyle = SkinTokens::TreeLines;
	t.badgeStyle = SkinTokens::OutlineOnly;
	t.zebraStripe = true;
	if (dark)
	{
		t.background = QStringLiteral("#191919");
		t.surface = QStringLiteral("#1f1f1f");
		t.card = QStringLiteral("#262626");
		t.cardHover = QStringLiteral("#2c2c2c");
		t.cardSelected = QStringLiteral("#2A4878");
		t.text = QStringLiteral("#cccccc");
		t.mutedText = QStringLiteral("#777777");
		t.border = QStringLiteral("#3c3c3c");
		t.graph = QStringLiteral("#0e0e0e");
		t.graphGridMajor = QStringLiteral("#383838");
		t.graphGridMinor = QStringLiteral("#2c2c2c");
	}
	else
	{
		t.background = QStringLiteral("#F6F6F3");
		t.surface = QStringLiteral("#FFFFFF");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#F0F0EC");
		t.cardSelected = QStringLiteral("#E8F1FF");
		t.text = QStringLiteral("#202020");
		t.mutedText = QStringLiteral("#666660");
		t.border = QStringLiteral("#D2D2CC");
		t.graph = QStringLiteral("#FFFFFF");
		t.graphGridMajor = QStringLiteral("#D2D2CC");
		t.graphGridMinor = QStringLiteral("#E6E6E0");
	}
	finishTokens(t);
	return t;
}

// Constitution: docs/skins/soft.md
SkinTokens softTokens(bool dark)
{
	SkinTokens t;
	t.dark = dark;
	t.fontFamily = QStringLiteral("DM Sans");
	t.monoFontFamily = QStringLiteral("DM Mono");
	t.borderRadius = 14;
	// 44, down from 48 (2026-08 density round): still the airiest header of
	// the five, but no longer half again the compact skins' height.
	t.rowHeight = 44;
	t.channelGroupIndent = 20;
	t.density = 2;
	t.channelGroupStyle = SkinTokens::SoftShadow;
	t.badgeStyle = SkinTokens::SoftPill;
	// The accent and the semantic colours live on the pastel shelf
	// themselves (the softPastelize recipe applied to the old saturated
	// values), so every consumer - knob arcs, focus rings, toggles, ON
	// pills, severity inks - is pastel without knowing it.
	if (dark)
	{
		t.background = QStringLiteral("#1C1A17");
		t.surface = QStringLiteral("#262320");
		t.card = QStringLiteral("#2F2B26");
		t.cardHover = QStringLiteral("#38332D");
		// The pastel accent mixed deep into the card (softMix 0.75).
		t.cardSelected = QStringLiteral("#3F4650");
		t.text = QStringLiteral("#F4F1EA");
		t.mutedText = QStringLiteral("#B3AB9D");
		t.border = QStringLiteral("#423D34");
		t.graph = QStringLiteral("#181613");
		t.accent = QStringLiteral("#6E96CF");
		t.accent2 = QStringLiteral("#8B6ECF");
		t.success = QStringLiteral("#6ECF91");
		t.warning = QStringLiteral("#CFAB6E");
		t.danger = QStringLiteral("#CF6E6E");
	}
	else
	{
		t.background = QStringLiteral("#F7F4EF");
		t.surface = QStringLiteral("#FFFDF9");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#FFF7EC");
		t.cardSelected = QStringLiteral("#EEF2FF");
		t.text = QStringLiteral("#28231F");
		t.mutedText = QStringLiteral("#786F67");
		t.border = QStringLiteral("#E9DED1");
		t.graph = QStringLiteral("#FFFAF3");
		t.accent = QStringLiteral("#6190D1");
		t.accent2 = QStringLiteral("#8361D1");
		t.success = QStringLiteral("#61D18A");
		t.warning = QStringLiteral("#D1A861");
		t.danger = QStringLiteral("#D16161");
	}
	finishTokens(t);
	return t;
}

// Constitution: docs/skins/rack.md
SkinTokens rackTokens(bool dark)
{
	SkinTokens t;
	t.dark = dark;
	t.fontFamily = QStringLiteral("DM Sans");
	t.monoFontFamily = QStringLiteral("DM Mono");
	t.borderRadius = 3;
	t.rowHeight = 36;
	t.channelGroupIndent = 16;
	t.channelGroupStyle = SkinTokens::DottedLine;
	t.badgeStyle = SkinTokens::WireframeBorder;
	t.accent = dark ? QStringLiteral("#F4B860") : QStringLiteral("#B66A00");
	t.accent2 = dark ? QStringLiteral("#5ED0A0") : QStringLiteral("#177A55");
	if (dark)
	{
		t.background = QStringLiteral("#0B0D0F");
		t.surface = QStringLiteral("#14181C");
		t.card = QStringLiteral("#1D2328");
		t.cardHover = QStringLiteral("#252B2F");
		t.cardSelected = QStringLiteral("#332718");
		t.text = QStringLiteral("#E6E0D4");
		t.mutedText = QStringLiteral("#9A9488");
		t.border = QStringLiteral("#3A4248");
		t.graph = QStringLiteral("#060807");
		t.graphGridMinor = QStringLiteral("#1F3A31");
	}
	else
	{
		t.background = QStringLiteral("#E7E2D8");
		t.surface = QStringLiteral("#F4EFE5");
		t.card = QStringLiteral("#FFFAEF");
		t.cardHover = QStringLiteral("#F7EEDC");
		t.cardSelected = QStringLiteral("#FCE8BD");
		t.text = QStringLiteral("#2B2721");
		t.mutedText = QStringLiteral("#746A5D");
		t.border = QStringLiteral("#C9BFAE");
		t.graph = QStringLiteral("#FFF7E6");
		t.graphGridMinor = QStringLiteral("#D6C4A6");
	}
	finishTokens(t);
	return t;
}

// Constitution: docs/skins/matrix.md
SkinTokens matrixTokens(bool dark)
{
	SkinTokens t;
	t.dark = dark;
	t.fontFamily = QStringLiteral("DM Sans");
	t.monoFontFamily = QStringLiteral("DM Mono");
	t.borderRadius = 0;
	t.rowHeight = 36;
	t.channelGroupIndent = 24;
	t.channelGroupStyle = SkinTokens::GradientBar;
	t.badgeStyle = SkinTokens::OutlineOnly;
	t.cardRailWidth = 3;
	t.accent = dark ? QStringLiteral("#22D3EE") : QStringLiteral("#008EAA");
	t.accent2 = dark ? QStringLiteral("#7CFFB2") : QStringLiteral("#0A8F57");
	if (dark)
	{
		t.background = QStringLiteral("#060B10");
		t.surface = QStringLiteral("#0B141C");
		t.card = QStringLiteral("#101B25");
		t.cardHover = QStringLiteral("#142432");
		t.cardSelected = QStringLiteral("#082B34");
		t.text = QStringLiteral("#DFF5FF");
		t.mutedText = QStringLiteral("#7FA0AE");
		t.border = QStringLiteral("#233443");
		t.graph = QStringLiteral("#041018");
		t.graphGridMinor = QStringLiteral("#183443");
	}
	else
	{
		t.background = QStringLiteral("#F0F6F8");
		t.surface = QStringLiteral("#FFFFFF");
		t.card = QStringLiteral("#F9FCFD");
		t.cardHover = QStringLiteral("#EDF7FA");
		t.cardSelected = QStringLiteral("#D7F8FF");
		t.text = QStringLiteral("#10242F");
		t.mutedText = QStringLiteral("#5F7782");
		t.border = QStringLiteral("#D4E2E8");
		t.graph = QStringLiteral("#F9FCFD");
		t.graphGridMinor = QStringLiteral("#D4E2E8");
		// Traffic-light status colours tuned for contrast on light surfaces.
		t.success = QStringLiteral("#15803D");
		t.warning = QStringLiteral("#B45309");
		t.danger = QStringLiteral("#DC2626");
	}
	finishTokens(t);
	return t;
}

SkinTokens midnightTokens(bool dark)
{
	SkinTokens t = studioTokens(dark);
	if (dark)
	{
		t.background = QStringLiteral("#040817");
		t.surface = QStringLiteral("#0A1021");
		t.card = QStringLiteral("#10182E");
		t.cardHover = QStringLiteral("#172342");
		t.cardSelected = QStringLiteral("#123B66");
		t.text = QStringLiteral("#EEF5FF");
		t.mutedText = QStringLiteral("#91A6C8");
		t.border = QStringLiteral("#1F3355");
		t.graph = QStringLiteral("#020611");
		t.graphGridMinor = QStringLiteral("#18304E");
		t.accent = QStringLiteral("#3DDCFF");
		t.accent2 = QStringLiteral("#7C6CFF");
		t.success = QStringLiteral("#2BEFA3");
		t.warning = QStringLiteral("#F8C45D");
		t.danger = QStringLiteral("#FF5D73");
	}
	else
	{
		t.background = QStringLiteral("#EAF4FF");
		t.surface = QStringLiteral("#F8FCFF");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#EEF7FF");
		t.cardSelected = QStringLiteral("#CFE9FF");
		t.text = QStringLiteral("#10233A");
		t.mutedText = QStringLiteral("#5F748D");
		t.border = QStringLiteral("#BAD5EF");
		t.graph = QStringLiteral("#F6FBFF");
		t.graphGridMinor = QStringLiteral("#D8E9FA");
		t.accent = QStringLiteral("#0978D8");
		t.accent2 = QStringLiteral("#6159E8");
		t.success = QStringLiteral("#0F9F75");
		t.warning = QStringLiteral("#B7791F");
		t.danger = QStringLiteral("#D93654");
	}
	finishTokens(t);
	return t;
}

SkinTokens arcticTokens(bool dark)
{
	SkinTokens t = softTokens(dark);
	if (dark)
	{
		t.background = QStringLiteral("#071013");
		t.surface = QStringLiteral("#0D1A1F");
		t.card = QStringLiteral("#14272E");
		t.cardHover = QStringLiteral("#1B343D");
		t.cardSelected = QStringLiteral("#164C55");
		t.text = QStringLiteral("#E9FCFF");
		t.mutedText = QStringLiteral("#95B8C2");
		t.border = QStringLiteral("#27444C");
		t.graph = QStringLiteral("#041014");
		t.graphGridMinor = QStringLiteral("#17343C");
		t.accent = QStringLiteral("#62E6F2");
		t.accent2 = QStringLiteral("#8EF7C9");
		t.success = QStringLiteral("#7FF0B8");
		t.warning = QStringLiteral("#FFE08A");
		t.danger = QStringLiteral("#FF8FA3");
	}
	else
	{
		t.background = QStringLiteral("#F0FBFC");
		t.surface = QStringLiteral("#FBFEFF");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#EAF8FA");
		t.cardSelected = QStringLiteral("#D7F4F8");
		t.text = QStringLiteral("#173238");
		t.mutedText = QStringLiteral("#66838B");
		t.border = QStringLiteral("#C8E4EA");
		t.graph = QStringLiteral("#F8FEFF");
		t.graphGridMinor = QStringLiteral("#D7EDF1");
		t.accent = QStringLiteral("#1597A6");
		t.accent2 = QStringLiteral("#30B37D");
		t.success = QStringLiteral("#15985F");
		t.warning = QStringLiteral("#B38A16");
		t.danger = QStringLiteral("#CB4B63");
	}
	finishTokens(t);
	return t;
}

SkinTokens emberTokens(bool dark)
{
	SkinTokens t = rackTokens(dark);
	if (dark)
	{
		t.background = QStringLiteral("#100806");
		t.surface = QStringLiteral("#1A100C");
		t.card = QStringLiteral("#27160F");
		t.cardHover = QStringLiteral("#332016");
		t.cardSelected = QStringLiteral("#4A2812");
		t.text = QStringLiteral("#FFEFE1");
		t.mutedText = QStringLiteral("#B99B84");
		t.border = QStringLiteral("#563121");
		t.graph = QStringLiteral("#0A0503");
		t.graphGridMinor = QStringLiteral("#3A2318");
		t.accent = QStringLiteral("#FF9A3D");
		t.accent2 = QStringLiteral("#FFD166");
		t.success = QStringLiteral("#88D97A");
		t.warning = QStringLiteral("#FFC44D");
		t.danger = QStringLiteral("#FF5A3D");
	}
	else
	{
		t.background = QStringLiteral("#FFF0E4");
		t.surface = QStringLiteral("#FFF9F2");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#FFEBD8");
		t.cardSelected = QStringLiteral("#FFD6A8");
		t.text = QStringLiteral("#372014");
		t.mutedText = QStringLiteral("#866651");
		t.border = QStringLiteral("#E2B98F");
		t.graph = QStringLiteral("#FFF8EE");
		t.graphGridMinor = QStringLiteral("#F0D2B2");
		t.accent = QStringLiteral("#C85A13");
		t.accent2 = QStringLiteral("#9B6A00");
		t.success = QStringLiteral("#2F8A4A");
		t.warning = QStringLiteral("#A86E00");
		t.danger = QStringLiteral("#C93626");
	}
	finishTokens(t);
	return t;
}

SkinTokens violetTokens(bool dark)
{
	SkinTokens t = matrixTokens(dark);
	if (dark)
	{
		t.background = QStringLiteral("#090616");
		t.surface = QStringLiteral("#120D22");
		t.card = QStringLiteral("#1B1431");
		t.cardHover = QStringLiteral("#251C43");
		t.cardSelected = QStringLiteral("#2B1C5A");
		t.text = QStringLiteral("#F4ECFF");
		t.mutedText = QStringLiteral("#AA98C9");
		t.border = QStringLiteral("#3B2B62");
		t.graph = QStringLiteral("#070410");
		t.graphGridMinor = QStringLiteral("#2D2450");
		t.accent = QStringLiteral("#B56CFF");
		t.accent2 = QStringLiteral("#42E8FF");
		t.success = QStringLiteral("#65F0B4");
		t.warning = QStringLiteral("#FFD36A");
		t.danger = QStringLiteral("#FF64A6");
	}
	else
	{
		t.background = QStringLiteral("#F8F1FF");
		t.surface = QStringLiteral("#FFFFFF");
		t.card = QStringLiteral("#FEFBFF");
		t.cardHover = QStringLiteral("#F2E7FF");
		t.cardSelected = QStringLiteral("#E5D5FF");
		t.text = QStringLiteral("#27183D");
		t.mutedText = QStringLiteral("#75648E");
		t.border = QStringLiteral("#D8C7EE");
		t.graph = QStringLiteral("#FFFBFF");
		t.graphGridMinor = QStringLiteral("#E6D8F6");
		t.accent = QStringLiteral("#7B2FD3");
		t.accent2 = QStringLiteral("#008FAB");
		t.success = QStringLiteral("#198754");
		t.warning = QStringLiteral("#A26A00");
		t.danger = QStringLiteral("#C93476");
	}
	finishTokens(t);
	return t;
}

SkinTokens solarTokens(bool dark)
{
	SkinTokens t = minimalTokens(dark);
	if (dark)
	{
		t.background = QStringLiteral("#002B36");
		t.surface = QStringLiteral("#073642");
		t.card = QStringLiteral("#0A3A46");
		t.cardHover = QStringLiteral("#104653");
		t.cardSelected = QStringLiteral("#123F55");
		t.text = QStringLiteral("#EEE8D5");
		t.mutedText = QStringLiteral("#93A1A1");
		t.border = QStringLiteral("#315862");
		t.graph = QStringLiteral("#001F27");
		t.graphGridMinor = QStringLiteral("#16424D");
		t.accent = QStringLiteral("#268BD2");
		t.accent2 = QStringLiteral("#D33682");
		t.success = QStringLiteral("#859900");
		t.warning = QStringLiteral("#B58900");
		t.danger = QStringLiteral("#DC322F");
	}
	else
	{
		t.background = QStringLiteral("#FDF6E3");
		t.surface = QStringLiteral("#FFFBEF");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#F7EFD8");
		t.cardSelected = QStringLiteral("#E9F1D2");
		t.text = QStringLiteral("#073642");
		t.mutedText = QStringLiteral("#657B83");
		t.border = QStringLiteral("#D8CBA6");
		t.graph = QStringLiteral("#FFF8E6");
		t.graphGridMinor = QStringLiteral("#EDE0BD");
		t.accent = QStringLiteral("#268BD2");
		t.accent2 = QStringLiteral("#D33682");
		t.success = QStringLiteral("#6C8500");
		t.warning = QStringLiteral("#A66F00");
		t.danger = QStringLiteral("#CB2D2A");
	}
	finishTokens(t);
	return t;
}

SkinTokens obsidianTokens(bool dark)
{
	SkinTokens t = studioTokens(dark);
	t.borderRadius = 12;
	t.rowHeight = 42;
	t.cardPadding = 14;
	t.cardGap = 10;
	t.graphRadius = 12;
	if (dark)
	{
		t.background = QStringLiteral("#02040A");
		t.surface = QStringLiteral("#070B14");
		t.card = QStringLiteral("#0D1322");
		t.cardHover = QStringLiteral("#141D31");
		t.cardSelected = QStringLiteral("#11284E");
		t.text = QStringLiteral("#F4F8FF");
		t.mutedText = QStringLiteral("#8997B2");
		t.border = QStringLiteral("#202C45");
		t.graph = QStringLiteral("#01030A");
		t.graphGridMinor = QStringLiteral("#17243A");
		t.accent = QStringLiteral("#34D6FF");
		t.accent2 = QStringLiteral("#B66CFF");
		t.success = QStringLiteral("#43F0B0");
		t.warning = QStringLiteral("#FFCF70");
		t.danger = QStringLiteral("#FF5F8C");
	}
	else
	{
		t.background = QStringLiteral("#E9EEF7");
		t.surface = QStringLiteral("#F7FAFF");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#EEF4FC");
		t.cardSelected = QStringLiteral("#D9E7FF");
		t.text = QStringLiteral("#111927");
		t.mutedText = QStringLiteral("#61708A");
		t.border = QStringLiteral("#C0CDDF");
		t.graph = QStringLiteral("#F5F8FD");
		t.graphGridMinor = QStringLiteral("#D9E2F0");
		t.accent = QStringLiteral("#0077B6");
		t.accent2 = QStringLiteral("#7C3BC8");
		t.success = QStringLiteral("#138A67");
		t.warning = QStringLiteral("#A66F00");
		t.danger = QStringLiteral("#C93462");
	}
	finishTokens(t);
	return t;
}

SkinTokens auroraTokens(bool dark)
{
	SkinTokens t = softTokens(dark);
	t.borderRadius = 18;
	t.rowHeight = 50;
	t.cardPadding = 16;
	t.cardGap = 12;
	t.graphRadius = 18;
	if (dark)
	{
		t.background = QStringLiteral("#03110F");
		t.surface = QStringLiteral("#0A1D1B");
		t.card = QStringLiteral("#102B29");
		t.cardHover = QStringLiteral("#173B38");
		t.cardSelected = QStringLiteral("#154E49");
		t.text = QStringLiteral("#ECFFF8");
		t.mutedText = QStringLiteral("#9BBDB4");
		t.border = QStringLiteral("#28514C");
		t.graph = QStringLiteral("#020B0B");
		t.graphGridMinor = QStringLiteral("#163933");
		t.accent = QStringLiteral("#66F0C2");
		t.accent2 = QStringLiteral("#7D8CFF");
		t.success = QStringLiteral("#86F6A8");
		t.warning = QStringLiteral("#FFE08A");
		t.danger = QStringLiteral("#FF8AA6");
	}
	else
	{
		t.background = QStringLiteral("#EFFAF5");
		t.surface = QStringLiteral("#FCFFFD");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#EAF8F2");
		t.cardSelected = QStringLiteral("#D7F4EA");
		t.text = QStringLiteral("#17302C");
		t.mutedText = QStringLiteral("#67837B");
		t.border = QStringLiteral("#CBE2DA");
		t.graph = QStringLiteral("#F8FFFB");
		t.graphGridMinor = QStringLiteral("#DAEEE8");
		t.accent = QStringLiteral("#159873");
		t.accent2 = QStringLiteral("#4F5ECF");
		t.success = QStringLiteral("#178A4D");
		t.warning = QStringLiteral("#B18100");
		t.danger = QStringLiteral("#C94B72");
	}
	finishTokens(t);
	return t;
}

SkinTokens forgeTokens(bool dark)
{
	SkinTokens t = rackTokens(dark);
	t.borderRadius = 5;
	t.rowHeight = 38;
	t.cardPadding = 10;
	t.cardGap = 7;
	t.graphRadius = 6;
	if (dark)
	{
		t.background = QStringLiteral("#090706");
		t.surface = QStringLiteral("#14110E");
		t.card = QStringLiteral("#211A14");
		t.cardHover = QStringLiteral("#2D231A");
		t.cardSelected = QStringLiteral("#4A2B13");
		t.text = QStringLiteral("#F5E7D6");
		t.mutedText = QStringLiteral("#B49D83");
		t.border = QStringLiteral("#51402E");
		t.graph = QStringLiteral("#050403");
		t.graphGridMinor = QStringLiteral("#342B22");
		t.accent = QStringLiteral("#E19A4B");
		t.accent2 = QStringLiteral("#55D6B0");
		t.success = QStringLiteral("#87D66A");
		t.warning = QStringLiteral("#FFC560");
		t.danger = QStringLiteral("#F05F45");
	}
	else
	{
		t.background = QStringLiteral("#F4EEE6");
		t.surface = QStringLiteral("#FFFBF4");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#F7EBDC");
		t.cardSelected = QStringLiteral("#F5D9B4");
		t.text = QStringLiteral("#2B2119");
		t.mutedText = QStringLiteral("#746758");
		t.border = QStringLiteral("#D5C1A9");
		t.graph = QStringLiteral("#FFF8EE");
		t.graphGridMinor = QStringLiteral("#E9D3B8");
		t.accent = QStringLiteral("#A85F16");
		t.accent2 = QStringLiteral("#147A61");
		t.success = QStringLiteral("#2E8044");
		t.warning = QStringLiteral("#9C6500");
		t.danger = QStringLiteral("#B83D2A");
	}
	finishTokens(t);
	return t;
}

SkinTokens nebulaTokens(bool dark)
{
	SkinTokens t = matrixTokens(dark);
	t.rowHeight = 38;
	t.channelGroupIndent = 26;
	t.cardPadding = 13;
	t.cardGap = 10;
	t.cardRailWidth = 5;
	t.graphRadius = 0;
	if (dark)
	{
		t.background = QStringLiteral("#07030F");
		t.surface = QStringLiteral("#100820");
		t.card = QStringLiteral("#1B1032");
		t.cardHover = QStringLiteral("#261848");
		t.cardSelected = QStringLiteral("#351D67");
		t.text = QStringLiteral("#F7EDFF");
		t.mutedText = QStringLiteral("#AA96C8");
		t.border = QStringLiteral("#3C2B62");
		t.graph = QStringLiteral("#05020C");
		t.graphGridMinor = QStringLiteral("#2B2148");
		t.accent = QStringLiteral("#FF5DD8");
		t.accent2 = QStringLiteral("#4EE8FF");
		t.success = QStringLiteral("#67F7B4");
		t.warning = QStringLiteral("#FFD36D");
		t.danger = QStringLiteral("#FF618A");
	}
	else
	{
		t.background = QStringLiteral("#F8F1FF");
		t.surface = QStringLiteral("#FFFFFF");
		t.card = QStringLiteral("#FEFBFF");
		t.cardHover = QStringLiteral("#F2E9FF");
		t.cardSelected = QStringLiteral("#E8D8FF");
		t.text = QStringLiteral("#27153D");
		t.mutedText = QStringLiteral("#76628E");
		t.border = QStringLiteral("#DAC7EE");
		t.graph = QStringLiteral("#FFFBFF");
		t.graphGridMinor = QStringLiteral("#E8D8F6");
		t.accent = QStringLiteral("#C02CA2");
		t.accent2 = QStringLiteral("#008CA6");
		t.success = QStringLiteral("#198754");
		t.warning = QStringLiteral("#A26A00");
		t.danger = QStringLiteral("#C93462");
	}
	finishTokens(t);
	return t;
}

SkinTokens noirTokens(bool dark)
{
	SkinTokens t = minimalTokens(dark);
	t.rowHeight = 34;
	t.cardPadding = 9;
	t.cardGap = 6;
	t.channelGroupIndent = 18;
	t.graphRadius = 0;
	t.zebraStripe = true;
	if (dark)
	{
		t.background = QStringLiteral("#050505");
		t.surface = QStringLiteral("#0C0C0D");
		t.card = QStringLiteral("#151516");
		t.cardHover = QStringLiteral("#202023");
		t.cardSelected = QStringLiteral("#2E3038");
		t.text = QStringLiteral("#F0F0EE");
		t.mutedText = QStringLiteral("#9A9A96");
		t.border = QStringLiteral("#343438");
		t.graph = QStringLiteral("#020202");
		t.graphGridMinor = QStringLiteral("#242428");
		t.accent = QStringLiteral("#D7DEE8");
		t.accent2 = QStringLiteral("#C8A96A");
		t.success = QStringLiteral("#72C27D");
		t.warning = QStringLiteral("#D7A84F");
		t.danger = QStringLiteral("#D86464");
	}
	else
	{
		t.background = QStringLiteral("#EDEDE8");
		t.surface = QStringLiteral("#FAFAF7");
		t.card = QStringLiteral("#FFFFFF");
		t.cardHover = QStringLiteral("#F2F2ED");
		t.cardSelected = QStringLiteral("#E4E8EF");
		t.text = QStringLiteral("#1A1A1A");
		t.mutedText = QStringLiteral("#666663");
		t.border = QStringLiteral("#C9C9C4");
		t.graph = QStringLiteral("#FFFFFF");
		t.graphGridMinor = QStringLiteral("#E2E2DC");
		t.accent = QStringLiteral("#4E5968");
		t.accent2 = QStringLiteral("#8A6A27");
		t.success = QStringLiteral("#2F8048");
		t.warning = QStringLiteral("#8A6500");
		t.danger = QStringLiteral("#A83A3A");
	}
	finishTokens(t);
	return t;
}

struct LegacyPalette
{
	const char* background;
	const char* surface;
	const char* card;
	const char* cardHover;
	const char* cardSelected;
	const char* text;
	const char* mutedText;
	const char* border;
	const char* graph;
	const char* graphGridMinor;
	const char* accent;
	const char* accent2;
	const char* success;
	const char* warning;
	const char* danger;
};

SkinTokens legacyVariantTokens(bool dark, const LegacyPalette& darkPalette, const LegacyPalette& lightPalette)
{
	const LegacyPalette& p = dark ? darkPalette : lightPalette;
	SkinTokens t = minimalTokens(dark);
	t.fontFamily = QStringLiteral("Segoe UI");
	t.monoFontFamily = QStringLiteral("Consolas");
	t.borderRadius = 5;
	t.rowHeight = 36;
	t.channelGroupIndent = 16;
	t.channelGroupStyle = SkinTokens::TreeLines;
	t.badgeStyle = SkinTokens::OutlineOnly;
	t.zebraStripe = false;
	t.background = QLatin1String(p.background);
	t.surface = QLatin1String(p.surface);
	t.card = QLatin1String(p.card);
	t.cardHover = QLatin1String(p.cardHover);
	t.cardSelected = QLatin1String(p.cardSelected);
	t.text = QLatin1String(p.text);
	t.mutedText = QLatin1String(p.mutedText);
	t.border = QLatin1String(p.border);
	t.graph = QLatin1String(p.graph);
	t.graphGridMinor = QLatin1String(p.graphGridMinor);
	t.accent = QLatin1String(p.accent);
	t.accent2 = QLatin1String(p.accent2);
	t.success = QLatin1String(p.success);
	t.warning = QLatin1String(p.warning);
	t.danger = QLatin1String(p.danger);
	finishTokens(t);
	return t;
}

SkinTokens legacySlateTokens(bool dark)
{
	static constexpr LegacyPalette darkPalette{
		"#111418", "#1A1E24", "#242930", "#2C333B", "#354154",
		"#ECEFF4", "#A7B0BD", "#48515D", "#0B0D10", "#303841",
		"#6EA8FE", "#9AA7B8", "#6BCB8F", "#E5B567", "#E06C75"
	};
	static constexpr LegacyPalette lightPalette{
		"#ECEFF3", "#F7F8FA", "#FFFFFF", "#EEF2F6", "#DDE8F8",
		"#1C232D", "#667080", "#C2CAD5", "#FFFFFF", "#D8DEE8",
		"#2F6FD6", "#5E6A7A", "#2E8A57", "#9B6500", "#B33A42"
	};
	return legacyVariantTokens(dark, darkPalette, lightPalette);
}

SkinTokens legacyBlueTokens(bool dark)
{
	static constexpr LegacyPalette darkPalette{
		"#08111C", "#101C2B", "#18283B", "#20344C", "#143B66",
		"#EAF4FF", "#9DB4CC", "#2F4A66", "#050B12", "#1C3348",
		"#48A6FF", "#7CC7FF", "#61D394", "#F1C45F", "#FF6B7A"
	};
	static constexpr LegacyPalette lightPalette{
		"#EAF3FC", "#F7FBFF", "#FFFFFF", "#ECF6FF", "#D7EAFF",
		"#162B40", "#60788E", "#BCD3E8", "#FFFFFF", "#D8E8F6",
		"#1D75C9", "#268FAE", "#218A54", "#A66F00", "#BD3B48"
	};
	return legacyVariantTokens(dark, darkPalette, lightPalette);
}

SkinTokens legacyForestTokens(bool dark)
{
	static constexpr LegacyPalette darkPalette{
		"#0B130E", "#121F17", "#1B2A21", "#25382C", "#1F4A35",
		"#EEF8EF", "#A8B9A9", "#3D5845", "#050B07", "#223A2B",
		"#6BCB8F", "#A1D47A", "#78D88A", "#E3C566", "#E36B66"
	};
	static constexpr LegacyPalette lightPalette{
		"#EEF5EE", "#FAFCF8", "#FFFFFF", "#EFF7EE", "#DDEFDD",
		"#1F3023", "#687A68", "#C3D5C1", "#FFFFFF", "#DCEADB",
		"#2F8A57", "#5B8E36", "#238044", "#9A7600", "#B5443F"
	};
	return legacyVariantTokens(dark, darkPalette, lightPalette);
}

SkinTokens legacyBronzeTokens(bool dark)
{
	static constexpr LegacyPalette darkPalette{
		"#17100A", "#241910", "#322318", "#402E20", "#5A351B",
		"#F8EDE0", "#C2A78A", "#65462F", "#0E0905", "#3F2D20",
		"#C58B48", "#E0B15E", "#87C56F", "#E6BC55", "#E06A4D"
	};
	static constexpr LegacyPalette lightPalette{
		"#F3ECE3", "#FFF9F1", "#FFFFFF", "#F8ECDC", "#F1D8B8",
		"#332317", "#806850", "#D7C0A6", "#FFFFFF", "#E7D2B8",
		"#A86420", "#8B6A2A", "#3F874A", "#9F6F00", "#B84830"
	};
	return legacyVariantTokens(dark, darkPalette, lightPalette);
}

SkinTokens legacyPlumTokens(bool dark)
{
	static constexpr LegacyPalette darkPalette{
		"#140D19", "#201428", "#2B1D36", "#362545", "#442566",
		"#F8ECFF", "#B8A0C6", "#523A61", "#0B0610", "#33243F",
		"#B06BFF", "#E06BB4", "#73DCA0", "#FFD06A", "#FF6F91"
	};
	static constexpr LegacyPalette lightPalette{
		"#F4ECF8", "#FFFAFF", "#FFFFFF", "#F5ECFB", "#E9D9F8",
		"#2D1D38", "#776284", "#D8C5E3", "#FFFFFF", "#E9D8F0",
		"#8046B8", "#B84C88", "#248A57", "#A67000", "#BE3D62"
	};
	return legacyVariantTokens(dark, darkPalette, lightPalette);
}
}

namespace SkinThemeData
{
void registerBundledFonts(bool includeSarasa)
{
	static bool commonAdded = false;
	static bool sarasaAdded = false;
	if (!commonAdded)
	{
		commonAdded = true;
		const QStringList fonts = {
			QStringLiteral(":/fonts/DMSans-Regular.ttf"),
			QStringLiteral(":/fonts/DMSans-Medium.ttf"),
			QStringLiteral(":/fonts/DMSans-SemiBold.ttf"),
			QStringLiteral(":/fonts/DMSans-Bold.ttf"),
			QStringLiteral(":/fonts/DMMono-Regular.ttf"),
			QStringLiteral(":/fonts/DMMono-Medium.ttf"),
			QStringLiteral(":/fonts/Pretendard-Regular.otf"),
			QStringLiteral(":/fonts/Pretendard-Medium.otf"),
			QStringLiteral(":/fonts/Pretendard-SemiBold.otf"),
			QStringLiteral(":/fonts/Pretendard-Bold.otf")
		};
		for (const QString& font : fonts)
			QFontDatabase::addApplicationFont(font);
	}
	if (includeSarasa && !sarasaAdded)
	{
		sarasaAdded = true;
		QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/SarasaMonoK-Regular.ttf"));
		QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/SarasaMonoK-Bold.ttf"));
	}

	const QStringList cjkChain = {
		QStringLiteral("Pretendard"), QStringLiteral("Noto Sans KR"),
		QStringLiteral("Noto Sans"), QStringLiteral("Malgun Gothic"),
		QStringLiteral("Microsoft YaHei")
	};
	QFont::insertSubstitutions(QStringLiteral("DM Sans"), cjkChain);
	QStringList monoChain{ QStringLiteral("Consolas") };
	if (includeSarasa)
		monoChain.append(QStringLiteral("Sarasa Mono K"));
	QFont::insertSubstitutions(QStringLiteral("DM Mono"), monoChain + cjkChain);
}

const QVector<SkinEntry>& roster()
{
	// Display order. Studio is first, and that is load-bearing twice over: it is
	// the default skin and it is what an unknown id falls back to.
	static const QVector<SkinEntry> entries = {
		{ QStringLiteral("studio"), QStringLiteral("studio"), QStringLiteral("studio"), &studioTokens },
		// The minimal skin's sheets are precision_light.qss / precision_dark.qss;
		// the name predates the skin's rename and the files were left alone.
		{ QStringLiteral("minimal"), QStringLiteral("precision"), QStringLiteral("minimal"), &minimalTokens },
		{ QStringLiteral("soft"), QStringLiteral("soft"), QStringLiteral("soft"), &softTokens },
		{ QStringLiteral("rack"), QStringLiteral("rack"), QStringLiteral("rack"), &rackTokens },
		{ QStringLiteral("matrix"), QStringLiteral("matrix"), QStringLiteral("matrix"), &matrixTokens },
		{ QStringLiteral("midnight"), QStringLiteral("studio"), QStringLiteral("studio"), &midnightTokens },
		{ QStringLiteral("arctic"), QStringLiteral("soft"), QStringLiteral("soft"), &arcticTokens },
		{ QStringLiteral("ember"), QStringLiteral("rack"), QStringLiteral("rack"), &emberTokens },
		{ QStringLiteral("violet"), QStringLiteral("matrix"), QStringLiteral("matrix"), &violetTokens },
		{ QStringLiteral("solar"), QStringLiteral("precision"), QStringLiteral("minimal"), &solarTokens },
		{ QStringLiteral("obsidian"), QStringLiteral("studio"), QStringLiteral("studio"), &obsidianTokens },
		{ QStringLiteral("aurora"), QStringLiteral("soft"), QStringLiteral("soft"), &auroraTokens },
		{ QStringLiteral("forge"), QStringLiteral("rack"), QStringLiteral("rack"), &forgeTokens },
		{ QStringLiteral("nebula"), QStringLiteral("matrix"), QStringLiteral("matrix"), &nebulaTokens },
		{ QStringLiteral("noir"), QStringLiteral("precision"), QStringLiteral("minimal"), &noirTokens },
		{ QStringLiteral("legacy-slate"), QStringLiteral("precision"), QStringLiteral("minimal"), &legacySlateTokens },
		{ QStringLiteral("legacy-blue"), QStringLiteral("precision"), QStringLiteral("minimal"), &legacyBlueTokens },
		{ QStringLiteral("legacy-forest"), QStringLiteral("precision"), QStringLiteral("minimal"), &legacyForestTokens },
		{ QStringLiteral("legacy-bronze"), QStringLiteral("precision"), QStringLiteral("minimal"), &legacyBronzeTokens },
		{ QStringLiteral("legacy-plum"), QStringLiteral("precision"), QStringLiteral("minimal"), &legacyPlumTokens },
	};
	return entries;
}

QStringList ids()
{
	QStringList result;
	result.reserve(roster().size());
	for (const SkinEntry& skin : roster())
		result.append(skin.id);
	return result;
}

const SkinEntry& entry(const QString& id)
{
	const QString resolved = resolveId(id);
	for (const SkinEntry& skin : roster())
	{
		if (skin.id == resolved)
			return skin;
	}
	// resolveId only ever returns a roster id, so this is unreachable; returning
	// the first entry rather than asserting keeps the fallback that has always
	// been the behaviour for an id nobody knows.
	return roster().first();
}

QString resolveId(const QString& id)
{
	// The two aliases are stored values from earlier releases, so they cannot be
	// derived from anything - they are the only hard-coded ids here.
	if (id == QStringLiteral("glassy"))
		return QStringLiteral("studio");
	if (id == QStringLiteral("industrial"))
		return QStringLiteral("rack");
	for (const SkinEntry& skin : roster())
	{
		if (skin.id == id)
			return id;
	}
	return roster().first().id;
}

void applyToApplication(QApplication& app, const QString& skinId, bool dark,
	bool setFusionStyle, bool includeSarasa)
{
	const QString resolvedId = resolveId(skinId);
	applyTokensToApplication(app, resolvedId, dark, tokens(resolvedId, dark), setFusionStyle, includeSarasa);
}

void applyTokensToApplication(QApplication& app, const QString& skinId, bool dark,
	const SkinTokens& themeTokens, bool setFusionStyle, bool includeSarasa)
{
	registerBundledFonts(includeSarasa);
	if (setFusionStyle)
		app.setStyle(QStyleFactory::create(QStringLiteral("fusion")));

	const QString resolvedId = resolveId(skinId);
	QString styleSheet;
	QFile sheet(qssResource(resolvedId, dark));
	if (!sheet.open(QFile::ReadOnly) && resolvedId != QLatin1String("studio"))
		sheet.setFileName(qssResource(QStringLiteral("studio"), dark));
	if (sheet.isOpen() || sheet.open(QFile::ReadOnly))
		styleSheet = QString::fromUtf8(sheet.readAll());

	app.setPalette(palette(themeTokens, dark));
	app.setStyleSheet(substituteTokens(styleSheet, themeTokens)
		+ tooltipOverride(themeTokens) + comboArrowOverride() + fileDialogOverride());
}

SkinTokens tokens(const QString& id, bool dark)
{
	SkinTokens result = entry(id).tokens(dark);
	enforceTextReadability(result);
	return result;
}

double contrastRatio(const QString& foreground, const QString& background)
{
	return contrastRatioForColors(QColor(foreground), QColor(background));
}

QVector<ReadabilityCheck> readabilityChecks(const SkinTokens& tokens)
{
	QVector<ReadabilityCheck> checks;
	struct Surface
	{
		const char* token;
		const char* label;
		const QString* color;
	};
	const Surface surfaces[] = {
		{ "background", "window", &tokens.background },
		{ "surface", "input surface", &tokens.surface },
		{ "card", "card", &tokens.card },
		{ "cardHover", "hover card", &tokens.cardHover },
		{ "cardSelected", "selected card", &tokens.cardSelected },
		{ "surfaceSunken", "sunken surface", &tokens.surfaceSunken }
	};
	for (const Surface& surface : surfaces)
	{
		appendReadabilityCheck(checks,
			QStringLiteral("Text on %1").arg(QString::fromLatin1(surface.label)),
			"text", tokens.text, surface.token, *surface.color);
		appendReadabilityCheck(checks,
			QStringLiteral("Muted text on %1").arg(QString::fromLatin1(surface.label)),
			"mutedText", tokens.mutedText, surface.token, *surface.color);
	}
	appendReadabilityCheck(checks, QStringLiteral("Selected text on accent"),
		"selectionText", selectionText(tokens), "accent", tokens.accent);
	return checks;
}

bool passesReadability(const SkinTokens& tokens)
{
	const QVector<ReadabilityCheck> checks = readabilityChecks(tokens);
	for (const ReadabilityCheck& check : checks)
	{
		if (!check.passes())
			return false;
	}
	return !checks.isEmpty();
}

void repairTextReadability(SkinTokens& tokens)
{
	enforceTextReadability(tokens);
}

QString selectionText(const SkinTokens& tokens)
{
	return repairInkForBackground(bestInkForBackground(tokens.accent, tokens), tokens.accent);
}

bool modesAreDistinct(const SkinTokens& light, const SkinTokens& dark)
{
	if (light.dark || !dark.dark)
		return false;

	const QColor lightBackground(light.background);
	const QColor darkBackground(dark.background);
	if (!lightBackground.isValid() || !darkBackground.isValid())
		return false;

	// The mode split is deliberately broad: a light window must read as light
	// and a dark window must read as dark even before its accent is noticed.
	return relativeLuminance(lightBackground) >= 0.55
		&& relativeLuminance(darkBackground) <= 0.20
		&& relativeLuminance(lightBackground) - relativeLuminance(darkBackground) >= 0.45;
}

QString tooltipOverride(const SkinTokens& tokens)
{
	return QStringLiteral(
		"QToolTip { background: %1; color: %2; border: 1px solid %3; }")
		.arg(tokens.card, tokens.text, tokens.border);
}

QString qssResource(const QString& id, bool dark)
{
	return QStringLiteral(":/skins/%1_%2.qss")
		.arg(entry(id).qssBaseName, dark ? QStringLiteral("dark") : QStringLiteral("light"));
}

QString substituteTokens(QString qss, const SkinTokens& tokens)
{
	// Token sentinels intentionally use the @TOKEN@ form so they survive Qt's
	// style sheet parser intact (a literal '@' is not meaningful in QSS) and
	// stand out in the source files. Order does not matter because every
	// sentinel is unique.
	struct Substitution { const char* placeholder = nullptr; QString value; };
	const Substitution table[] = {
		{ "@BG@", tokens.background },
		{ "@SURFACE@", tokens.surface },
		{ "@SURFACE_RAISED@", tokens.surfaceRaised },
		{ "@SURFACE_SUNKEN@", tokens.surfaceSunken },
		{ "@CARD@", tokens.card },
		{ "@CARD_HOVER@", tokens.cardHover },
		{ "@CARD_SELECTED@", tokens.cardSelected },
		{ "@TEXT@", tokens.text },
		{ "@SELECTION_TEXT@", selectionText(tokens) },
		{ "@MUTED@", tokens.mutedText },
		{ "@BORDER@", tokens.border },
		{ "@GRAPH@", tokens.graph },
		{ "@GRID_MAJOR@", tokens.graphGridMajor },
		{ "@GRID_MINOR@", tokens.graphGridMinor },
		{ "@ACCENT@", tokens.accent },
		{ "@ACCENT2@", tokens.accent2 },
		{ "@SUCCESS@", tokens.success },
		{ "@WARNING@", tokens.warning },
		{ "@DANGER@", tokens.danger },
		{ "@FOCUS@", tokens.focusRing },
		{ "@FONT@", tokens.fontFamily },
		{ "@MONO@", tokens.monoFontFamily }
	};
	for (const Substitution& s : table)
	{
		// The alpha form first, because "@ACCENT@" is a prefix of "@ACCENT_RGB@"
		// and replacing the shorter one first would leave "#7AA2F7_RGB".
		//
		// QSS has no variables and its rgba() takes three numbers, so a sheet that
		// wanted a token at 30% had to write the palette value out by hand - and
		// then a token change did not reach it. With this it can say
		// rgba(@ACCENT_RGB@, 0.30) instead.
		//
		// Only colour tokens get the form; a font family has no channels. A value
		// that does not parse as a colour is skipped rather than emitting
		// something that would make the whole rule invalid.
		const QColor color(s.value);
		if (color.isValid())
		{
			qss.replace(QLatin1String(s.placeholder).left(int(qstrlen(s.placeholder)) - 1) + QLatin1String("_RGB@"),
				QStringLiteral("%1, %2, %3").arg(color.red()).arg(color.green()).arg(color.blue()));
		}
		qss.replace(QLatin1String(s.placeholder), s.value);
	}
	return qss;
}

QString comboArrowOverride()
{
	return QStringLiteral(
		"QComboBox::down-arrow,"
		"QComboBox[paramSelector=\"true\"]::down-arrow,"
		"QComboBox[filterSelector=\"true\"]::down-arrow {"
		" image: url(:/icons/modern/chevron-down.svg); width: 12px; height: 12px;"
		" border: none; background: transparent; }"
		"QAbstractSpinBox::up-arrow {"
		" image: url(:/icons/modern/chevron-up.svg); width: 12px; height: 12px;"
		" border: none; background: transparent; }"
		"QAbstractSpinBox::down-arrow {"
		" image: url(:/icons/modern/chevron-down.svg); width: 12px; height: 12px;"
		" border: none; background: transparent; }");
}

QString fileDialogOverride()
{
	return QStringLiteral(
		"QFileDialog QToolButton {"
		" padding: 2px; min-height: 0px; min-width: 0px; }");
}

QPalette palette(const SkinTokens& tokens, bool dark)
{
	QPalette palette;
	QColor background(tokens.background);
	QColor surface(tokens.surface);
	QColor card(tokens.card);
	QColor text(tokens.text);
	QColor accent(tokens.accent);
	palette.setColor(QPalette::Window, background);
	palette.setColor(QPalette::WindowText, text);
	palette.setColor(QPalette::Base, surface);
	palette.setColor(QPalette::AlternateBase, card);
	palette.setColor(QPalette::Text, text);
	palette.setColor(QPalette::Button, card);
	palette.setColor(QPalette::ButtonText, text);
	palette.setColor(QPalette::ToolTipBase, card);
	palette.setColor(QPalette::ToolTipText, text);
	palette.setColor(QPalette::Highlight, accent);
	palette.setColor(QPalette::HighlightedText, QColor(selectionText(tokens)));
	palette.setColor(QPalette::PlaceholderText, QColor(tokens.mutedText));
	palette.setColor(QPalette::Light, card.lighter(120));
	palette.setColor(QPalette::Midlight, card.lighter(105));
	palette.setColor(QPalette::Mid, surface);
	palette.setColor(QPalette::Dark, background.darker(120));
	palette.setColor(QPalette::Shadow, background.darker(160));
	palette.setColor(QPalette::Link, accent);
	palette.setColor(QPalette::LinkVisited, accent.darker(110));
	return palette;
}
}
