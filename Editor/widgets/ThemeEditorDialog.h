/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#pragma once

#include <QDialog>
#include <QString>

#include "Editor/SkinTokens.h"
#include "Editor/skins/CustomThemeStore.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class ThemeLabPreview;

class ThemeEditorDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ThemeEditorDialog(const QString& skinId, bool dark, QWidget* parent = nullptr);
	static int runSelfTest();

signals:
	void builtInThemeRequested(const QString& skinId, bool dark);
	void customThemeRequested(const QString& skinId);
	void themePreviewRequested(const QString& skinId, bool dark, const SkinTokens& tokens);

private slots:
	void reloadFromBase();
	void updatePreviewPanel();
	void loadSavedTheme();
	void applySavedTheme();
	void saveTheme();
	void importJson();
	void deleteSavedTheme();
	void previewInApplication();
	void resetActiveTheme();
	void copyJson();
	void exportJson();
	void chooseColorAt(int row, int column);
	void chooseSelectedColor();
	void resetSelectedColor();
	void repairTextReadability();

private:
	QString currentSkinId() const;
	int selectedColorRow() const;
	SkinTokens tokensFromTable(bool* ok = nullptr) const;
	CustomThemeStore::Theme themeFromTable(const QString& name = QString()) const;
	QString themeJson(const CustomThemeStore::Theme& theme) const;
	void populateTable(const SkinTokens& tokens);
	void setTableColor(int row, const QString& color);
	void reloadSavedThemes(const QString& selectSkinId = QString());
	bool selectedSavedTheme(CustomThemeStore::Theme* theme) const;

	QComboBox* skinComboBox = nullptr;
	QCheckBox* darkCheckBox = nullptr;
	QComboBox* savedThemeComboBox = nullptr;
	QTableWidget* colorTable = nullptr;
	ThemeLabPreview* previewWidget = nullptr;
	QLabel* auditLabel = nullptr;
	QPushButton* chooseColorButton = nullptr;
	QPushButton* resetTokenButton = nullptr;
	QPushButton* repairTextButton = nullptr;
};
