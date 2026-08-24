/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.

	The data half of the skin system: id aliases, per-skin colour/metric token
	tables, QSS resource paths, the @TOKEN@ substitution and the token-derived
	widget palette. Everything here is behaviour-free (no pickers, renderers or
	chrome painters), so satellite executables - first user: DeviceSelector -
	can compile this single unit plus the .qss resources and wear the exact
	skin the user picked in the Editor, without linking the Editor's widget
	stack. The full ISkin classes delegate their tokens()/qssResource() here,
	so the tables cannot drift apart.
*/

#pragma once

#include <QPalette>
#include <QString>
#include <QStringList>
#include <QVector>

#include "Editor/SkinTokens.h"

class QApplication;

namespace SkinThemeData
{
// THE SKIN ROSTER. The one list of which skins exist.
//
// Adding a skin used to mean editing eighteen places, and missing one did not
// fail: resolveId() falls back to "studio" for an id it does not know, so a new
// skin that compiled, registered and appeared in the menu was drawn as Studio
// with no error anywhere. Everything that needs to know the membership - the
// token lookup, the QSS path, the Editor's ISkin instances, Device Selector's
// painters and shot harness, the tests - reads it from here now.
//
// The display name is deliberately absent: it is translated, and this unit is
// compiled into satellite tools that install no translators. The Editor's menu
// keeps that table and looks names up by id, so a roster id with no name shows as
// its raw id rather than vanishing from the menu.
struct SkinEntry
{
	// As stored in the registry and written in a config. Never translated.
	QString id;
	// Base name of the .qss pair, which is not always the id: the minimal skin's
	// sheets keep their original precision_* names (docs/skins/minimal.md).
	QString qssBaseName;
	// Skin id whose custom painters implement this theme's form language. Token
	// variants keep their own id and colours while reusing a shipped skin's
	// widget grammar.
	QString paintBaseId;
	// The skin's token table. Every skin builds both modes from one function,
	// so the roster carries one pointer rather than a light/dark pair.
	SkinTokens (*tokens)(bool dark) = nullptr;
};

// A named foreground/background pair used by Theme Lab and the non-widget
// logic tests. Every item is ordinary, continuously-read text, so the
// threshold is WCAG's 4.5:1 normal-text floor rather than a decorative-control
// contrast target.
struct ReadabilityCheck
{
	QString label;
	QString foregroundToken;
	QString backgroundToken;
	double ratio = 0.0;
	double minimum = 4.5;

	bool passes() const { return ratio >= minimum; }
};

// Every built-in skin, in display order.
const QVector<SkinEntry>& roster();
// Just the ids, in the same order.
QStringList ids();
// The entry for a stored id, alias-resolved. Never null: an unknown id resolves
// to the first entry, the way resolveId() has always fallen back to Studio.
const SkinEntry& entry(const QString& id);

// Registers the shared static font faces and fallback chain. Editor passes
// includeSarasa=true for its monospace CJK surfaces; satellite tools keep the
// smaller common set.
void registerBundledFonts(bool includeSarasa = false);

// Applies the complete process theme contract: optional Fusion base style,
// token palette, QSS with Studio fallback, and common widget overrides.
void applyToApplication(QApplication& app, const QString& skinId, bool dark,
	bool setFusionStyle = true, bool includeSarasa = false);

// Applies a built-in skin's QSS grammar with an explicit token table. Theme
// Lab uses this for saved themes and temporary previews without adding a
// second stylesheet path outside the shared skin contract.
void applyTokensToApplication(QApplication& app, const QString& skinId, bool dark,
	const SkinTokens& themeTokens, bool setFusionStyle = true, bool includeSarasa = false);

// Canonical skin id for any stored value: applies the legacy aliases
// (glassy -> studio, industrial -> rack) and falls back to "studio" for
// unknown ids, mirroring Skins::byId.
QString resolveId(const QString& id);

// The token table for a (resolved or unresolved) skin id.
SkinTokens tokens(const QString& id, bool dark);

// WCAG contrast ratio for two opaque #RRGGBB values. Invalid colours return
// zero so imported/custom theme errors cannot accidentally pass an audit.
double contrastRatio(const QString& foreground, const QString& background);

// The text-on-surface contract shared by every built-in and custom theme.
// Theme Lab shows these rows live; the logic suite pins all built-in pairs.
QVector<ReadabilityCheck> readabilityChecks(const SkinTokens& tokens);
bool passesReadability(const SkinTokens& tokens);

// Ink selected from the existing palette tokens for legible text over the
// accent selection fill. Keeping this derived avoids a hidden black/white
// fallback that would drift from a custom palette.
QString selectionText(const SkinTokens& tokens);

// Adjust only the primary and muted ink lightness, preserving their hue, until
// they meet the shared readability contract where a single ink can do so.
// Theme Lab offers this as an explicit one-click repair for custom palettes.
void repairTextReadability(SkinTokens& tokens);

// A light/dark pair must have the expected mode flag and a materially
// different window luminance. This prevents a cosmetic checkbox from silently
// producing two near-identical presentations.
bool modesAreDistinct(const SkinTokens& light, const SkinTokens& dark);

// A token-driven fallback appended after each modern sheet and used directly
// by the heritage layer. Keeping this in the shared theme contract prevents a
// system tooltip from falling back to the platform's light defaults.
QString tooltipOverride(const SkinTokens& tokens);

// The ":/skins/..." QSS resource path for the id, honouring the historical
// precision_* file names of the minimal skin.
QString qssResource(const QString& id, bool dark);

// Replaces the @TOKEN@ sentinels of a skin sheet with the token values.
//
// Every colour token also answers to an @TOKEN_RGB@ form, which expands to the
// three channels without the "#": a sheet writes rgba(@ACCENT_RGB@, 0.30) where
// it previously had to spell the palette value out by hand, which meant a token
// change did not reach it. QSS has no variables and its rgba() wants numbers, so
// this is the only way a sheet can hold a token at partial alpha.
QString substituteTokens(QString qss, const SkinTokens& tokens);

// Token-derived QPalette for the widgets QSS does not cover (item views,
// native popups). The same mapping SkinManager::applySkin applies in the
// Editor on every skin/dark switch.
QPalette palette(const SkinTokens& tokens, bool dark);

// App-wide combo/spin arrow override appended AFTER a skin sheet: the CSS
// border-triangle trick every sheet uses collapses to a dash on Qt 6.10, so
// the arrows are replaced with a real chevron SVG. Needs the
// :/icons/modern/chevron-*.svg resources.
QString comboArrowOverride();

// App-wide file-dialog override appended AFTER a skin sheet, like the combo
// arrows: the non-native QFileDialog's navigation buttons are icon-only, so
// every sheet's text-button QToolButton padding (up to 5px 12px on soft)
// squeezes the icon out of its content box. Scoped to QFileDialog so the
// skins' regular tool buttons keep their padding language.
QString fileDialogOverride();
}
