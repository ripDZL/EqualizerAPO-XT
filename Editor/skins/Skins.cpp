/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "Skins.h"
#include "shared/SkinSupport.h"
#include "SkinThemeData.h"
#include "services/logging/Logging.h"

#include <QHash>

namespace
{
class DelegatingSkin final : public ISkin
{
public:
	DelegatingSkin(const QString& skinId, ISkin* delegate)
		: skinId(skinId), delegate(delegate)
	{
	}

	QString id() const override
	{
		return skinId;
	}

	IRoutingRenderer* routingRenderer() const override
	{
		return delegate->routingRenderer();
	}

	void paintKnob(QPainter& painter, const QRect& rect, const KnobState& state,
		const SkinTokens& tokens) const override
	{
		delegate->paintKnob(painter, rect, state, tokens);
	}

	QString cardFrameStyle(const CommandRowInfo& info, const SkinTokens& tokens) const override
	{
		return delegate->cardFrameStyle(info, tokens);
	}

	QString cardHeaderStyle(const CommandRowInfo& info, const SkinTokens& tokens) const override
	{
		return delegate->cardHeaderStyle(info, tokens);
	}

	BadgeTreatment badgeTreatment(const CommandRowInfo& info, const QString& typeColor,
		const QString& badgeToken, const SkinTokens& tokens) const override
	{
		return delegate->badgeTreatment(info, typeColor, badgeToken, tokens);
	}

	void prepareCommandRow(const CommandRowInfo& info, QWidget* card, QWidget* header,
		QWidget* body, const SkinTokens& tokens) const override
	{
		delegate->prepareCommandRow(info, card, header, body, tokens);
	}

	void paintCardChrome(QPainter& painter, const QRect& rect, const CommandRowInfo& info,
		const SkinTokens& tokens) const override
	{
		delegate->paintCardChrome(painter, rect, info, tokens);
	}

	bool paintScopeGutter(QPainter& painter, const QSize& size, const CommandRowInfo& info,
		const SkinTokens& tokens) const override
	{
		return delegate->paintScopeGutter(painter, size, info, tokens);
	}

	bool logicSiblingsIndentAsMembers() const override
	{
		return delegate->logicSiblingsIndentAsMembers();
	}

	void paintAddRow(QPainter& painter, const QRect& rect, const ListChromeState& state,
		const SkinTokens& tokens) const override
	{
		delegate->paintAddRow(painter, rect, state, tokens);
	}

	void paintInsertSeam(QPainter& painter, const QRect& rect, const ListChromeState& state,
		const SkinTokens& tokens) const override
	{
		delegate->paintInsertSeam(painter, rect, state, tokens);
	}

	void paintGraphicEqPlot(QPainter& painter, const GraphicEQPlotState& state,
		const SkinTokens& tokens) const override
	{
		delegate->paintGraphicEqPlot(painter, state, tokens);
	}

	void paintAnalysisGraph(QPainter& painter, const AnalysisGraphState& state,
		const SkinTokens& tokens) const override
	{
		delegate->paintAnalysisGraph(painter, state, tokens);
	}

	void paintSegmentedControl(QPainter& painter, const SegmentedControlState& state,
		const SkinTokens& tokens) const override
	{
		delegate->paintSegmentedControl(painter, state, tokens);
	}

	void paintVstBusSelector(QPainter& painter, const VstBusSelectorState& state,
		const SkinTokens& tokens) const override
	{
		delegate->paintVstBusSelector(painter, state, tokens);
	}

	void paintVstBusFrame(QPainter& painter, const VstBusFrameState& state,
		const SkinTokens& tokens) const override
	{
		delegate->paintVstBusFrame(painter, state, tokens);
	}

	FilterPickerView* createFilterPicker(QWidget* parent, const SkinTokens& tokens) const override
	{
		return delegate->createFilterPicker(parent, tokens);
	}

	ReferenceCardView* createReferenceCardView(const QString& kind, QWidget* parent,
		const SkinTokens& tokens) const override
	{
		return delegate->createReferenceCardView(kind, parent, tokens);
	}

	SubwooferRoutingCardView* createSubwooferRoutingCardView(QWidget* parent,
		const SkinTokens& tokens) const override
	{
		return delegate->createSubwooferRoutingCardView(parent, tokens);
	}

	void paintTitleBarChrome(QPainter& painter, const QRect& rect, const SkinTokens& tokens) const override
	{
		delegate->paintTitleBarChrome(painter, rect, tokens);
	}

	void styleMainToolbar(QToolBar* toolBar, const SkinTokens& tokens) const override
	{
		delegate->styleMainToolbar(toolBar, tokens);
	}

	void styleFileDialog(QFileDialog* dialog, const SkinTokens& tokens) const override
	{
		delegate->styleFileDialog(dialog, tokens);
	}

private:
	QString skinId;
	ISkin* delegate = nullptr;
};

ISkin* baseImplementationFor(const QString& id)
{
	if (id == QStringLiteral("studio"))
		return studioSkin();
	if (id == QStringLiteral("minimal"))
		return minimalSkin();
	if (id == QStringLiteral("soft"))
		return softSkin();
	if (id == QStringLiteral("rack"))
		return rackSkin();
	if (id == QStringLiteral("matrix"))
		return matrixSkin();
	return nullptr;
}

ISkin* delegatingImplementationFor(const QString& id, ISkin* delegate)
{
	static QHash<QString, DelegatingSkin*> variants;
	if (variants.contains(id))
		return variants.value(id);

	DelegatingSkin* skin = new DelegatingSkin(id, delegate);
	variants.insert(id, skin);
	return skin;
}

// The ISkin half of the roster: which class implements each id. The membership
// and the order come from SkinThemeData::roster(), so this table only answers
// "who paints it" - and an id in the roster with no implementation here is
// reported rather than silently drawn as Studio, which is what the old
// hand-written list allowed.
ISkin* implementationFor(const QString& id)
{
	const SkinThemeData::SkinEntry& entry = SkinThemeData::entry(id);
	ISkin* base = baseImplementationFor(entry.paintBaseId);
	if (base == nullptr)
		return nullptr;
	return entry.paintBaseId == entry.id ? base : delegatingImplementationFor(entry.id, base);
}
}

namespace Skins
{
QList<ISkin*> all()
{
	QList<ISkin*> result;
	for (const QString& id : SkinThemeData::ids())
	{
		ISkin* skin = implementationFor(id);
		if (skin != nullptr)
			result.append(skin);
		else
			LogFStatic(L"skin \"%s\" is in the roster but has no ISkin implementation", id.toStdWString().c_str());
	}
	return result;
}

ISkin* byId(const QString& id)
{
	// Alias resolution and the studio fallback live in SkinThemeData so
	// satellite tools resolve stored ids exactly like the Editor.
	const QString resolved = SkinThemeData::resolveId(id);
	ISkin* skin = implementationFor(resolved);
	return skin != nullptr ? skin : studioSkin();
}
}
