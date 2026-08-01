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
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "Editor/skins/SkinDisplayNames.h"
#include "Editor/skins/SkinSupport.h"
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

	skinComboBox = new QComboBox(this);
	const QStringList ids = SkinThemeData::ids();
	for (const QString& id : ids)
		skinComboBox->addItem(SkinDisplayNames::displayName(id), id);
	const int selectedIndex = qMax(0, skinComboBox->findData(SkinThemeData::resolveId(skinId)));
	skinComboBox->setCurrentIndex(selectedIndex);

	darkCheckBox = new QCheckBox(tr("Dark"), this);
	darkCheckBox->setChecked(dark);

	QHBoxLayout* chooserLayout = new QHBoxLayout;
	chooserLayout->addWidget(new QLabel(tr("Base theme:"), this));
	chooserLayout->addWidget(skinComboBox, 1);
	chooserLayout->addWidget(darkCheckBox);

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
	mainLayout->addLayout(bodyLayout, 1);
	mainLayout->addLayout(buttonLayout);

	connect(skinComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(reloadFromBase()));
	connect(darkCheckBox, SIGNAL(toggled(bool)), this, SLOT(reloadFromBase()));
	connect(colorTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(updatePreviewPanel()));
	connect(previewButton, SIGNAL(clicked()), this, SLOT(previewInApplication()));
	connect(resetButton, SIGNAL(clicked()), this, SLOT(resetActiveTheme()));
	connect(copyButton, SIGNAL(clicked()), this, SLOT(copyJson()));
	connect(exportButton, SIGNAL(clicked()), this, SLOT(exportJson()));
	connect(closeButton, SIGNAL(clicked()), this, SLOT(accept()));

	reloadFromBase();
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

void ThemeEditorDialog::resetActiveTheme()
{
	emit builtInThemeRequested(currentSkinId(), darkCheckBox->isChecked());
}

QString ThemeEditorDialog::themeJson(const SkinTokens& tokens) const
{
	QJsonObject colors;
	for (const ColorRow& spec : colorRows())
		colors.insert(QString::fromLatin1(spec.key), tokens.*(spec.field));
	colors.insert(QStringLiteral("surfaceRaised"), tokens.surfaceRaised);
	colors.insert(QStringLiteral("surfaceSunken"), tokens.surfaceSunken);
	colors.insert(QStringLiteral("graphGridMajor"), tokens.graphGridMajor);
	colors.insert(QStringLiteral("focusRing"), tokens.focusRing);

	QJsonObject root;
	root.insert(QStringLiteral("baseTheme"), currentSkinId());
	root.insert(QStringLiteral("dark"), darkCheckBox->isChecked());
	root.insert(QStringLiteral("colors"), colors);
	return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void ThemeEditorDialog::copyJson()
{
	bool ok = false;
	const SkinTokens tokens = tokensFromTable(&ok);
	if (!ok)
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Fix invalid colors before copying."));
		return;
	}
	QApplication::clipboard()->setText(themeJson(tokens));
}

void ThemeEditorDialog::exportJson()
{
	bool ok = false;
	const SkinTokens tokens = tokensFromTable(&ok);
	if (!ok)
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Fix invalid colors before exporting."));
		return;
	}

	const QString path = QFileDialog::getSaveFileName(this, tr("Export theme JSON"),
		QStringLiteral("%1-theme.json").arg(currentSkinId()),
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
	file.write(themeJson(tokens).toUtf8());
}
