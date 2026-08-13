/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#pragma once

#include <QWidget>

#include "Editor/skins/ISkin.h"
#include "vst/VST3BusLayout.h"

class VSTBusSelector : public QWidget
{
	Q_OBJECT

public:
	explicit VSTBusSelector(bool output, QWidget* parent = nullptr);

	VST3BusLayout busLayout() const;
	void setBusLayout(VST3BusLayout layout);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

signals:
	void busLayoutPicked(VST3BusLayout layout);

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;

private:
	void openMenu();
	void refreshAccessibleValue();

	bool output = false;
	VST3BusLayout current = VST3BusLayout::Auto;
	bool hovered = false;
	bool pressed = false;
	bool menuOpen = false;
};

class VSTBusStrip : public QWidget
{
	Q_OBJECT

public:
	explicit VSTBusStrip(QWidget* parent = nullptr);

	VST3BusLayout inputLayout() const;
	VST3BusLayout outputLayout() const;
	void setBusLayouts(VST3BusLayout input, VST3BusLayout output);
	void setSelectorsEnabled(bool enabled, const QString& disabledReason = QString());
	void setVerdict(const QString& text, VstBusFrameState::Tone tone);
	void setVerdictPair(const QString& input, const QString& output, VstBusFrameState::Tone tone);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

signals:
	void busLayoutsPicked(VST3BusLayout input, VST3BusLayout output);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	VstBusFrameState frameState() const;
	void relayout();
	int verdictWidth() const;

	VSTBusSelector* inputSelector = nullptr;
	VSTBusSelector* outputSelector = nullptr;
	QString verdictText;
	QString verdictInputText;
	QString verdictOutputText;
	VstBusFrameState::Tone verdictTone = VstBusFrameState::Tone::Neutral;
};
