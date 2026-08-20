/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "ThemeEditorDialog.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "services/registry/RegistryPaths.h"
#include "Editor/skins/SkinDisplayNames.h"
#include "Editor/skins/shared/SkinSupport.h"
#include "Editor/skins/SkinThemeData.h"

namespace
{
struct ColorRow
{
	const char* key = nullptr;
	const char* label = nullptr;
	QString SkinTokens::* field = nullptr;
};

const QVector<ColorRow>& colorRows()
{
	static const QVector<ColorRow> rows = {
		{ "background", "Background", &SkinTokens::background },
		{ "surface", "Surface", &SkinTokens::surface },
		{ "card", "Card", &SkinTokens::card },
		{ "cardHover", "Card hover", &SkinTokens::cardHover },
		{ "cardSelected", "Card selected", &SkinTokens::cardSelected },
		{ "text", "Text", &SkinTokens::text },
		{ "mutedText", "Muted text", &SkinTokens::mutedText },
		{ "border", "Border", &SkinTokens::border },
		{ "graph", "Graph", &SkinTokens::graph },
		{ "graphGridMinor", "Graph grid", &SkinTokens::graphGridMinor },
		{ "accent", "Accent", &SkinTokens::accent },
		{ "accent2", "Accent 2", &SkinTokens::accent2 },
		{ "success", "Success", &SkinTokens::success },
		{ "warning", "Warning", &SkinTokens::warning },
		{ "danger", "Danger", &SkinTokens::danger }
	};
	return rows;
}

QString normalizeColor(const QString& value)
{
	const QColor color(value.trimmed());
	if (!color.isValid())
		return QString();
	return color.name(QColor::HexRgb).toUpper();
}

QString defaultThemeName(const QComboBox* skinComboBox)
{
	if (skinComboBox == nullptr)
		return QStringLiteral("Custom Theme");
	return QStringLiteral("%1 Custom").arg(skinComboBox->currentText());
}

void setItemValid(QTableWidgetItem* item, bool valid)
{
	if (item == nullptr)
		return;
	item->setBackground(valid ? QBrush() : QBrush(QColor(QStringLiteral("#5A1E2A"))));
}
}

ThemeEditorDialog::ThemeEditorDialog(const QString& skinId, bool dark, QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Theme Lab"));
	setMinimumSize(760, 520);

	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	CustomThemeStore::Theme initialCustomTheme;
	const bool initialThemeIsCustom =
		CustomThemeStore::findTheme(settings, skinId, &initialCustomTheme);
	const QString initialBaseTheme = initialThemeIsCustom
		? initialCustomTheme.baseTheme
		: SkinThemeData::resolveId(skinId);
	const bool initialDark = initialThemeIsCustom ? initialCustomTheme.dark : dark;

	skinComboBox = new QComboBox(this);
	const QStringList ids = SkinThemeData::ids();
	for (const QString& id : ids)
		skinComboBox->addItem(SkinDisplayNames::displayName(id), id);
	const int selectedIndex = qMax(0, skinComboBox->findData(initialBaseTheme));
	skinComboBox->setCurrentIndex(selectedIndex);

	darkCheckBox = new QCheckBox(tr("Dark"), this);
	darkCheckBox->setChecked(initialDark);

	QHBoxLayout* chooserLayout = new QHBoxLayout;
	chooserLayout->addWidget(new QLabel(tr("Base theme:"), this));
	chooserLayout->addWidget(skinComboBox, 1);
	chooserLayout->addWidget(darkCheckBox);

	savedThemeComboBox = new QComboBox(this);
	savedThemeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	QPushButton* loadSavedButton = new QPushButton(tr("Load"), this);
	QPushButton* applySavedButton = new QPushButton(tr("Apply"), this);
	QPushButton* saveButton = new QPushButton(tr("Save as..."), this);
	QPushButton* importButton = new QPushButton(tr("Import JSON..."), this);
	QPushButton* deleteButton = new QPushButton(tr("Delete"), this);

	QHBoxLayout* savedLayout = new QHBoxLayout;
	savedLayout->addWidget(new QLabel(tr("Saved theme:"), this));
	savedLayout->addWidget(savedThemeComboBox, 1);
	savedLayout->addWidget(loadSavedButton);
	savedLayout->addWidget(applySavedButton);
	savedLayout->addWidget(saveButton);
	savedLayout->addWidget(importButton);
	savedLayout->addWidget(deleteButton);

	colorTable = new QTableWidget(this);
	colorTable->setColumnCount(3);
	colorTable->setHorizontalHeaderLabels(QStringList()
		<< tr("Token") << tr("Color") << tr("Swatch"));
	colorTable->verticalHeader()->setVisible(false);
	colorTable->horizontalHeader()->setStretchLastSection(true);
	colorTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	colorTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	colorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	colorTable->setEditTriggers(QAbstractItemView::DoubleClicked
		| QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);

	previewLabel = new QLabel(this);
	previewLabel->setTextFormat(Qt::RichText);
	previewLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	previewLabel->setMinimumWidth(300);
	previewLabel->setWordWrap(true);

	QHBoxLayout* bodyLayout = new QHBoxLayout;
	bodyLayout->addWidget(colorTable, 2);
	bodyLayout->addWidget(previewLabel, 1);

	QPushButton* previewButton = new QPushButton(tr("Preview in app"), this);
	QPushButton* resetButton = new QPushButton(tr("Reset active theme"), this);
	QPushButton* copyButton = new QPushButton(tr("Copy JSON"), this);
	QPushButton* exportButton = new QPushButton(tr("Export JSON..."), this);
	QPushButton* closeButton = new QPushButton(tr("Close"), this);
	closeButton->setDefault(true);

	QHBoxLayout* buttonLayout = new QHBoxLayout;
	buttonLayout->addWidget(previewButton);
	buttonLayout->addWidget(resetButton);
	buttonLayout->addStretch(1);
	buttonLayout->addWidget(copyButton);
	buttonLayout->addWidget(exportButton);
	buttonLayout->addWidget(closeButton);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(chooserLayout);
	mainLayout->addLayout(savedLayout);
	mainLayout->addLayout(bodyLayout, 1);
	mainLayout->addLayout(buttonLayout);

	connect(skinComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(reloadFromBase()));
	connect(darkCheckBox, SIGNAL(toggled(bool)), this, SLOT(reloadFromBase()));
	connect(colorTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(updatePreviewPanel()));
	connect(loadSavedButton, SIGNAL(clicked()), this, SLOT(loadSavedTheme()));
	connect(applySavedButton, SIGNAL(clicked()), this, SLOT(applySavedTheme()));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(saveTheme()));
	connect(importButton, SIGNAL(clicked()), this, SLOT(importJson()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteSavedTheme()));
	connect(previewButton, SIGNAL(clicked()), this, SLOT(previewInApplication()));
	connect(resetButton, SIGNAL(clicked()), this, SLOT(resetActiveTheme()));
	connect(copyButton, SIGNAL(clicked()), this, SLOT(copyJson()));
	connect(exportButton, SIGNAL(clicked()), this, SLOT(exportJson()));
	connect(closeButton, SIGNAL(clicked()), this, SLOT(accept()));

	reloadFromBase();
	reloadSavedThemes(initialThemeIsCustom ? initialCustomTheme.skinId() : QString());
	if (initialThemeIsCustom)
	{
		populateTable(CustomThemeStore::tokensForTheme(initialCustomTheme));
		updatePreviewPanel();
	}
}

QString ThemeEditorDialog::currentSkinId() const
{
	return skinComboBox->currentData().toString();
}

void ThemeEditorDialog::reloadFromBase()
{
	populateTable(SkinThemeData::tokens(currentSkinId(), darkCheckBox->isChecked()));
	updatePreviewPanel();
}

void ThemeEditorDialog::populateTable(const SkinTokens& tokens)
{
	colorTable->blockSignals(true);
	colorTable->setRowCount(colorRows().size());
	for (int row = 0; row < colorRows().size(); ++row)
	{
		const ColorRow& spec = colorRows()[row];
		QTableWidgetItem* keyItem = new QTableWidgetItem(QString::fromLatin1(spec.label));
		keyItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		keyItem->setData(Qt::UserRole, QString::fromLatin1(spec.key));
		colorTable->setItem(row, 0, keyItem);

		QTableWidgetItem* valueItem = new QTableWidgetItem(tokens.*(spec.field));
		colorTable->setItem(row, 1, valueItem);

		QTableWidgetItem* swatchItem = new QTableWidgetItem;
		swatchItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		swatchItem->setBackground(QColor(tokens.*(spec.field)));
		colorTable->setItem(row, 2, swatchItem);
	}
	colorTable->blockSignals(false);
}

SkinTokens ThemeEditorDialog::tokensFromTable(bool* ok) const
{
	const QSignalBlocker blocker(colorTable);
	bool valid = true;
	SkinTokens tokens = SkinThemeData::tokens(currentSkinId(), darkCheckBox->isChecked());
	tokens.dark = darkCheckBox->isChecked();

	for (int row = 0; row < colorRows().size(); ++row)
	{
		const ColorRow& spec = colorRows()[row];
		QTableWidgetItem* valueItem = colorTable->item(row, 1);
		const QString normalized = normalizeColor(valueItem != nullptr ? valueItem->text() : QString());
		const bool rowValid = !normalized.isEmpty();
		setItemValid(valueItem, rowValid);
		if (QTableWidgetItem* swatchItem = colorTable->item(row, 2))
			swatchItem->setBackground(rowValid ? QColor(normalized) : QColor(QStringLiteral("#5A1E2A")));
		if (!rowValid)
		{
			valid = false;
			continue;
		}
		tokens.*(spec.field) = normalized;
	}

	finishTokens(tokens);
	if (ok != nullptr)
		*ok = valid;
	return tokens;
}

void ThemeEditorDialog::updatePreviewPanel()
{
	bool ok = false;
	const SkinTokens tokens = tokensFromTable(&ok);
	QString swatches;
	for (const ColorRow& spec : colorRows())
	{
		const QString value = tokens.*(spec.field);
		swatches += QStringLiteral(
			"<span style=\"display:inline-block;margin:0 6px 6px 0;\">"
			"<span style=\"display:inline-block;width:18px;height:18px;border:1px solid %1;background:%2;\"></span>"
			" %3</span>")
			.arg(tokens.border.toHtmlEscaped(), value.toHtmlEscaped(),
				QString::fromLatin1(spec.key).toHtmlEscaped());
	}

	QString html = QStringLiteral(
		"<div style=\"background:%1;color:%2;border:1px solid %3;border-radius:10px;padding:14px;\">"
		"<div style=\"font-size:16px;font-weight:600;color:%4;\">%5</div>"
		"<div style=\"color:%6;margin:4px 0 12px 0;\">%7</div>"
		"<div style=\"background:%8;border:1px solid %3;border-left:4px solid %4;border-radius:8px;padding:10px;margin-bottom:12px;\">"
		"<b>%9</b><br/><span style=\"color:%6;\">%10</span></div>"
		"<div>%11</div>"
		"<div style=\"margin-top:12px;color:%12;\">%13</div>"
		"</div>");
	html = html.arg(tokens.background.toHtmlEscaped());
	html = html.arg(tokens.text.toHtmlEscaped());
	html = html.arg(tokens.border.toHtmlEscaped());
	html = html.arg(tokens.accent.toHtmlEscaped());
	html = html.arg(tr("Theme Lab preview").toHtmlEscaped());
	html = html.arg(tokens.mutedText.toHtmlEscaped());
	html = html.arg(tr("Edit colors, then preview them against the live app.").toHtmlEscaped());
	html = html.arg(tokens.card.toHtmlEscaped());
	html = html.arg(tr("Sample card").toHtmlEscaped());
	html = html.arg(tr("Accent rails, graphs, badges, and text inherit these tokens.").toHtmlEscaped());
	html = html.arg(swatches);
	html = html.arg(ok ? tokens.success.toHtmlEscaped() : tokens.danger.toHtmlEscaped());
	html = html.arg((ok ? tr("All colors are valid.") : tr("Fix invalid colors before preview/export.")).toHtmlEscaped());
	previewLabel->setText(html);
}

void ThemeEditorDialog::previewInApplication()
{
	bool ok = false;
	const SkinTokens tokens = tokensFromTable(&ok);
	if (!ok)
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Fix invalid colors before previewing."));
		return;
	}
	emit themePreviewRequested(currentSkinId(), darkCheckBox->isChecked(), tokens);
}

CustomThemeStore::Theme ThemeEditorDialog::themeFromTable(const QString& name) const
{
	bool ok = false;
	const SkinTokens tokens = tokensFromTable(&ok);
	Q_UNUSED(ok);

	CustomThemeStore::Theme theme;
	theme.name = name.trimmed().isEmpty() ? defaultThemeName(skinComboBox) : name.trimmed();
	theme.baseTheme = currentSkinId();
	theme.dark = darkCheckBox->isChecked();
	for (const ColorRow& spec : colorRows())
		theme.colors.insert(QString::fromLatin1(spec.key), tokens.*(spec.field));
	return theme;
}

void ThemeEditorDialog::reloadSavedThemes(const QString& selectSkinId)
{
	if (savedThemeComboBox == nullptr)
		return;

	const QSignalBlocker blocker(savedThemeComboBox);
	savedThemeComboBox->clear();

	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	const QList<CustomThemeStore::Theme> savedThemes = CustomThemeStore::themes(settings);
	for (const CustomThemeStore::Theme& theme : savedThemes)
		savedThemeComboBox->addItem(theme.name, theme.skinId());

	if (savedThemeComboBox->count() == 0)
	{
		savedThemeComboBox->addItem(tr("No saved themes"), QString());
		savedThemeComboBox->setEnabled(false);
		return;
	}

	savedThemeComboBox->setEnabled(true);
	if (!selectSkinId.isEmpty())
	{
		const int index = savedThemeComboBox->findData(selectSkinId);
		if (index >= 0)
			savedThemeComboBox->setCurrentIndex(index);
	}
}

bool ThemeEditorDialog::selectedSavedTheme(CustomThemeStore::Theme* theme) const
{
	if (savedThemeComboBox == nullptr || !savedThemeComboBox->isEnabled())
		return false;

	const QString skinId = savedThemeComboBox->currentData().toString();
	if (skinId.isEmpty())
		return false;

	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	return CustomThemeStore::findTheme(settings, skinId, theme);
}

void ThemeEditorDialog::loadSavedTheme()
{
	CustomThemeStore::Theme theme;
	if (!selectedSavedTheme(&theme))
		return;

	{
		const QSignalBlocker skinBlocker(skinComboBox);
		const QSignalBlocker darkBlocker(darkCheckBox);
		const int index = skinComboBox->findData(SkinThemeData::resolveId(theme.baseTheme));
		if (index >= 0)
			skinComboBox->setCurrentIndex(index);
		darkCheckBox->setChecked(theme.dark);
	}
	populateTable(CustomThemeStore::tokensForTheme(theme));
	updatePreviewPanel();
}

void ThemeEditorDialog::applySavedTheme()
{
	CustomThemeStore::Theme theme;
	if (selectedSavedTheme(&theme))
		emit customThemeRequested(theme.skinId());
}

void ThemeEditorDialog::saveTheme()
{
	bool ok = false;
	tokensFromTable(&ok);
	if (!ok)
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Fix invalid colors before saving."));
		return;
	}

	QString defaultName = defaultThemeName(skinComboBox);
	CustomThemeStore::Theme selectedTheme;
	if (selectedSavedTheme(&selectedTheme))
		defaultName = selectedTheme.name;

	const QString name = QInputDialog::getText(this, tr("Save theme"),
		tr("Theme name:"), QLineEdit::Normal, defaultName, &ok).trimmed();
	if (!ok || name.isEmpty())
		return;

	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	CustomThemeStore::Theme theme = themeFromTable(name);
	theme.id = CustomThemeStore::uniqueIdForName(settings, name);
	if (!CustomThemeStore::saveTheme(settings, theme))
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Could not save the theme."));
		return;
	}
	reloadSavedThemes(theme.skinId());
}

void ThemeEditorDialog::importJson()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("Import theme JSON"),
		QString(), tr("Theme JSON (*.json)"));
	if (path.isEmpty())
		return;

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, tr("Theme Lab"),
			tr("Could not read %1").arg(QDir::toNativeSeparators(path)));
		return;
	}

	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
	if (parseError.error != QJsonParseError::NoError || !document.isObject())
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("The selected file is not valid theme JSON."));
		return;
	}

	QString error;
	CustomThemeStore::Theme theme;
	if (!CustomThemeStore::fromJsonObject(document.object(), &theme, &error))
	{
		QMessageBox::warning(this, tr("Theme Lab"), error);
		return;
	}
	if (theme.name.isEmpty())
		theme.name = QFileInfo(path).completeBaseName();

	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	if (theme.id.isEmpty()
		|| CustomThemeStore::findTheme(settings, theme.skinId(), nullptr)
		|| SkinThemeData::ids().contains(theme.id))
	{
		theme.id = CustomThemeStore::uniqueIdForName(settings, theme.name);
	}

	if (!CustomThemeStore::saveTheme(settings, theme))
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Could not import the theme."));
		return;
	}
	reloadSavedThemes(theme.skinId());
	loadSavedTheme();
}

void ThemeEditorDialog::deleteSavedTheme()
{
	CustomThemeStore::Theme theme;
	if (!selectedSavedTheme(&theme))
		return;

	if (QMessageBox::question(this, tr("Delete theme"),
		tr("Delete saved theme \"%1\"?").arg(theme.name)) != QMessageBox::Yes)
		return;

	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	CustomThemeStore::removeTheme(settings, theme.skinId());
	reloadSavedThemes();
}

void ThemeEditorDialog::resetActiveTheme()
{
	emit builtInThemeRequested(currentSkinId(), darkCheckBox->isChecked());
}

QString ThemeEditorDialog::themeJson(const CustomThemeStore::Theme& theme) const
{
	return QString::fromUtf8(QJsonDocument(
		CustomThemeStore::toJsonObject(theme)).toJson(QJsonDocument::Indented));
}

void ThemeEditorDialog::copyJson()
{
	bool ok = false;
	tokensFromTable(&ok);
	if (!ok)
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Fix invalid colors before copying."));
		return;
	}
	QApplication::clipboard()->setText(themeJson(themeFromTable(defaultThemeName(skinComboBox))));
}

void ThemeEditorDialog::exportJson()
{
	bool ok = false;
	tokensFromTable(&ok);
	if (!ok)
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Fix invalid colors before exporting."));
		return;
	}

	const CustomThemeStore::Theme theme = themeFromTable(defaultThemeName(skinComboBox));
	const QString path = QFileDialog::getSaveFileName(this, tr("Export theme JSON"),
		QStringLiteral("%1-theme.json").arg(CustomThemeStore::suggestedId(theme.name)),
		tr("Theme JSON (*.json)"));
	if (path.isEmpty())
		return;

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, tr("Theme Lab"),
			tr("Could not write %1").arg(QDir::toNativeSeparators(path)));
		return;
	}
	file.write(themeJson(theme).toUtf8());
}
