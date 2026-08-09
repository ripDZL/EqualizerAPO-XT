/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#pragma once

#include <QKeyEvent>
#include <QLineEdit>

// The add-channel fields commit their text from editingFinished().  Hiding a
// field during that callback can otherwise let the same Return reach the
// containing QDialog and activate one of its actions.  Consume Return only
// after the field has handled it; every other dialog control keeps Qt's normal
// keyboard behavior.
class RoutingAddChannelEditor final : public QLineEdit
{
public:
	explicit RoutingAddChannelEditor(QWidget* parent = nullptr)
		: QLineEdit(parent)
	{
	}

protected:
	void keyPressEvent(QKeyEvent* event) override
	{
		QLineEdit::keyPressEvent(event);
		if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
			event->accept();
	}
};
