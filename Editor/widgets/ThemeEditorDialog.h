/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#pragma once

#include <QDialog>
#include <QString>

#include "Editor/SkinTokens.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QTableWidget;

class ThemeEditorDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ThemeEditorDialog(const QString& skinId, bool dark, QWidget* parent = nullptr);

signals:
	void builtInThemeRequested(const QString& skinId, bool dark);
	void themePreviewRequested(const QString& skinId, bool dark, const SkinTokens& tokens);

private slots:
	void reloadFromBase();
	void updatePreviewPanel();
	void previewInApplication();
	void resetActiveTheme();
	void copyJson();
	void exportJson();

private:
	QString currentSkinId() const;
	SkinTokens tokensFromTable(bool* ok = nullptr) const;
	QString themeJson(const SkinTokens& tokens) const;
	void populateTable(const SkinTokens& tokens);

	QComboBox* skinComboBox = nullptr;
	QCheckBox* darkCheckBox = nullptr;
	QTableWidget* colorTable = nullptr;
	QLabel* previewLabel = nullptr;
};
