#include "SkinManager.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QSettings>
#include <QWidget>

#include "services/logging/Logging.h"
#include "services/registry/RegistryPaths.h"
#include "Editor/helpers/CrashHandler.h"
#include "skins/CustomThemeStore.h"
#include "skins/ISkin.h"
#include "skins/Skins.h"
#include "skins/SkinThemeData.h"

namespace
{
QString heritageStyleSheet(const SkinTokens& tokens)
{
	const QString selectedInk = SkinThemeData::selectionText(tokens);
	return QStringLiteral(
		"QMainWindow, QWidget#centralWidget, QWidget#WindowChromeHost {"
		" background: %1; color: %7; font-family: \"%12\"; }"
		"QWidget#AppTitleBar {"
		" background: %2; border-bottom: 1px solid %10; }"
		"QLabel#TitleBarText { color: %7; font-weight: 600; }"
		"QMenuBar { background: %2; color: %7; border-bottom: 1px solid %10; }"
		"QMenuBar::item { background: transparent; padding: 4px 8px; }"
		"QMenuBar::item:selected { background: %4; color: %7; }"
		"QMenu { background: %3; color: %7; border: 1px solid %10; }"
		"QMenu::item { padding: 4px 22px 4px 20px; }"
		"QMenu::item:selected { background: %5; color: %7; }"
		"QMenu::separator { height: 1px; background: %10; margin: 4px 8px; }"
		"QDialog, QMessageBox { background: %1; color: %7; font-family: \"%12\"; }"
		"QMessageBox QLabel, QDialog QLabel { color: %7; background: transparent; }"
		"QDialogButtonBox { background: %1; }"
		"QToolBar { background: %2; border: 0; border-bottom: 1px solid %10; spacing: 3px; }"
		"QToolButton { background: transparent; color: %7; border: 1px solid transparent; padding: 2px 4px; }"
		"QToolButton:hover { background: %4; border-color: %10; }"
		"QToolButton:pressed, QToolButton:checked { background: %5; border-color: %9; }"
		"QTabWidget::pane { background: %1; border-top: 1px solid %10; }"
		"QTabBar::tab { background: %2; color: %8; border: 1px solid %10; padding: 4px 10px; }"
		"QTabBar::tab:selected { background: %3; color: %7; border-bottom-color: %3; }"
		"QTabBar::tab:hover { background: %4; color: %7; }"
		"QDockWidget { background: %1; color: %7; titlebar-close-icon: none; titlebar-normal-icon: none; }"
		"QDockWidget::title { background: %2; color: %7; padding: 4px 6px; border-top: 1px solid %10; }"
		"QWidget#dockWidgetContents, QWidget#analysisControlBar { background: %1; color: %7; }"
		"QFrame#AnalysisStatChip { background: %3; border: 1px solid %10; border-radius: 3px; }"
		"QLabel#AnalysisFormLabel, QLabel#AnalysisStatLabel { color: %8; }"
		"QLabel#AnalysisStatValue { color: %7; font-weight: 600; }"
		"QScrollArea, QAbstractScrollArea, QGraphicsView { background: %1; color: %7; border: 1px solid %10; }"
		"QScrollArea > QWidget > QWidget { background: %1; }"
		"QLabel { color: %7; background: transparent; }"
		"QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {"
		" background: %6; color: %7; border: 1px solid %10; selection-background-color: %9;"
		" selection-color: %11; padding: 2px 4px; }"
		"QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled, QComboBox:disabled,"
		" QSpinBox:disabled, QDoubleSpinBox:disabled { background: %2; color: %8; }"
		"QPushButton { background: %3; color: %7; border: 1px solid %10; padding: 3px 8px; }"
		"QPushButton:hover { background: %4; border-color: %9; }"
		"QPushButton:pressed { background: %5; }"
		"QPushButton:disabled, QToolButton:disabled, QLabel:disabled { color: %8; }"
		"QCheckBox, QRadioButton { color: %7; background: transparent; }"
		"QHeaderView::section { background: %2; color: %7; border: 1px solid %10; padding: 3px; }"
		"QTableView, QTreeView, QListView { background: %6; color: %7; alternate-background-color: %2;"
		" gridline-color: %10; selection-background-color: %5; selection-color: %7; }"
		"QSplitter::handle { background: %10; }"
		"QStatusBar { background: %2; color: %8; }")
		.arg(tokens.background,
			tokens.surface,
			tokens.card,
			tokens.cardHover,
			tokens.cardSelected,
			tokens.surfaceSunken,
			tokens.text,
			tokens.mutedText,
			tokens.accent,
			tokens.border,
			selectedInk,
			tokens.fontFamily)
		+ SkinThemeData::tooltipOverride(tokens)
		+ SkinThemeData::toolButtonMenuOverride(tokens);
}
}

SkinManager::SkinManager(QObject* parent)
	: QObject(parent)
{
	// Establishes the never-null invariant on activeSkin (see the header).
	activeSkin = Skins::byId(skinId);
	Q_ASSERT(activeSkin != nullptr);
	renderSkinId = SkinThemeData::entry(activeSkin->id()).paintBaseId;
	skinId = activeSkin->id();
	currentTokens = activeSkin->tokens(darkMode);
}

SkinManager* SkinManager::instance()
{
	static SkinManager manager;
	return &manager;
}

const SkinTokens& SkinManager::tokens() const
{
	return currentTokens;
}

const QString& SkinManager::currentSkinId() const
{
	return skinId;
}

const QString& SkinManager::baseSkinId() const
{
	return renderSkinId;
}

bool SkinManager::isDark() const
{
	return darkMode;
}

bool SkinManager::isHeritage() const
{
	return heritageMode;
}

void SkinManager::applyHeritage(const QString& newSkinId, bool dark)
{
	CrashHandler::setBreadcrumb(L"applyHeritage (legacy rows)");
	LogFStatic(L"Applying heritage presentation (legacy rows)");

	heritageMode = true;
	previewMode = false;
	CustomThemeStore::Theme customTheme;
	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	if (CustomThemeStore::findTheme(settings, newSkinId, &customTheme))
	{
		const SkinTokens customTokens = CustomThemeStore::tokensForTheme(customTheme);
		activeSkin = Skins::byId(customTheme.baseTheme);
		renderSkinId = SkinThemeData::entry(activeSkin->id()).paintBaseId;
		darkMode = customTheme.dark;
		if (SkinThemeData::passesReadability(customTokens))
		{
			skinId = customTheme.skinId();
			currentTokens = customTokens;
		}
		else
		{
			LogFStatic(L"Ignoring unreadable saved custom theme %s", reinterpret_cast<const wchar_t*>(customTheme.skinId().utf16()));
			skinId = activeSkin->id();
			currentTokens = activeSkin->tokens(darkMode);
		}
	}
	else
	{
		activeSkin = Skins::byId(newSkinId);
		renderSkinId = SkinThemeData::entry(activeSkin->id()).paintBaseId;
		skinId = activeSkin->id();
		darkMode = dark;
		currentTokens = activeSkin->tokens(darkMode);
	}

	// Legacy rows keep their original widgets and factory chain. Only the Qt
	// palette/QSS layer changes, so graph, menu, tab and toolbar chrome no
	// longer stay light while the row list is dark.
	currentTokens.fontFamily = QStringLiteral("Segoe UI");
	currentTokens.monoFontFamily = QStringLiteral("Consolas");
	SkinThemeData::registerBundledFonts(false);
	qApp->setPalette(SkinThemeData::palette(currentTokens, darkMode));
	qApp->setStyleSheet(heritageStyleSheet(currentTokens));
	sheetApplied = true;

	emit skinChanged(currentTokens);
	for (QWidget* widget : qApp->allWidgets())
		widget->update();

	LogFStatic(L"Heritage presentation applied");
}

// The @TOKEN@ substitution lives in SkinThemeData::substituteTokens so
// satellite tools (DeviceSelector) dress the same sheets identically.

void SkinManager::applySkin(const QString& newSkinId, bool dark)
{
	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	CustomThemeStore::Theme customTheme;
	const bool isCustomTheme = CustomThemeStore::findTheme(settings, newSkinId, &customTheme);
	const SkinTokens customTokens = isCustomTheme
		? CustomThemeStore::tokensForTheme(customTheme) : SkinTokens();
	const bool useCustomTheme = isCustomTheme && SkinThemeData::passesReadability(customTokens);
	const QString fallbackSkinId = isCustomTheme ? customTheme.baseTheme : newSkinId;
	const QString targetSkinId = useCustomTheme ? customTheme.skinId() : Skins::byId(fallbackSkinId)->id();
	const bool targetDark = isCustomTheme ? customTheme.dark : dark;

	// Re-dressing the app with the identical sheet is not free: Qt re-resolves
	// the stylesheet against every live widget. At startup this used to run
	// three times (main(), loadPreferences() before and after the open-files
	// restore); the post-restore pass alone re-polished every filter card and
	// took seconds on a large config.
	if (sheetApplied && !previewMode && !heritageMode && darkMode == targetDark
		&& targetSkinId == skinId)
	{
		LogFStatic(L"Skin %s (dark=%d) already active, skipping re-apply",
			reinterpret_cast<const wchar_t*>(skinId.utf16()), targetDark ? 1 : 0);
		return;
	}

	heritageMode = false;
	previewMode = false;
	// Breadcrumb + unconditional log line: a skin-switch crash reported from
	// the field must identify the dying skin in the crash report and in
	// %TEMP%\EqualizerAPO.log.
	CrashHandler::setBreadcrumb(QStringLiteral("applySkin %1 dark=%2").arg(targetSkinId).arg(targetDark).toStdWString());
	LogFStatic(L"Applying skin %s (dark=%d)", reinterpret_cast<const wchar_t*>(targetSkinId.utf16()), targetDark ? 1 : 0);

	// Skins::byId applies legacy aliases (glassy->studio, industrial->rack) and
	// falls back to the studio skin for unknown ids.
	if (useCustomTheme)
	{
		activeSkin = Skins::byId(customTheme.baseTheme);
		renderSkinId = SkinThemeData::entry(activeSkin->id()).paintBaseId;
		skinId = customTheme.skinId();
		darkMode = customTheme.dark;
		currentTokens = customTokens;
		SkinThemeData::applyTokensToApplication(*qApp, activeSkin->id(), darkMode,
			currentTokens, false, true);
	}
	else
	{
		if (isCustomTheme)
			LogFStatic(L"Ignoring unreadable saved custom theme %s", reinterpret_cast<const wchar_t*>(customTheme.skinId().utf16()));
		activeSkin = Skins::byId(fallbackSkinId);
		renderSkinId = SkinThemeData::entry(activeSkin->id()).paintBaseId;
		skinId = activeSkin->id();
		darkMode = dark;
		currentTokens = activeSkin->tokens(darkMode);

		// The process-wide QSS/palette/font contract is shared with companion
		// executables. The Editor keeps its CustomStyle, so Fusion is not reset.
		SkinThemeData::applyToApplication(*qApp, skinId, darkMode, false, true);
	}

	sheetApplied = true;
	emit skinChanged(currentTokens);
	for (QWidget* widget : qApp->allWidgets())
		widget->update();

	LogFStatic(L"Skin %s applied", reinterpret_cast<const wchar_t*>(skinId.utf16()));
}

void SkinManager::applyTokenPreview(const QString& newSkinId, bool dark, const SkinTokens& tokens)
{
	heritageMode = false;
	previewMode = true;
	activeSkin = Skins::byId(newSkinId);
	renderSkinId = SkinThemeData::entry(activeSkin->id()).paintBaseId;
	skinId = activeSkin->id();
	darkMode = dark;
	currentTokens = tokens;

	SkinThemeData::applyTokensToApplication(*qApp, skinId, darkMode, currentTokens, false, true);
	sheetApplied = true;

	emit skinChanged(currentTokens);
	for (QWidget* widget : qApp->allWidgets())
		widget->update();
}

// The forwarders below delegate without a null check on purpose: activeSkin
// is never null (class invariant, see the header). Only genuinely different
// behavior - the heritage branches - earns a conditional.

IRoutingRenderer* SkinManager::routingRenderer() const
{
	if (heritageMode)
		return nullptr;
	return activeSkin->routingRenderer();
}

void SkinManager::paintKnob(QPainter& painter, const QRect& rect, const KnobState& state) const
{
	// HeritageSkin inherits the base painter, which is exactly the heritage
	// AudioKnob rendering; no conditional needed.
	activeSkin->paintKnob(painter, rect, state, currentTokens);
}

QString SkinManager::cardFrameStyle(const CommandRowInfo& info) const
{
	return activeSkin->cardFrameStyle(info, currentTokens);
}

QString SkinManager::cardHeaderStyle(const CommandRowInfo& info) const
{
	return activeSkin->cardHeaderStyle(info, currentTokens);
}

BadgeTreatment SkinManager::badgeTreatment(const CommandRowInfo& info, const QString& typeColor, const QString& badgeToken) const
{
	return activeSkin->badgeTreatment(info, typeColor, badgeToken, currentTokens);
}

void SkinManager::prepareCommandRow(const CommandRowInfo& info, QWidget* card, QWidget* header, QWidget* body) const
{
	// The tokens come from here rather than from each skin reaching back into this
	// singleton, which is what all five did before the hook took them.
	activeSkin->prepareCommandRow(info, card, header, body, tokens());
}

void SkinManager::paintCardChrome(QPainter& painter, const QRect& rect, const CommandRowInfo& info) const
{
	activeSkin->paintCardChrome(painter, rect, info, currentTokens);
}

bool SkinManager::paintScopeGutter(QPainter& painter, const QSize& size, const CommandRowInfo& info) const
{
	return activeSkin->paintScopeGutter(painter, size, info, currentTokens);
}

bool SkinManager::paintChannelBadge(QPainter& painter, const QRect& rect, const QString& channel,
	bool virtualChannel) const
{
	return activeSkin->paintChannelBadge(painter, rect, channel, virtualChannel, currentTokens);
}

bool SkinManager::logicSiblingsIndentAsMembers() const
{
	return activeSkin->logicSiblingsIndentAsMembers();
}

void SkinManager::paintAddRow(QPainter& painter, const QRect& rect, const ListChromeState& state) const
{
	activeSkin->paintAddRow(painter, rect, state, currentTokens);
}

void SkinManager::paintInsertSeam(QPainter& painter, const QRect& rect, const ListChromeState& state) const
{
	activeSkin->paintInsertSeam(painter, rect, state, currentTokens);
}

void SkinManager::paintGraphicEqPlot(QPainter& painter, const GraphicEQPlotState& state) const
{
	activeSkin->paintGraphicEqPlot(painter, state, currentTokens);
}

void SkinManager::paintAnalysisGraph(QPainter& painter, const AnalysisGraphState& state) const
{
	activeSkin->paintAnalysisGraph(painter, state, currentTokens);
}

void SkinManager::paintSegmentedControl(QPainter& painter, const SegmentedControlState& state) const
{
	activeSkin->paintSegmentedControl(painter, state, currentTokens);
}

void SkinManager::paintVstBusSelector(QPainter& painter, const VstBusSelectorState& state) const
{
	activeSkin->paintVstBusSelector(painter, state, currentTokens);
}

void SkinManager::paintVstBusFrame(QPainter& painter, const VstBusFrameState& state) const
{
	activeSkin->paintVstBusFrame(painter, state, currentTokens);
}

void SkinManager::paintVstSlotFillCell(QPainter& painter, const VstSlotFillCellState& state) const
{
	activeSkin->paintVstSlotFillCell(painter, state, currentTokens);
}

void SkinManager::paintVstSlotFillRail(QPainter& painter, const VstSlotFillRailState& state) const
{
	activeSkin->paintVstSlotFillRail(painter, state, currentTokens);
}

FilterPickerView* SkinManager::createFilterPicker(QWidget* parent) const
{
	return activeSkin->createFilterPicker(parent, tokens());
}

ReferenceCardView* SkinManager::createReferenceCardView(const QString& kind, QWidget* parent) const
{
	return activeSkin->createReferenceCardView(kind, parent, tokens());
}

SubwooferRoutingCardView* SkinManager::createSubwooferRoutingCardView(QWidget* parent) const
{
	return activeSkin->createSubwooferRoutingCardView(parent, tokens());
}

void SkinManager::paintTitleBarChrome(QPainter& painter, const QRect& rect) const
{
	activeSkin->paintTitleBarChrome(painter, rect, currentTokens);
}

void SkinManager::styleMainToolbar(QToolBar* toolBar) const
{
	if (toolBar == nullptr)
		return;
	// Reset the shared mutable toolbar state before delegating so one skin's
	// choices cannot leak across a live skin switch (minimal sets
	// Qt::ToolButtonTextOnly; everyone else expects icon-only).
	toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	activeSkin->styleMainToolbar(toolBar, currentTokens);
}

void SkinManager::styleFileDialog(QFileDialog* dialog) const
{
	activeSkin->styleFileDialog(dialog, currentTokens);
}
