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
#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSettings>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QtMath>

#include <limits>

#include "services/registry/RegistryPaths.h"
#include "Editor/skins/SkinDisplayNames.h"
#include "Editor/skins/shared/SkinSupport.h"
#include "Editor/skins/SkinThemeData.h"
#include "Editor/SkinManager.h"

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

QString themeLabText(const char* sourceText)
{
	return QCoreApplication::translate("ThemeEditorDialog", sourceText);
}
}

// A neutral token/control preview gives Theme Lab a compact sanity check:
// token swatches alone cannot reveal whether a light/dark pair still reads as
// an application. It intentionally does not impersonate a skin's real knob
// painter, and derives every colour from the edited token table.
class ThemeLabPreview final : public QWidget
{
public:
	explicit ThemeLabPreview(QWidget* parent = nullptr)
		: QWidget(parent)
	{
		setMinimumSize(330, 330);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}

	void setTheme(const SkinTokens& value, int checksPassed, int checksTotal)
	{
		tokens = value;
		passed = checksPassed;
		total = checksTotal;
		update();
	}

protected:
	void paintEvent(QPaintEvent*) override
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.fillRect(rect(), QColor(tokens.background));

		const QRectF canvas = QRectF(rect()).adjusted(10.0, 10.0, -10.0, -10.0);
		if (canvas.width() < 80.0 || canvas.height() < 80.0)
			return;

		const QColor card(tokens.card);
		const QColor border(tokens.border);
		const QColor ink(tokens.text);
		const QColor muted(tokens.mutedText);
		const QColor accent(tokens.accent);
		const qreal radius = qMax<qreal>(4.0, qMin<qreal>(tokens.borderRadius, 14.0));

		QPainterPath frame;
		frame.addRoundedRect(canvas, radius, radius);
		painter.fillPath(frame, card);
		painter.setPen(QPen(border, 1.0));
		painter.setBrush(Qt::NoBrush);
		painter.drawPath(frame);

		QFont titleFont(tokens.fontFamily);
		titleFont.setPixelSize(15);
		titleFont.setWeight(QFont::DemiBold);
		painter.setFont(titleFont);
		painter.setPen(ink);
		const QRectF titleRect(canvas.left() + 14.0, canvas.top() + 10.0, canvas.width() - 140.0, 24.0);
		painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, themeLabText("Theme Lab preview"));

		const QString mode = (tokens.dark ? themeLabText("Dark mode") : themeLabText("Light mode")).toUpper();
		QFont modeFont(tokens.monoFontFamily);
		modeFont.setPixelSize(9);
		modeFont.setBold(true);
		painter.setFont(modeFont);
		const QFontMetricsF modeMetrics(modeFont);
		const qreal modeWidth = modeMetrics.horizontalAdvance(mode) + 18.0;
		const QRectF modeRect(canvas.right() - modeWidth - 12.0, canvas.top() + 12.0, modeWidth, 20.0);
		painter.setPen(Qt::NoPen);
		QColor modeFill = accent;
		modeFill.setAlpha(tokens.dark ? 70 : 48);
		painter.setBrush(modeFill);
		painter.drawRoundedRect(modeRect, 10.0, 10.0);
		painter.setPen(ink);
		painter.drawText(modeRect, Qt::AlignCenter, mode);

		const QRectF sampleRect(canvas.left() + 14.0, canvas.top() + 48.0,
			canvas.width() - 28.0, qMax<qreal>(135.0, canvas.height() * 0.46));
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(tokens.surface));
		painter.drawRoundedRect(sampleRect, radius, radius);
		painter.setPen(QPen(border, 1.0));
		painter.setBrush(Qt::NoBrush);
		painter.drawRoundedRect(sampleRect, radius, radius);

		QFont labelFont(tokens.fontFamily);
		labelFont.setPixelSize(11);
		painter.setFont(labelFont);
		painter.setPen(muted);
		painter.drawText(QRectF(sampleRect.left() + 12.0, sampleRect.top() + 10.0,
			sampleRect.width() - 24.0, 18.0), Qt::AlignLeft | Qt::AlignVCenter,
			themeLabText("Output gain").toUpper());

		const qreal knobSide = qMin<qreal>(76.0, qMax<qreal>(44.0, sampleRect.height() - 54.0));
		const QRectF knobRect(sampleRect.left() + 16.0, sampleRect.top() + 34.0, knobSide, knobSide);
		const QPointF knobCenter = knobRect.center();
		const qreal knobRadius = knobSide / 2.0 - 4.0;

		// This is a neutral control sample, not a substitute for the skin's
		// actual AudioKnob grammar.
		painter.setPen(QPen(QColor(tokens.graphGridMinor), 5.0, Qt::SolidLine, Qt::RoundCap));
		painter.setBrush(Qt::NoBrush);
		painter.drawArc(knobRect.adjusted(3.0, 3.0, -3.0, -3.0), -135 * 16, -270 * 16);
		painter.setPen(QPen(accent, 5.0, Qt::SolidLine, Qt::RoundCap));
		painter.drawArc(knobRect.adjusted(3.0, 3.0, -3.0, -3.0), -135 * 16, -172 * 16);

		QRadialGradient face(knobCenter - QPointF(knobRadius * 0.35, knobRadius * 0.40), knobRadius * 1.9);
		face.setColorAt(0.0, card.lighter(tokens.dark ? 165 : 118));
		face.setColorAt(0.62, card.lighter(tokens.dark ? 110 : 104));
		face.setColorAt(1.0, tokens.dark ? card.darker(160) : card.darker(135));
		painter.setPen(QPen(tokens.dark ? QColor(tokens.surfaceSunken).darker(120) : border.darker(120), 1.0));
		painter.setBrush(face);
		painter.drawEllipse(knobCenter, knobRadius, knobRadius);
		QColor highlight(tokens.surface);
		highlight.setAlpha(tokens.dark ? 70 : 150);
		painter.setPen(QPen(highlight, 1.2));
		painter.setBrush(Qt::NoBrush);
		painter.drawArc(QRectF(knobCenter.x() - knobRadius + 2.0, knobCenter.y() - knobRadius + 2.0,
			(knobRadius - 2.0) * 2.0, (knobRadius - 2.0) * 2.0), 45 * 16, 72 * 16);

		const qreal indicatorAngle = qDegreesToRadians(-315.0);
		const QPointF indicatorBase = knobCenter + QPointF(qCos(indicatorAngle), qSin(indicatorAngle)) * (knobRadius * 0.25);
		const QPointF indicatorTip = knobCenter + QPointF(qCos(indicatorAngle), qSin(indicatorAngle)) * (knobRadius * 0.80);
		QColor indicatorShadow(tokens.surfaceSunken);
		indicatorShadow.setAlpha(100);
		painter.setPen(QPen(indicatorShadow, 3.5, Qt::SolidLine, Qt::RoundCap));
		painter.drawLine(indicatorBase, indicatorTip);
		painter.setPen(QPen(accent, 2.1, Qt::SolidLine, Qt::RoundCap));
		painter.drawLine(indicatorBase, indicatorTip);

		const QRectF valueRect(knobRect.right() + 12.0, sampleRect.top() + 43.0,
			sampleRect.right() - knobRect.right() - 24.0, 28.0);
		painter.setPen(QPen(border, 1.0));
		painter.setBrush(QColor(tokens.surfaceSunken));
		painter.drawRoundedRect(valueRect, qMax<qreal>(3.0, radius - 3.0), qMax<qreal>(3.0, radius - 3.0));
		QFont valueFont(tokens.monoFontFamily);
		valueFont.setPixelSize(13);
		valueFont.setBold(true);
		painter.setFont(valueFont);
		painter.setPen(ink);
		painter.drawText(valueRect, Qt::AlignCenter, QStringLiteral("-3.0 dB"));

		painter.setFont(labelFont);
		painter.setPen(muted);
		painter.drawText(QRectF(valueRect.left(), valueRect.bottom() + 7.0, valueRect.width(), 18.0),
			Qt::AlignCenter, themeLabText("Live control sample"));

		const QRectF graphRect(valueRect.left(), sampleRect.top() + 100.0, valueRect.width(),
			qMax<qreal>(28.0, sampleRect.bottom() - (sampleRect.top() + 112.0)));
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(tokens.graph));
		painter.drawRoundedRect(graphRect, qMax<qreal>(3.0, radius - 4.0), qMax<qreal>(3.0, radius - 4.0));
		painter.setPen(QPen(QColor(tokens.graphGridMinor), 1.0));
		for (int line = 1; line < 4; ++line)
		{
			const qreal y = graphRect.top() + graphRect.height() * line / 4.0;
			painter.drawLine(QPointF(graphRect.left() + 5.0, y), QPointF(graphRect.right() - 5.0, y));
		}
		QPainterPath response;
		response.moveTo(graphRect.left() + 4.0, graphRect.center().y() + 6.0);
		response.cubicTo(graphRect.left() + graphRect.width() * 0.28, graphRect.bottom() - 5.0,
			graphRect.left() + graphRect.width() * 0.56, graphRect.top() + 5.0,
			graphRect.right() - 4.0, graphRect.center().y() - 6.0);
		painter.setPen(QPen(accent, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter.drawPath(response);

		const QRectF tooltipRect(canvas.left() + 14.0, sampleRect.bottom() + 12.0,
			canvas.width() - 28.0, 42.0);
		painter.setPen(QPen(border, 1.0));
		painter.setBrush(QColor(tokens.card));
		painter.drawRoundedRect(tooltipRect, qMax<qreal>(4.0, radius - 2.0), qMax<qreal>(4.0, radius - 2.0));
		painter.setFont(labelFont);
		painter.setPen(ink);
		painter.drawText(tooltipRect.adjusted(10.0, 3.0, -10.0, -3.0),
			Qt::AlignLeft | Qt::AlignVCenter, themeLabText("Tooltip  •  Remove selected filter"));

		const QRectF auditRect(canvas.left() + 14.0, tooltipRect.bottom() + 8.0,
			canvas.width() - 28.0, 20.0);
		QFont auditFont(tokens.monoFontFamily);
		auditFont.setPixelSize(10);
		painter.setFont(auditFont);
		painter.setPen(passed == total ? QColor(tokens.success) : QColor(tokens.danger));
		painter.drawText(auditRect, Qt::AlignLeft | Qt::AlignVCenter,
			themeLabText("Readability  %1/%2 pass").toUpper().arg(passed).arg(total));
	}

private:
	SkinTokens tokens;
	int passed = 0;
	int total = 0;
};

int ThemeEditorDialog::runSelfTest()
{
	int previews = 0;
	int failures = 0;
	for (const QString& skinId : SkinThemeData::ids())
	{
		for (const bool dark : { false, true })
		{
			const SkinTokens tokens = SkinThemeData::tokens(skinId, dark);
			if (skinId.startsWith(QStringLiteral("legacy-")))
				SkinManager::instance()->applyHeritage(skinId, dark);
			else
				SkinThemeData::applyToApplication(*qApp, skinId, dark, false, true);

			ThemeEditorDialog dialog(skinId, dark);
			dialog.resize(1080, 720);
			dialog.show();
			qApp->processEvents();
			++previews;
			if (dialog.previewWidget == nullptr || dialog.previewWidget->grab().isNull())
				++failures;
			if (!qApp->styleSheet().contains(SkinThemeData::tooltipOverride(tokens)))
				++failures;
			dialog.hide();
			qApp->processEvents();
		}
	}
	qWarning().noquote() << QStringLiteral("ThemeLabTest: %1 previews, %2 failures")
		.arg(previews).arg(failures);
	return failures == 0 ? 0 : 1;
}

ThemeEditorDialog::ThemeEditorDialog(const QString& skinId, bool dark, QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Theme Lab"));
	setMinimumSize(980, 640);

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
		<< tr("Token") << tr("Color") << tr("Swatch / picker"));
	colorTable->verticalHeader()->setVisible(false);
	colorTable->horizontalHeader()->setStretchLastSection(true);
	colorTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	colorTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	colorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	colorTable->setSelectionMode(QAbstractItemView::SingleSelection);
	colorTable->setAlternatingRowColors(true);
	colorTable->setEditTriggers(QAbstractItemView::DoubleClicked
		| QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);

	chooseColorButton = new QPushButton(tr("Choose color..."), this);
	resetTokenButton = new QPushButton(tr("Reset selected"), this);
	repairTextButton = new QPushButton(tr("Repair text contrast"), this);
	chooseColorButton->setToolTip(tr("Choose the selected token's color with a picker."));
	resetTokenButton->setToolTip(tr("Restore the selected token from the chosen base theme and mode."));
	repairTextButton->setToolTip(tr("Adjust only Text and Muted text until they pass the readability audit."));
	QHBoxLayout* tokenToolsLayout = new QHBoxLayout;
	tokenToolsLayout->setContentsMargins(0, 6, 0, 0);
	tokenToolsLayout->addWidget(chooseColorButton);
	tokenToolsLayout->addWidget(resetTokenButton);
	tokenToolsLayout->addStretch(1);
	tokenToolsLayout->addWidget(repairTextButton);

	previewWidget = new ThemeLabPreview(this);
	auditLabel = new QLabel(this);
	auditLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	auditLabel->setWordWrap(true);
	auditLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	auditLabel->setMinimumWidth(330);

	QVBoxLayout* tokenLayout = new QVBoxLayout;
	tokenLayout->setContentsMargins(0, 0, 0, 0);
	tokenLayout->addWidget(colorTable, 1);
	tokenLayout->addLayout(tokenToolsLayout);

	QVBoxLayout* previewLayout = new QVBoxLayout;
	previewLayout->setContentsMargins(0, 0, 0, 0);
	previewLayout->addWidget(previewWidget, 1);
	previewLayout->addWidget(auditLabel);

	QHBoxLayout* bodyLayout = new QHBoxLayout;
	bodyLayout->addLayout(tokenLayout, 2);
	bodyLayout->addLayout(previewLayout, 1);

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
	connect(colorTable, SIGNAL(cellClicked(int,int)), this, SLOT(chooseColorAt(int,int)));
	connect(loadSavedButton, SIGNAL(clicked()), this, SLOT(loadSavedTheme()));
	connect(applySavedButton, SIGNAL(clicked()), this, SLOT(applySavedTheme()));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(saveTheme()));
	connect(importButton, SIGNAL(clicked()), this, SLOT(importJson()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteSavedTheme()));
	connect(previewButton, SIGNAL(clicked()), this, SLOT(previewInApplication()));
	connect(resetButton, SIGNAL(clicked()), this, SLOT(resetActiveTheme()));
	connect(copyButton, SIGNAL(clicked()), this, SLOT(copyJson()));
	connect(exportButton, SIGNAL(clicked()), this, SLOT(exportJson()));
	connect(chooseColorButton, SIGNAL(clicked()), this, SLOT(chooseSelectedColor()));
	connect(resetTokenButton, SIGNAL(clicked()), this, SLOT(resetSelectedColor()));
	connect(repairTextButton, SIGNAL(clicked()), this, SLOT(repairTextReadability()));
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
		valueItem->setToolTip(tr("Enter #RRGGBB, or select this row and use Choose color."));
		colorTable->setItem(row, 1, valueItem);

		QTableWidgetItem* swatchItem = new QTableWidgetItem(tr("Pick..."));
		swatchItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		swatchItem->setBackground(QColor(tokens.*(spec.field)));
		swatchItem->setTextAlignment(Qt::AlignCenter);
		swatchItem->setToolTip(tr("Click to choose this color."));
		colorTable->setItem(row, 2, swatchItem);
		colorTable->setRowHeight(row, 30);
	}
	colorTable->blockSignals(false);
	if (colorTable->rowCount() > 0)
		colorTable->setCurrentCell(0, 1);
}

int ThemeEditorDialog::selectedColorRow() const
{
	const int row = colorTable != nullptr ? colorTable->currentRow() : -1;
	return row >= 0 && row < colorRows().size() ? row : -1;
}

void ThemeEditorDialog::setTableColor(int row, const QString& color)
{
	if (row < 0 || row >= colorRows().size())
		return;
	const QString normalized = normalizeColor(color);
	if (normalized.isEmpty())
		return;
	if (QTableWidgetItem* item = colorTable->item(row, 1))
		item->setText(normalized);
}

void ThemeEditorDialog::chooseColorAt(int row, int column)
{
	if (column != 2)
		return;
	colorTable->setCurrentCell(row, 1);
	chooseSelectedColor();
}

void ThemeEditorDialog::chooseSelectedColor()
{
	const int row = selectedColorRow();
	if (row < 0)
		return;

	QColor initial(colorTable->item(row, 1)->text());
	if (!initial.isValid())
		initial = QColor(SkinThemeData::tokens(currentSkinId(), darkCheckBox->isChecked())
			.*(colorRows()[row].field));
	const QColor selected = QColorDialog::getColor(initial, this,
		tr("Choose %1").arg(QString::fromLatin1(colorRows()[row].label)));
	if (selected.isValid())
		setTableColor(row, selected.name(QColor::HexRgb));
}

void ThemeEditorDialog::resetSelectedColor()
{
	const int row = selectedColorRow();
	if (row < 0)
		return;
	const SkinTokens base = SkinThemeData::tokens(currentSkinId(), darkCheckBox->isChecked());
	setTableColor(row, base.*(colorRows()[row].field));
}

void ThemeEditorDialog::repairTextReadability()
{
	bool colorsValid = false;
	SkinTokens tokens = tokensFromTable(&colorsValid);
	if (!colorsValid)
	{
		QMessageBox::warning(this, tr("Theme Lab"), tr("Fix invalid colors before repairing readability."));
		return;
	}

	SkinThemeData::repairTextReadability(tokens);
	for (int row = 0; row < colorRows().size(); ++row)
	{
		const ColorRow& spec = colorRows()[row];
		if (QString::fromLatin1(spec.key) == QLatin1String("text")
			|| QString::fromLatin1(spec.key) == QLatin1String("mutedText"))
			setTableColor(row, tokens.*(spec.field));
	}
	updatePreviewPanel();
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
	bool colorsValid = false;
	const SkinTokens tokens = tokensFromTable(&colorsValid);
	const QVector<SkinThemeData::ReadabilityCheck> checks =
		SkinThemeData::readabilityChecks(tokens);
	int passed = 0;
	double weakestText = std::numeric_limits<double>::max();
	double weakestMuted = std::numeric_limits<double>::max();
	double selectionContrast = 0.0;
	QStringList failures;
	for (const SkinThemeData::ReadabilityCheck& check : checks)
	{
		if (check.passes())
			passed++;
		else
			failures.append(QStringLiteral("%1 (%2:1)")
				.arg(check.label).arg(check.ratio, 0, 'f', 2));
		if (check.foregroundToken == QLatin1String("text"))
			weakestText = qMin(weakestText, check.ratio);
		else if (check.foregroundToken == QLatin1String("mutedText"))
			weakestMuted = qMin(weakestMuted, check.ratio);
		else if (check.foregroundToken == QLatin1String("selectionText"))
			selectionContrast = check.ratio;
	}
	const bool readable = colorsValid && SkinThemeData::passesReadability(tokens);

	{
		const QSignalBlocker blocker(colorTable);
		for (int row = 0; row < colorRows().size(); ++row)
		{
			const ColorRow& spec = colorRows()[row];
			const QColor swatch(tokens.*(spec.field));
			QTableWidgetItem* swatchItem = colorTable->item(row, 2);
			if (swatchItem != nullptr && swatch.isValid())
			{
				swatchItem->setBackground(swatch);
				swatchItem->setText(tr("Pick..."));
				const QString swatchInk = SkinThemeData::contrastRatio(tokens.text, swatch.name())
					>= SkinThemeData::contrastRatio(tokens.surface, swatch.name())
					? tokens.text : tokens.surface;
				swatchItem->setForeground(QColor(swatchInk));
			}
			if (QTableWidgetItem* valueItem = colorTable->item(row, 1))
			{
				if (QString::fromLatin1(spec.key) == QLatin1String("text"))
					valueItem->setToolTip(tr("Worst normal-text contrast: %1:1")
						.arg(weakestText, 0, 'f', 2));
				else if (QString::fromLatin1(spec.key) == QLatin1String("mutedText"))
					valueItem->setToolTip(tr("Worst support-text contrast: %1:1")
						.arg(weakestMuted, 0, 'f', 2));
			}
		}
	}

	previewWidget->setTheme(tokens, passed, checks.size());
	const QString mode = darkCheckBox->isChecked() ? tr("Dark mode") : tr("Light mode");
	const QString ratios = tr("Text %1:1  ·  Muted text %2:1  ·  Selection %3:1")
		.arg(weakestText, 0, 'f', 2).arg(weakestMuted, 0, 'f', 2)
		.arg(selectionContrast, 0, 'f', 2);
	const QString status = readable
		? tr("%1 is ready: %2 readability checks pass.").arg(mode).arg(passed)
		: (colorsValid
			? tr("%1 needs repair: %2 of %3 checks pass. %4")
				.arg(mode).arg(passed).arg(checks.size()).arg(failures.mid(0, 2).join(QStringLiteral(", ")))
			: tr("Fix invalid #RRGGBB values before previewing, saving, or exporting."));
	auditLabel->setText(QStringLiteral(
		"<span style=\"color:%1;font-weight:600;\">%2</span><br/>"
		"<span style=\"color:%3;\">%4</span><br/>"
		"<span style=\"color:%3;\">%5</span>")
		.arg((readable ? tokens.success : tokens.danger).toHtmlEscaped(),
			status.toHtmlEscaped(), tokens.mutedText.toHtmlEscaped(), ratios.toHtmlEscaped(),
			tr("Pick a swatch, reset one token, or repair text contrast before applying your custom theme.").toHtmlEscaped()));
}

void ThemeEditorDialog::previewInApplication()
{
	bool colorsValid = false;
	const SkinTokens tokens = tokensFromTable(&colorsValid);
	if (!colorsValid || !SkinThemeData::passesReadability(tokens))
	{
		QMessageBox::warning(this, tr("Theme Lab"),
			tr("Fix invalid or low-contrast colors before previewing."));
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
	if (!selectedSavedTheme(&theme))
		return;
	if (!SkinThemeData::passesReadability(CustomThemeStore::tokensForTheme(theme)))
	{
		QMessageBox::warning(this, tr("Theme Lab"),
			tr("This saved theme has low-contrast text. Load it, use Repair text contrast, then save it again."));
		return;
	}
	emit customThemeRequested(theme.skinId());
}

void ThemeEditorDialog::saveTheme()
{
	bool colorsValid = false;
	const SkinTokens tokens = tokensFromTable(&colorsValid);
	if (!colorsValid || !SkinThemeData::passesReadability(tokens))
	{
		QMessageBox::warning(this, tr("Theme Lab"),
			tr("Fix invalid or low-contrast colors before saving."));
		return;
	}

	QString defaultName = defaultThemeName(skinComboBox);
	CustomThemeStore::Theme selectedTheme;
	if (selectedSavedTheme(&selectedTheme))
		defaultName = selectedTheme.name;

	bool accepted = false;
	const QString name = QInputDialog::getText(this, tr("Save theme"),
		tr("Theme name:"), QLineEdit::Normal, defaultName, &accepted).trimmed();
	if (!accepted || name.isEmpty())
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
	const SkinTokens importedTokens = CustomThemeStore::tokensForTheme(theme);
	if (!SkinThemeData::passesReadability(importedTokens))
	{
		{
			const QSignalBlocker skinBlocker(skinComboBox);
			const QSignalBlocker darkBlocker(darkCheckBox);
			const int index = skinComboBox->findData(SkinThemeData::resolveId(theme.baseTheme));
			if (index >= 0)
				skinComboBox->setCurrentIndex(index);
			darkCheckBox->setChecked(theme.dark);
		}
		populateTable(importedTokens);
		updatePreviewPanel();
		QMessageBox::information(this, tr("Theme Lab"),
			tr("The imported colors are loaded for repair but were not saved. Use Repair text contrast, then Save as..."));
		return;
	}

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
	bool colorsValid = false;
	const SkinTokens tokens = tokensFromTable(&colorsValid);
	if (!colorsValid || !SkinThemeData::passesReadability(tokens))
	{
		QMessageBox::warning(this, tr("Theme Lab"),
			tr("Fix invalid or low-contrast colors before copying."));
		return;
	}
	QApplication::clipboard()->setText(themeJson(themeFromTable(defaultThemeName(skinComboBox))));
}

void ThemeEditorDialog::exportJson()
{
	bool colorsValid = false;
	const SkinTokens tokens = tokensFromTable(&colorsValid);
	if (!colorsValid || !SkinThemeData::passesReadability(tokens))
	{
		QMessageBox::warning(this, tr("Theme Lab"),
			tr("Fix invalid or low-contrast colors before exporting."));
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
