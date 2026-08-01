/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.

	Mechanical paint helpers shared by the skin TUs (skins, pickers,
	reference cards, routing renderers). Nothing here carries a design
	decision, so sharing them cannot breach the differentiation gate - a
	skin's grammar lives in what it draws, not in how a QColor gets its
	alpha. Additions must stay design-free; per-skin recipes are only
	admitted when several TUs of the SAME skin need them and are marked as
	off-limits to the neighbours.

	Header-only on purpose: SkinThemeData/DeviceSelector-style satellite
	consumers can include it without linking anything.
*/

#pragma once

#include <QColor>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QVector>
#include <QtMath>

#include "Editor/SkinTokens.h"

// House rule for paint-only chrome: its paintEvent owns every background
// pixel, so framework/QSS polishing must never stamp an opaque background
// before or after that event. Per-sheet transparent rules may decorate the
// widget but are not the load-bearing defense.
inline void configurePaintOnlyChrome(QWidget* widget)
{
	widget->setAttribute(Qt::WA_NoSystemBackground, true);
}

inline bool skinColorIsDark(const QColor& color)
{
	return color.lightness() < 128;
}

inline bool skinIsDark(const SkinTokens& tokens)
{
	return tokens.dark;
}

inline QColor withAlpha(QColor color, int alpha)
{
	color.setAlpha(alpha);
	return color;
}

inline QColor withAlpha(const QString& hex, int alpha)
{
	return withAlpha(QColor(hex), alpha);
}

inline QColor withAlphaF(QColor color, double alpha)
{
	color.setAlphaF(alpha);
	return color;
}

// Fixed paint material effects matching QSS @SHADOW_Axx@ / @HIGHLIGHT_Axx@.
// These are not semantic palette colours; use them for bevels, recesses,
// sheens and other black/white lighting math.
inline QColor skinMaterialShadow(int alpha = 255)
{
	return QColor(0, 0, 0, alpha);
}

inline QColor skinMaterialHighlight(int alpha = 255)
{
	return QColor(255, 255, 255, alpha);
}

// Linear blend between two colours; t = 0 returns a, t = 1 returns b. The
// skins fake elevation steps and pastel arcs by mixing token colours instead
// of introducing palette entries.
inline QColor mixColor(const QColor& a, const QColor& b, double t)
{
	return QColor(
		qRound(a.red() + (b.red() - a.red()) * t),
		qRound(a.green() + (b.green() - a.green()) * t),
		qRound(a.blue() + (b.blue() - a.blue()) * t));
}

// QSS colour strings for inline rules built from token colours.
inline QString cssRgba(const QColor& color, double alpha)
{
	return QStringLiteral("rgba(%1, %2, %3, %4)")
		.arg(color.red()).arg(color.green()).arg(color.blue())
		.arg(alpha, 0, 'f', 2);
}

inline QString cssRgba(const QString& hex, double alpha)
{
	return cssRgba(QColor(hex), alpha);
}

inline QString cssColor(const QColor& color)
{
	return color.name(color.alpha() < 255 ? QColor::HexArgb : QColor::HexRgb);
}

// Screen point on a circle around center. Qt-style angles: counter-clockwise
// from 3 o'clock, and screen Y grows downward, so sin is subtracted. Pass the
// negated clockwise sweep angle.
inline QPointF skinArcPoint(const QPointF& center, double radius, double degrees)
{
	const double radians = qDegreesToRadians(degrees);
	return QPointF(center.x() + qCos(radians) * radius, center.y() - qSin(radians) * radius);
}

// Mechanical layout facts for the left scope gutter. Skins decide what each
// lane looks like; this object only answers where channel-group lanes, If
// lanes, branch stations and the row face sit. The row model already computed
// depth and logicDepth from neighbouring lines, so paint code must not
// rediscover that structure differently in each skin.
struct SkinScopeGutterLayout
{
	bool ifFamily = false;
	bool headRow = false;
	bool branchRow = false;
	bool tailRow = false;
	bool branchOrTail = false;
	bool shouldPaint = false;
	int logic = 0;
	int unit = 0;
	int height = 0;
	int junctionY = 0;
	double junctionYF = 0.0;
	int indentUnits = 0;
	int cardLeft = 0;
	int ifLevels = 0;
	int channelLevels = 0;
	int ownLevel = -1;

	int bandCenter(int level) const
	{
		return 8 + level * unit + unit / 2;
	}

	double bandCenterF(int level) const
	{
		return 8.0 + level * unit + unit / 2.0;
	}
};

inline SkinScopeGutterLayout skinScopeGutterLayout(const QString& type, const QString& command,
	int depth, int logicDepth, const SkinTokens& tokens, const QSize& size)
{
	SkinScopeGutterLayout layout;
	layout.ifFamily = type == QStringLiteral("if");
	layout.headRow = layout.ifFamily && command == QStringLiteral("if");
	layout.branchRow = layout.ifFamily
		&& (command == QStringLiteral("elseif") || command == QStringLiteral("else"));
	layout.tailRow = layout.ifFamily && command == QStringLiteral("endif");
	layout.branchOrTail = layout.branchRow || layout.tailRow;
	layout.logic = logicDepth;
	layout.shouldPaint = layout.headRow || layout.logic > 0;
	layout.unit = tokens.channelGroupIndent;
	layout.height = size.height();
	layout.junctionY = 4 + tokens.rowHeight / 2;
	layout.junctionYF = 4.0 + tokens.rowHeight / 2.0;

	// Branch/tail rows are visually mounted with block members so the If lane
	// passes their face instead of stopping behind a full-width command row.
	layout.indentUnits = (layout.ifFamily && !layout.headRow) ? depth + 1 : depth;
	layout.cardLeft = 8 + layout.indentUnits * layout.unit;
	layout.ifLevels = layout.headRow
		? layout.logic
		: (layout.branchOrTail ? layout.logic - 1 : layout.logic);
	layout.channelLevels = qMax(0, layout.indentUnits - layout.ifLevels - (layout.branchOrTail ? 1 : 0));
	layout.ownLevel = layout.headRow
		? layout.channelLevels + layout.logic
		: layout.channelLevels + layout.logic - 1;
	return layout;
}

struct SkinAxisLabelRect
{
	QRect rect;
	int alignment = Qt::AlignHCenter;
};

// Mechanical layout facts for analysis-style graphs. EqGraphView and
// GraphicEQPlotWidget own the data-to-pixel mapping; skins still decide the
// material language, but they should not each invent label/cursor rectangles
// and grid thinning from scratch.
struct SkinAnalysisGraphLayout
{
	QRect rect;
	QRectF plot;
	double zeroY = 0.0;
	double zeroClamped = 0.0;
	double hover = 0.0;

	QRect plotRect() const
	{
		return plot.toRect();
	}

	int plotLeft() const
	{
		return int(plot.left());
	}

	int plotRight() const
	{
		return int(plot.right());
	}

	int plotTop() const
	{
		return int(plot.top());
	}

	int plotBottom() const
	{
		return int(plot.bottom());
	}

	int zeroRow() const
	{
		return int(zeroY);
	}

	QRect truncatedXAxisLabelRect(double centerX, int yOffset, int width, int height) const
	{
		return QRect(int(centerX) - width / 2, int(plot.bottom()) + yOffset, width, height);
	}

	QRect roundedXAxisLabelRect(double centerX, int yOffset, int width, int height) const
	{
		return QRect(qRound(centerX) - width / 2, int(plot.bottom()) + yOffset, width, height);
	}

	SkinAxisLabelRect clampedRoundedXAxisLabelRect(double centerX, int yOffset,
		int width, int height, int edgeInset) const
	{
		SkinAxisLabelRect label;
		label.rect = roundedXAxisLabelRect(centerX, yOffset, width, height);
		if (label.rect.right() > rect.right() - edgeInset)
		{
			label.rect.setRight(rect.right() - edgeInset);
			label.alignment = Qt::AlignRight;
		}
		if (label.rect.left() < rect.left() + edgeInset)
		{
			label.rect.setLeft(rect.left() + edgeInset);
			label.alignment = Qt::AlignLeft;
		}
		return label;
	}

	QRect centeredRectClampedToX(int centerX, int top, int width, int height,
		int minLeft, int maxLeft) const
	{
		return QRect(qBound(minLeft, centerX - width / 2, maxLeft), top, width, height);
	}

	QRectF footerRectF(double yOffset, double height) const
	{
		return QRectF(plot.left(), plot.bottom() + yOffset, plot.width(), height);
	}

	QRectF leftPlotLabelRectF(double xOffset, double top, double width, double height) const
	{
		return QRectF(plot.left() + xOffset, top, width, height);
	}
};

inline SkinAnalysisGraphLayout skinAnalysisGraphLayout(const QRect& rect, const QRectF& plotRect,
	double zeroY, double hover)
{
	SkinAnalysisGraphLayout layout;
	layout.rect = rect;
	layout.plot = plotRect;
	layout.zeroY = zeroY;
	layout.zeroClamped = qBound(plotRect.top(), zeroY, plotRect.bottom());
	layout.hover = qBound(0.0, hover, 1.0);
	return layout;
}

template<typename GridLines>
inline double skinMinimumAdjacentGridGap(const GridLines& lines, double fallback = 0.0)
{
	if (lines.size() < 2)
		return fallback;
	double gap = fallback;
	for (int i = 1; i < lines.size(); i++)
	{
		const double current = qAbs(lines.at(i).pos - lines.at(i - 1).pos);
		gap = (i == 1 && fallback == 0.0) ? current : qMin(gap, current);
	}
	return gap;
}

template<typename GridLines>
inline int skinFirstMajorGridIndex(const GridLines& lines)
{
	for (int i = 0; i < lines.size(); i++)
	{
		if (lines.at(i).major)
			return i;
	}
	return 0;
}

inline int skinLabelStrideForGap(double gap, double minimumGap)
{
	return gap > 0.5 ? qMax(1, qCeil(minimumGap / gap)) : 1;
}

// Soft's pastel-shelf recipe (constitution-cited: "파스텔은 토큰 혼합으로").
// Shared here only because SoftSkin and SoftReferenceCardView both need it;
// the other four skins must not adopt it - pastel derivation IS Soft's
// grammar (differentiation gate).
inline QColor softPastelize(const QColor& base, bool dark)
{
	const double hue = base.hslHueF() < 0.0 ? 215.0 / 360.0 : base.hslHueF();
	const double saturation = qMin(base.hslSaturationF(), dark ? 0.50 : 0.55);
	return QColor::fromHslF(hue, saturation, dark ? 0.62 : 0.60);
}
