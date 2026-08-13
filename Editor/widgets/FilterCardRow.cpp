#include "FilterCardRow.h"

#include <QAbstractButton>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include <utility>

#include "Editor/SkinManager.h"
#include "Editor/widgets/ChBadge.h"
#include "Editor/widgets/ElidedLabel.h"
#include "Editor/widgets/routing/IRoutingRenderer.h"
#include "Editor/widgets/routing/CopyRoutingAdapter.h"

namespace
{
// Row types the engine narrows to the enclosing Channel: selection (every
// downstream filter initializes against the selected channel set), so an
// inherited scope badge on them states a fact. Channel/Copy rows carry their
// own channel badges, and control rows (device, stage, eval, the If family),
// notes and unknown raw text are not gated by the selection at all.
bool channelSelectionGatesType(const QString& type)
{
	return type == QStringLiteral("biquad")
		|| type == QStringLiteral("preamp")
		|| type == QStringLiteral("delay")
		|| type == QStringLiteral("graphiceq")
		|| type == QStringLiteral("convolution")
		|| type == QStringLiteral("velvet")
		|| type == QStringLiteral("vst")
		|| type == QStringLiteral("loudness")
		|| type == QStringLiteral("include");
}
}

FilterCardRow::FilterCardRow(FilterTable* table, int number, FilterTable::Item* item, IFilterGUI* gui,
	FilterCardDescriptor preparedDescriptor, QWidget* parent)
	: QWidget(parent), table(table), item(item), gui(gui), descriptor(std::move(preparedDescriptor)), rowNumber(number)
{
	setAttribute(Qt::WA_StyledBackground, false);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	QVBoxLayout* outerLayout = new QVBoxLayout(this);
	outerLayout->setContentsMargins(8 + rowIndentUnits() * SkinManager::instance()->tokens().channelGroupIndent, 4, 8, 4);
	outerLayout->setSpacing(0);
	// Detach the layout's minimumSize from the children's: the body can contain
	// legacy filter GUIs with huge content-driven sizeHints (DeviceFilterGUI,
	// VST, Convolution, ...) and SetMinimumSize would propagate that up here
	// and force every cell in FilterTable's grid to that width.
	outerLayout->setSizeConstraint(QLayout::SetNoConstraint);

	cardFrame = new CommandRowFrame(this);
	cardFrame->setObjectName(QStringLiteral("FilterCardRow"));
	cardFrame->setAttribute(Qt::WA_StyledBackground, true);
	cardFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	outerLayout->addWidget(cardFrame);

	QVBoxLayout* cardLayout = new QVBoxLayout(cardFrame);
	cardLayout->setContentsMargins(0, 0, 0, 0);
	cardLayout->setSpacing(0);

	headerWidget = new QWidget(cardFrame);
	headerWidget->setObjectName(QStringLiteral("FilterCardHeader"));
	headerWidget->setAttribute(Qt::WA_StyledBackground, true);
	headerWidget->setMinimumHeight(SkinManager::instance()->tokens().rowHeight);
	cardLayout->addWidget(headerWidget);

	// Dress the frame and header now, while the card is two widgets deep:
	// setStyleSheet() re-resolves the style of every descendant, so applying
	// these sheets to the finished ~40-widget card (the applyDescriptor() call
	// at the end of this constructor used to be the first apply) re-styled the
	// whole subtree per card - measured ~3 ms each on a 300-row config.
	// Children created below inherit the sheets as they appear, and the
	// applyDescriptor() re-run finds identical strings and properties, so it
	// skips both the re-set and the construction-time repolish entirely.
	syncVisualState();

	QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
	headerLayout->setContentsMargins(8, 4, 8, 4);
	headerLayout->setSpacing(8);

	expandButton = new QToolButton(headerWidget);
	expandButton->setObjectName(QStringLiteral("FilterCardIconButton"));
	expandButton->setCheckable(true);
	expandButton->setChecked(gui != nullptr);
	expandButton->setText(expandButton->isChecked() ? QStringLiteral("v") : QStringLiteral(">"));
	expandButton->setToolTip(tr("Expand filter card"));
	connect(expandButton, SIGNAL(toggled(bool)), this, SLOT(expandedToggled(bool)));
	headerLayout->addWidget(expandButton);

	numberLabel = new QLabel(QString::number(number), headerWidget);
	numberLabel->setObjectName(QStringLiteral("FilterCardNumber"));
	numberLabel->setAlignment(Qt::AlignCenter);
	numberLabel->setMinimumWidth(28);
	headerLayout->addWidget(numberLabel);

	typeBadge = new QLabel(headerWidget);
	typeBadge->setObjectName(QStringLiteral("FilterTypeBadge"));
	typeBadge->setAlignment(Qt::AlignCenter);
	typeBadge->setMinimumWidth(46);
	headerLayout->addWidget(typeBadge);

	titleLabel = new ElidedLabel(headerWidget);
	titleLabel->setObjectName(QStringLiteral("FilterCardTitle"));
	titleLabel->setElideMode(Qt::ElideRight);
	titleLabel->setMinimumWidth(92);
	titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	headerLayout->addWidget(titleLabel);

	summaryLabel = new ElidedLabel(headerWidget);
	summaryLabel->setObjectName(QStringLiteral("FilterCardSummary"));
	summaryLabel->setElideMode(Qt::ElideRight);
	// No text interaction here: a selectable label consumes the mouse press,
	// and since this label is the header's expanding filler, that reduced the
	// row's drag/select surface to the narrow number/title strip. The header
	// is the drag handle (FilterTable hit-tests getHeaderRect); the raw line
	// below stays selectable for copying.
	summaryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	headerLayout->addWidget(summaryLabel, 1);

	channelBadgeContainer = new QWidget(headerWidget);
	// A plain QWidget matches every skin's global "QWidget { background: @BG@ }"
	// rule, and the style then paints that app-background plate over the card
	// surface - a dark rectangle cut around the badges. The widget-level rule
	// outranks any skin sheet, so the strip stays transparent under all skins.
	channelBadgeContainer->setStyleSheet(QStringLiteral("background: transparent;"));
	channelBadgeLayout = new QHBoxLayout(channelBadgeContainer);
	channelBadgeLayout->setContentsMargins(0, 0, 0, 0);
	channelBadgeLayout->setSpacing(3);
	channelBadgeContainer->setVisible(false);
	headerLayout->addWidget(channelBadgeContainer);

	enabledButton = new QToolButton(headerWidget);
	enabledButton->setObjectName(QStringLiteral("FilterCardIconButton"));
	enabledButton->setCheckable(true);
	enabledButton->setToolTip(tr("Enable or comment out this command"));
	enabledButton->setChecked(descriptor.enabled);
	enabledButton->setIcon(QIcon(descriptor.enabled ? QStringLiteral(":/icons/power_on.svg") : QStringLiteral(":/icons/power_off.svg")));
	connect(enabledButton, SIGNAL(toggled(bool)), this, SLOT(enabledToggled(bool)));
	headerLayout->addWidget(enabledButton);

	addButton = new QToolButton(headerWidget);
	addButton->setObjectName(QStringLiteral("FilterCardIconButton"));
	addButton->setText(QStringLiteral("+"));
	addButton->setToolTip(tr("Add filter above this card"));
	connect(addButton, SIGNAL(clicked()), this, SLOT(addAbove()));
	headerLayout->addWidget(addButton);

	removeButton = new QToolButton(headerWidget);
	removeButton->setObjectName(QStringLiteral("FilterCardIconButton"));
	removeButton->setText(QStringLiteral("-"));
	removeButton->setToolTip(tr("Remove filter"));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(removeThis()));
	headerLayout->addWidget(removeButton);

	editButton = new QToolButton(headerWidget);
	editButton->setObjectName(QStringLiteral("FilterCardIconButton"));
	editButton->setCheckable(true);
	editButton->setText(QStringLiteral("..."));
	editButton->setToolTip(tr("Edit raw command"));
	connect(editButton, SIGNAL(toggled(bool)), this, SLOT(editTextToggled(bool)));
	headerLayout->addWidget(editButton);

	rawPreviewLabel = new QLabel(cardFrame);
	rawPreviewLabel->setObjectName(QStringLiteral("FilterCardRawPreview"));
	rawPreviewLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	cardLayout->addWidget(rawPreviewLabel);

	bodyStack = new QStackedWidget(cardFrame);
	bodyStack->setObjectName(QStringLiteral("FilterCardBody"));
	bodyStack->setAttribute(Qt::WA_StyledBackground, true);
	// Stop an overgrown body editor (e.g. the legacy DeviceFilterGUI's
	// QTreeWidget with sizeAdjustPolicy=AdjustToContents, or any other GUI that
	// reports a content-driven sizeHint) from propagating its preferred width
	// up through the card. Otherwise the card grows past the visible viewport
	// and the right-side header toolbar (enable / + / - / ...) renders
	// thousands of pixels off screen and becomes invisible.
	// Ignored sizePolicy alone is not enough: it stops sizeHint propagation
	// but the layout system still inherits the inner widgets' minimumSize.
	// Pin the bodyStack's own minimumSize to 0 so the card frame's minimum
	// width is driven by the header only.
	bodyStack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	bodyStack->setMinimumSize(0, 0);
	cardLayout->addWidget(bodyStack);

	lineEdit = new QLineEdit(bodyStack);
	lineEdit->setObjectName(QStringLiteral("FilterCardRawEditor"));
	connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditingFinished()));
	bodyStack->addWidget(lineEdit);

	// A Copy line with inline `expression` factors must not open the routing
	// view (FilterCardModel::opensRoutingView): such lines keep the raw body
	// (dynamic-value contract), like every other dynamic line without a
	// dynamic-capable editor.
	IRoutingRenderer* routingRenderer = FilterCardModel::opensRoutingView(descriptor)
		? SkinManager::instance()->routingRenderer() : nullptr;

	if (routingRenderer != nullptr)
	{
		// Skin-specific Copy routing view (crosspoint matrix, step list, ...).
		// The view owns its working routing state; on edit we serialise it
		// back into item->text.
		QWidget* editorContainer = new QWidget(bodyStack);
		editorContainer->setObjectName(QStringLiteral("FilterCardEditor"));
		editorContainer->setAttribute(Qt::WA_StyledBackground, true);
		editorContainer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		editorContainer->setMinimumSize(0, 0);
		QVBoxLayout* editorLayout = new QVBoxLayout(editorContainer);
		editorLayout->setContentsMargins(12, 10, 12, 12);

		std::vector<Assignment> routingAssignments = CopyRoutingAdapter::parse(descriptor.parameters);
		// Seed the routing editor with the real device channel set (L, R, C, ...),
		// the same list the legacy CopyFilterGUI receives via configureChannels.
		// Without it the graph only shows channels already named in the line, so
		// the user cannot route to/from a channel that has no assignment yet
		// (e.g. copying L onto R when R is not referenced).
		std::vector<std::wstring> channelNames = table->getChannelNames();
		// Copy uses the default port model: symmetric sources/targets seeded
		// from the device channels, with editable factors.
		routingView = routingRenderer->create(routingAssignments, channelNames, RoutingPortModel(), editorContainer);

		QScrollArea* routingScroll = new QScrollArea(editorContainer);
		routingScroll->setObjectName(QStringLiteral("FilterCardEditorScroll"));
		routingScroll->setFrameShape(QFrame::NoFrame);
		routingScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		routingScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		routingScroll->setWidgetResizable(true);
		routingScroll->setMinimumSize(0, 0);
		routingScroll->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		routingScroll->setWidget(routingView);
		watchEditorScroll(routingScroll);
		editorLayout->addWidget(routingScroll);

		bodyStack->addWidget(editorContainer);
		bodyStack->setCurrentWidget(editorContainer);
		connect(routingView, SIGNAL(routingChanged()), this, SLOT(routingEdited()));
		connect(routingView, SIGNAL(routingChanged()), table, SLOT(updateChannels()));

		// The expand default above keys off the gui the body shows; routing
		// rows are their own editor and start open.
		expandButton->setChecked(true);
	}
	else if (gui != nullptr)
	{
		QWidget* editorContainer = new QWidget(bodyStack);
		editorContainer->setObjectName(QStringLiteral("FilterCardEditor"));
		editorContainer->setAttribute(Qt::WA_StyledBackground, true);
		// Match bodyStack: stop overgrown filter GUIs from inflating the card width.
		editorContainer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		editorContainer->setMinimumSize(0, 0);
		QVBoxLayout* editorLayout = new QVBoxLayout(editorContainer);
		editorLayout->setContentsMargins(12, 10, 12, 12);
		// Wrap the legacy filter GUI in a borderless scroll area. Without this,
		// any filter GUI that has a content-driven sizeHint (DeviceFilterGUI's
		// QTreeWidget AdjustToContents, GraphicEQ / Convolution / VSTPlugin's
		// internal AdjustToContents widgets) propagates a huge minimumSize up
		// through editorContainer/bodyStack/cardFrame/FilterCardRow and the
		// QGridLayout in FilterTable then forces every card column to that
		// width, pushing the right-side header toolbar (enable / + / - / ...)
		// thousands of pixels off screen.
		QWidget* guiWidget = qobject_cast<QWidget*>(gui);
		if (guiWidget != nullptr)
		{
			QScrollArea* guiScroll = new QScrollArea(editorContainer);
			guiScroll->setObjectName(QStringLiteral("FilterCardEditorScroll"));
			guiScroll->setFrameShape(QFrame::NoFrame);
			guiScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
			guiScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			guiScroll->setWidgetResizable(true);
			guiScroll->setMinimumSize(0, 0);
			guiScroll->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
			guiScroll->setWidget(guiWidget);
			watchEditorScroll(guiScroll);
			editorLayout->addWidget(guiScroll);
		}
		else
		{
			editorLayout->addWidget(gui);
		}
		bodyStack->addWidget(editorContainer);
		bodyStack->setCurrentWidget(editorContainer);
		connect(gui, SIGNAL(updateModel()), this, SLOT(updateModel()));
	}
	else
	{
		// Unrecognized command line: no dedicated editor. Present a styled
		// monospace raw card with the command token emphasized, so it reads
		// as a deliberate raw row instead of bare plain text.
		const SkinTokens& tk = SkinManager::instance()->tokens();
		QWidget* rawContainer = new QWidget(bodyStack);
		rawContainer->setObjectName(QStringLiteral("FilterCardEditor"));
		rawContainer->setAttribute(Qt::WA_StyledBackground, true);
		rawContainer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		rawContainer->setMinimumSize(0, 0);
		QHBoxLayout* rawLayout = new QHBoxLayout(rawContainer);
		rawLayout->setContentsMargins(12, 10, 12, 12);
		rawLayout->setSpacing(10);

		QLabel* glyph = new QLabel(QStringLiteral(">_"), rawContainer);
		glyph->setObjectName(QStringLiteral("FilterCardRawGlyph"));
		glyph->setStyleSheet(QStringLiteral("color:%1; font-family:\"%2\"; font-weight:700;")
			.arg(tk.mutedText, tk.monoFontFamily));
		rawLayout->addWidget(glyph, 0, Qt::AlignTop);

		QLabel* rawLaß~½¶‰žËkºwµçTÍ…µ”¡¥¹ÑÌÑ¡”Ý¥‘Ñ ±…µÀ•á¥ÍÑÌ™½È¤ì„($¼¼ÉÕ¹…Ý…ä‰½‘äµÕÍÐ¹½ÐÍÝ…±±½ÜÑ¡”Ý¡½±”Ñ…‰±”¸(%‘•Í¥É•€ôÅ	½Õ¹ ÈÐ°‘•Í¥É•°€ØÀÀ¤ì(%¥˜€¡ÍÉ½±°´ùµ¥¹¥µÕµ!•¥¡Ð ¤€„ô‘•Í¥É•ñðÍÉ½±°´ùµ…á¥µÕµ!•¥¡Ð ¤€„ô‘•Í¥É•¤(%ì($%ÍÉ½±°´ùÍ•Ñ¥á•‘!•¥¡Ð¡‘•Í¥É•¤ì($$¼¼Q¡”¹•Ü¡•¥¡ÐµÕÍÐÉ•… Ñ¡”¥±Ñ•ÉQ…‰±”É¥°‰ÕÐÑ¡”±…å½ÕÐ($$¼¼¡…¥¸…‰½Ù”Ñ¡”ÍÉ½±°€¡•‘¥Ñ½È½¹Ñ…¥¹•È€´ø‰½‘äÍÑ…¬€´ø…É($$¼¼™É…µ”€´øÉ½Ü¤É”µÙ…±¥‘…Ñ•Ì±…é¥±ä¡½À‰ä¡½À…¹Ñ¡”…Í…‘”…¸($$¼¼ÍÑ…±°Ý¥Ñ „ÍÑ…±”…¡•µ¥¹¥µÕ´µ¥µ¡…¥¸€´½‰Í•ÉÙ•…Ì€‰Ñ¡”($$¼¼…É¹•Ù•ÈÉ½ÝÌˆÝ¡•¸Ñ¡”É½ÕÑ¥¹œÙ¥•ÝÌœ¡…¹¹•°™½±•áÁ…¹‘Ì¸($$¼¼%¹Ù…±¥‘…Ñ”Ñ¡”Ý¡½±”¡…¥¸•áÁ±¥¥Ñ±äÍ¼Ñ¡”¹•áÐÉ¥Á…ÍÌ($$¼¼É”µÅÕ•É¥•Ì„™É•Í É½Üµ¥¹¥µÕ´¸($%™½È€¡E]¥‘•Ð¨Ü€ôÍÉ½±°´ùÁ…É•¹Ñ]¥‘•Ð ¤ìÜ€„ô¹Õ±±ÁÑÈ€˜˜Ü€„ôÑ¡¥ÌìÜ€ôÜ´ùÁ…É•¹Ñ]¥‘•Ð ¤¤($$%¥˜€¡Ü´ù±…å½ÕÐ ¤€„ô¹Õ±±ÁÑÈ¤($$$%Ü´ù±…å½ÕÐ ¤´ù¥¹Ù…±¥‘…Ñ” ¤ì($%¥˜€¡±…å½ÕÐ ¤€„ô¹Õ±±ÁÑÈ¤($$%±…å½ÕÐ ¤´ù¥¹Ù…±¥‘…Ñ” ¤ì($%ÕÁ‘…Ñ••½µ•ÑÉä ¤ì(%ô)ô()‰½½°¥±Ñ•É…É‘I½Üèé•Ù•¹Ñ¥±Ñ•È¡E=‰©•Ð¨Ý…Ñ¡•°EÙ•¹Ð¨•Ù•¹Ð¤)ì(%ÍÝ¥Ñ €¡•Ù•¹Ð´ùÑåÁ” ¤¤(%ì(%…Í”EÙ•¹Ðèé¡¥±‘A½±¥Í¡•è(%ì($%E¡¥±‘Ù•¹Ð¨¡¥±‘Ù•¹Ð€ôÍÑ…Ñ¥}…ÍÐñE¡¥±‘Ù•¹Ð¨ø¡•Ù•¹Ð¤ì($%Ý…Ñ¡A½¥¹Ñ•ÉM•±•Ñ¥½¸¡Å½‰©•Ñ}…ÍÐñE]¥‘•Ð¨ø¡¡¥±‘Ù•¹Ð´ù¡¥± ¤¤¤ì($%‰É•…¬ì(%ô(%…Í”EÙ•¹Ðèé5½ÕÍ•	ÕÑÑ½¹AÉ•ÍÌè(%ì($%E5½ÕÍ•Ù•¹Ð¨µ½ÕÍ•Ù•¹Ð€ôÍÑ…Ñ¥}…ÍÐñE5½ÕÍ•Ù•¹Ð¨ø¡•Ù•¹Ð¤ì($%¥˜€¡µ½ÕÍ•Ù•¹Ð´ù‰ÕÑÑ½¸ ¤€ôôEÐèé1•™Ñ	ÕÑÑ½¸($$$˜˜µ½ÕÍ•Ù•¹Ð´ùµ½‘¥™¥•ÉÌ ¤€ôôEÐèé9½5½‘¥™¥•È($$$˜˜Ñ…‰±”€„ô¹Õ±±ÁÑÈ€˜˜¥Ñ•´€„ô¹Õ±±ÁÑÈ¤($%ì($$%Ñ…‰±”´ùÍ•±•Ñ=¹±åÉ½µ…É¡¥Ñ•´¤ì($%ô($%‰É•…¬ì(%ô(%…Í”EÙ•¹ÐèéI•Í¥é”è(%…Í”EÙ•¹ÐèéM¡½Üè(%…Í”EÙ•¹Ðèé1…å½ÕÑI•ÅÕ•ÍÐè(%ì($$¼¼]…Ñ¡•½‰©•ÑÌ…É”Ñ¡”•‘¥Ñ½ÈÍÉ½±°¥ÑÍ•±˜…¹¥ÑÌ½¹Ñ•¹ÐÝ¥‘•Ð($$¼¼€¡Ý¡½Í”Á…É•¹Ð¡…¥¸¥Ì½¹Ñ•¹Ð€´øÙ¥•ÝÁ½ÉÐ€´øÍÉ½±°¤¸($%EMÉ½±±É•„¨ÍÉ½±°€ôÅ½‰©•Ñ}…ÍÐñEMÉ½±±É•„¨ø¡Ý…Ñ¡•¤ì($%¥˜€¡ÍÉ½±°€ôô¹Õ±±ÁÑÈ€˜˜Ý…Ñ¡•´ùÁ…É•¹Ð ¤€„ô¹Õ±±ÁÑÈ¤($$%ÍÉ½±°€ôÅ½‰©•Ñ}…ÍÐñEMÉ½±±É•„¨ø¡Ý…Ñ¡•´ùÁ…É•¹Ð ¤´ùÁ…É•¹Ð ¤¤ì($%¥˜€¡ÍÉ½±°€„ô¹Õ±±ÁÑÈ¤($$%Íå¹‘¥Ñ½ÉMÉ½±±!•¥¡Ð¡ÍÉ½±°¤ì($%‰É•…¬ì(%ô(%‘•™…Õ±Ðè($%‰É•…¬ì(%ô(%É•ÑÕÉ¸E]¥‘•Ðèé•Ù•¹Ñ¥±Ñ•È¡Ý…Ñ¡•°•Ù•¹Ð¤ì)ô()¥¹Ð¥±Ñ•É…É‘I½ÜèéÉ½Ý%¹‘•¹ÑU¹¥ÑÌ ¤½¹ÍÐ)ì($¼¼5•µ‰•È±•Ù•°¥Ì½¹”Õ¹¥ÐÁ…ÍÐÑ¡”‰É…¹ ½Ñ…¥°É½ÜÌ½Ý¸Í•µ…¹Ñ¥Œ±•Ù•°($¼¼€¡‘•ÁÑ …±É•…‘ä…ÉÉ¥•Ì…¹ä•¹±½Í¥¹œ¡…¹¹•°É½ÕÀ°Í¼€¬Ä­••ÁÌµ¥á•($¼¼¡…¹¹•°à%˜¹•ÍÑ¥¹œ…±¥¹•Ý¥Ñ Ñ¡”‰±½¬µ•µ‰•ÉÌ¤¸(%¥˜€¡‘•ÍÉ¥ÁÑ½È¹ÑåÁ”€ôôEMÑÉ¥¹1¥Ñ•É…° ‰¥˜ˆ¤€˜˜‘•ÍÉ¥ÁÑ½È¹‰…‘”€„ôEMÑÉ¥¹1¥Ñ•É…° ‰%ˆ¤($$˜˜M­¥¹5…¹…•Èèé¥¹ÍÑ…¹” ¤´ù±½¥M¥‰±¥¹Í%¹‘•¹ÑÍ5•µ‰•ÉÌ ¤¤($%É•ÑÕÉ¸‘•ÍÉ¥ÁÑ½È¹‘•ÁÑ €¬€Äì(%É•ÑÕÉ¸‘•ÍÉ¥ÁÑ½È¹‘•ÁÑ ì)ô()Ù½¥¥±Ñ•É…É‘I½ÜèéÁ…¥¹ÑÙ•¹Ð¡EA…¥¹ÑÙ•¹Ð¨¤)ì($¼¼Q¡”…Ñ¥Ù”Í­¥¸µ…ä½Ý¸Ñ¡”Ý¡½±”Í½Á”ÕÑÑ•È€¡Ñ¡”%˜µ‰±½¬±…¹•Ì¤ì($¼¼Ñ¡”Í¡…É•¡…¹¹•°É…¥°‰•±½ÜÍÑ…åÌÑ¡”‘•™…Õ±Ð™½ÈÍ­¥¹ÌÑ¡…Ð‘¼¹½Ð($¼¼…¹ÍÝ•È¸(%ì($%EA…¥¹Ñ•ÈÕÑÑ•ÉA…¥¹Ñ•È¡Ñ¡¥Ì¤ì($%¥˜€¡M­¥¹5…¹…•Èèé¥¹ÍÑ…¹” ¤´ùÁ…¥¹ÑM½Á•ÕÑÑ•È¡ÕÑÑ•ÉA…¥¹Ñ•È°Í¥é” ¤°Ù¥ÍÕ…±%¹™¼¤¤($$%É•ÑÕÉ¸ì(%ô((%½¹ÍÐM­¥¹Q½­•¹Ì˜Ñ½­•¹Ì€ôM­¥¹5…¹…•Èèé¥¹ÍÑ…¹” ¤´ùÑ½­•¹Ì ¤ì(%¥˜€¡‘•ÍÉ¥ÁÑ½È¹‘•ÁÑ €ðô€À¤($%É•ÑÕÉ¸ì((%EA…¥¹Ñ•ÈÁ…¥¹Ñ•È¡Ñ¡¥Ì¤ì(%Á…¥¹Ñ•È¹Í•ÑI•¹‘•É!¥¹Ð¡EA…¥¹Ñ•Èèé¹Ñ¥…±¥…Í¥¹œ¤ì(%E½±½È½±½È¡‘•ÍÉ¥ÁÑ½È¹½±½È¤ì(%¥¹Ð¥¹‘•¹Ð€ô€à€¬€¡‘•ÍÉ¥ÁÑ½È¹‘•ÁÑ €´€Ä¤€¨Ñ½­•¹Ì¹¡…¹¹•±É½ÕÁ%¹‘•¹Ðì(%EI•Ð±¥¹•I•Ð¡¥¹‘•¹Ð°€À°Ñ½­•¹Ì¹¡…¹¹•±É½ÕÁ%¹‘•¹Ð°¡•¥¡Ð ¤¤ì((%ÍÝ¥Ñ €¡Ñ½­•¹Ì¹¡…¹¹•±É½ÕÁMÑå±”¤(%ì(%…Í”M­¥¹Q½­•¹ÌèéQÉ••1¥¹•Ìè($%Á…¥¹Ñ•È¹Í•ÑA•¸¡EA•¸¡E½±½È¡Ñ½­•¹Ì¹‰½É‘•È¤°€Ä¤¤ì($%Á…¥¹Ñ•È¹‘É…Ý1¥¹”¡±¥¹•I•Ð¹±•™Ð ¤€¬€Ü°€À°±¥¹•I•Ð¹±•™Ð ¤€¬€Ü°¡•¥¡Ð ¤¤ì($%Á…¥¹Ñ•È¹‘É…ÝQ•áÐ¡EI•Ð¡±¥¹•I•Ð¹±•™Ð ¤€¬€Ä°€À°€ÄØ°Ñ½­•¹Ì¹É½Ý!•¥¡Ð¤°EÐèé±¥¹•¹Ñ•È°EMÑÉ¥¹1¥Ñ•É…° ‰ðˆ¤¤ì($%‰É•…¬ì(%…Í”M­¥¹Q½­•¹Ìèé½ÑÑ•‘1¥¹”è($%Á…¥¹Ñ•È¹Í•ÑA•¸¡EA•¸¡½±½È°€Ä°EÐèé½Ñ1¥¹”¤¤ì($%Á…¥¹Ñ•È¹‘É…Ý1¥¹”¡±¥¹•I•Ð¹±•™Ð ¤€¬€Ô°€À°±¥¹•I•Ð¹±•™Ð ¤€¬€Ô°¡•¥¡Ð ¤¤ì($%‰É•…¬ì(%…Í”M­¥¹Q½­•¹ÌèéM½™ÑM¡…‘½Üè(%ì($%E1¥¹•…ÉÉ…‘¥•¹ÐÍ¡…‘½Ü¡±¥¹•I•Ð¹±•™Ð ¤°€À°±¥¹•I•Ð¹É¥¡Ð ¤°€À¤ì($%E½±½ÈÍÑ…ÉÐ€ô½±½Èì($%ÍÑ…ÉÐ¹Í•Ñ±Á¡„ Ìà¤ì($%E½±½È•¹€ô½±½Èì($%•¹¹Í•Ñ±Á¡„ À¤ì($%Í¡…‘½Ü¹Í•Ñ½±½ÉÐ À°ÍÑ…ÉÐ¤ì($%Í¡…‘½Ü¹Í•Ñ½±½ÉÐ Ä°•¹¤ì($%Á…¥¹Ñ•È¹™¥±±I•Ð¡±¥¹•I•Ð°Í¡…‘½Ü¤ì($%‰É•…¬ì(%ô(%…Í”M­¥¹Q½­•¹ÌèéÉ…‘¥•¹Ñ	…Èè(%‘•™…Õ±Ðè(%ì($%E1¥¹•…ÉÉ…‘¥•¹ÐÉ…‘¥•¹Ð¡±¥¹•I•Ð¹±•™Ð ¤°€À°±¥¹•I•Ð¹±•™Ð ¤°¡•¥¡Ð ¤¤ì($%E½±½ÈÍÑ…ÉÐ€ô½±½Èì($%ÍÑ…ÉÐ¹Í•Ñ±Á¡„ ÔÔ¤ì($%E½±½È•¹€ô½±½Èì($%•¹¹Í•Ñ±Á¡„ ÄÈ¤ì($%É…‘¥•¹Ð¹Í•Ñ½±½ÉÐ À°ÍÑ…ÉÐ¤ì($%É…‘¥•¹Ð¹Í•Ñ½±½ÉÐ Ä°•¹¤ì($%Á…¥¹Ñ•È¹™¥±±I•Ð¡EI•Ð¡±¥¹•I•Ð¹±•™Ð ¤€¬€Ü°€À°€Ì°¡•¥¡Ð ¤¤°É…‘¥•¹Ð¤ì($%‰É•…¬ì(%ô(%ô)ô((¼¼I•¹‘•ÉÌÑ¡”‰…‘”Á¥Ñ½É…´…ÐÑ¡”±…‰•°Ì‘•Ù¥”Á¥á•°É…Ñ¥¼¥¸Ñ¡”(¼¼Í­¥¸µÉ•Í½±Ù•¥¹¬¸Q¡”Ñ½½±‰…ÈÌU%!•±Á•ÈèéÑ¥¹Ñ•‘%½¸(¼¼‰…­•Ì„AH´ÄÁ¥áµ…À™½ÈE%½¸½¹ÍÕµ•ÉÌì„E1…‰•°¹••‘Ì…¸•áÁ±¥¥Ð(¼¼AHµ…Ý…É”Á¥áµ…À½ÈÑ¡”±åÁ ‰±ÕÉÌ½¸Í…±•‘¥ÍÁ±…åÌ¸=Ù•ÉÍ¡½½Ñ¥¹œÑ¡”(¼¼™¥±°Á…ÍÐÑ¡”‘•Ù¥”µ¥¹‘•Á•¹‘•¹ÐÍ¥é”¥Ì¡…Éµ±•ÍÌ€´Õ¹Ñ½Õ¡•Á¥á•±Ì¡…Ù”(¼¼¹¼½Ù•É…”™½È½µÁ½Í¥Ñ¥½¹5½‘•}M½ÕÉ•%¸Ñ¼Á…¥¹Ð½¸¸)ÍÑ…Ñ¥ŒEA¥áµ…À‰…‘•A¥Ñ½É…´¡½¹ÍÐEMÑÉ¥¹œ˜É•Í½ÕÉ”°½¹ÍÐE½±½È˜¥¹¬°¥¹ÐÍ¥é”°ÅÉ•…°‘•Ù¥•A¥á•±I…Ñ¥¼¤)ì(%½¹ÍÐEMÑÉ¥¹œ…¡•-•ä€ôEMÑÉ¥¹1¥Ñ•É…° ‰¥±Ñ•É…É‘	…‘”è”Äè”Èè”Ìè”Ðˆ¤($$¹…Éœ¡É•Í½ÕÉ”°¥¹¬¹¹…µ”¡E½±½Èèé!•áÉˆ¤¤($$¹…Éœ¡Í¥é”¤($$¹…Éœ¡‘•Ù¥•A¥á•±I…Ñ¥¼°€À°€˜œ°€Ì¤ì(%EA¥áµ…À…¡•ì(%¥˜€¡EA¥áµ…Á…¡”èé™¥¹¡…¡•-•ä°€™…¡•¤¤($%É•ÑÕÉ¸…¡•ì((%EA¥áµ…ÀÁ¥áµ…À€ôE%½¸¡É•Í½ÕÉ”¤¹Á¥áµ…À¡EM¥é”¡Í¥é”°Í¥é”¤°‘•Ù¥•A¥á•±I…Ñ¥¼¤ì(%¥˜€¡Á¥áµ…À¹¥Í9Õ±° ¤¤($%É•ÑÕÉ¸Á¥áµ…Àì(%EA…¥¹Ñ•ÈÁ…¥¹Ñ•È ™Á¥áµ…À¤ì(%Á…¥¹Ñ•È¹Í•Ñ½µÁ½Í¥Ñ¥½¹5½‘”¡EA…¥¹Ñ•Èèé½µÁ½Í¥Ñ¥½¹5½‘•}M½ÕÉ•%¸¤ì(%Á…¥¹Ñ•È¹™¥±±I•Ð¡Á¥áµ…À¹É•Ð ¤°¥¹¬¤ì(%Á…¥¹Ñ•È¹•¹ ¤ì(%EA¥áµ…Á…¡”èé¥¹Í•ÉÐ¡…¡•-•ä°Á¥áµ…À¤ì(%É•ÑÕÉ¸Á¥áµ…Àì)ô()Ù½¥¥±Ñ•É…É‘I½ÜèéÉ•‰Õ¥±‘MÕµµ…Éä ¤)ì($¼¼‘•ÍÉ¥‰•1¥¹” ¤…¹¹½ÐÍ•”¹•¥¡‰½ÕÉ¥¹œ±¥¹•Ì°Í¼Ñ¡”%˜µÍ½Á”½Õ¹Ð¥Ì($¼¼…ÉÉ¥•½Ù•È™É½´Ñ¡”ÁÉ•Ù¥½ÕÌ‘•ÍÉ¥ÁÑ½È€¡…ÍÍ¥¹•‰äÑ¡”½¹ÍÑÉÕÑ½È€¼($¼¼ÕÁ‘…Ñ•I½ÝA½Í¥Ñ¥½¸™É½´…±Õ±…Ñ•M½Á•Ì¤¸(%½¹ÍÐ¥¹Ð±½¥•ÁÑ €ô‘•ÍÉ¥ÁÑ½È¹±½¥•ÁÑ ì(%½¹ÍÐEMÑÉ¥¹1¥ÍÐÍ½Á•¡…¹¹•±Ì€ô‘•ÍÉ¥ÁÑ½È¹Í½Á•¡…¹¹•±Ìì(%‘•ÍÉ¥ÁÑ½È€ô¥±Ñ•É…É‘5½‘•°èé‘•ÍÉ¥‰•1¥¹”¡¥Ñ•´´ùÑ•áÐ°‘•ÍÉ¥ÁÑ½È¹‘•ÁÑ ¤ì(%‘•ÍÉ¥ÁÑ½È¹±½¥•ÁÑ €ô±½¥•ÁÑ ì(%‘•ÍÉ¥ÁÑ½È¹Í½Á•¡…¹¹•±Ì€ôÍ½Á•¡…¹¹•±Ìì(%…ÁÁ±å•ÍÉ¥ÁÑ½È ¤ì)ô()Ù½¥¥±Ñ•É…É‘I½Üèé…ÁÁ±å•ÍÉ¥ÁÑ½È ¤)ì(($¼¼	±…¹¬±¥¹•ÌÉ•¹‘•È…Ì„Ñ¡¥¸ÍÁ…•Èè¹¼¡•…‘•È°¹¼‰½‘ä°¹¼É…ÜÁÉ•Ù¥•Ü¸($¼¼Q¡”…É™É…µ”¥ÑÍ•±˜ÍÑ…åÌÙ¥Í¥‰±”€¡Í¼¥ÑÌ‰…­É½Õ¹™¥±±ÌÑ¡”…À…¹($¼¼Í½Á”µÉ…¥°Á…¥¹Ñ¥¹œÍÑ¥±°Ý½É­Ì™½È¥¹‘•¹Ñ•‰±½­Ì¤‰ÕÐ¥Ì½±±…ÁÍ•($¼¼Ñ¼„Íµ…±°™¥á•¡•¥¡Ð‰äÍ¥é•!¥¹Ð ¤€¼µ¥¹¥µÕµM¥é•!¥¹Ð ¤‰•±½Ü¸(%½¹ÍÐ‰½½°¥ÍMÁ…•È€ô‘•ÍÉ¥ÁÑ½È¹ÑåÁ”€ôôEMÑÉ¥¹1¥Ñ•É…° ‰ÍÁ…•Èˆ¤ì(%¥˜€¡¡•…‘•É]¥‘•Ð€„ô¹Õ±±ÁÑÈ¤($%¡•…‘•É]¥‘•Ð´ùÍ•ÑY¥Í¥‰±” …¥ÍMÁ…•È¤ì(%¥˜€¡‰½‘åMÑ…¬€„ô¹Õ±±ÁÑÈ¤($%‰½‘åMÑ…¬´ùÍ•ÑY¥Í¥‰±” …¥ÍMÁ…•È€˜˜•áÁ…¹‘	ÕÑÑ½¸€„ô¹Õ±±ÁÑÈ€˜˜•áÁ…¹‘	ÕÑÑ½¸´ù¥Í¡•­• ¤¤ì(%¥˜€¡¥ÍMÁ…•È¤(%ì($%¥˜€¡É…ÝAÉ•Ù¥•Ý1…‰•°€„ô¹Õ±±ÁÑÈ¤($$%É…ÝAÉ•Ù¥•Ý1…‰•°´ùÍ•ÑY¥Í¥‰±”¡™…±Í”¤ì($%Íå¹Y¥ÍÕ…±MÑ…Ñ” ¤ì($%ÕÁ‘…Ñ••½µ•ÑÉä ¤ì($%ÕÁ‘…Ñ” ¤ì($%É•ÑÕÉ¸ì(%ô((%½¹ÍÐ	…‘•QÉ•…Ñµ•¹Ð‰…‘•QÉ•…Ñµ•¹Ð€ôM­¥¹5…¹…•Èèé¥¹ÍÑ…¹” ¤´ù‰…‘•QÉ•…Ñµ•¹Ð ($%ÕÉÉ•¹ÑI½Ý%¹™¼ ¤°‘•ÍÉ¥ÁÑ½È¹½±½È°‘•ÍÉ¥ÁÑ½È¹‰…‘”¤ì($¼¼Q¡”‰…‘”¡É½µ”…¹Á¥Ñ½É…´¥¹¬…É”½¹”Í­¥¸µ½Ý¹•‘•¥Í¥½¸¸($¼¼=¹±äÑ½Õ Ñ¡”Ý¥‘•ÐÝ¡•¸Ñ¡”ÍÑå±”…ÑÕ…±±ä¡…¹•èÍ•ÑMÑå±•M¡••Ð($¼¼Õ¹½¹‘¥Ñ¥½¹…±±äÉ•‰Õ¥±‘ÌÑ¡”Ý¥‘•ÐÌÍÑå±”°…¹…ÁÁ±å•ÍÉ¥ÁÑ½ÈÉÕ¹Ì($¼¼……¥¸½¸•Ù•ÉäÍÕµµ…ÉäÉ•‰Õ¥±¸(%¥˜€¡ÑåÁ•	…‘”´ùÍÑå±•M¡••Ð ¤€„ô‰…‘•QÉ•…Ñµ•¹Ð¹ÅÍÌ¤($%ÑåÁ•	…‘”´ùÍ•ÑMÑå±•M¡••Ð¡‰…‘•QÉ•…Ñµ•¹Ð¹ÅÍÌ¤ì($¼¼Q¡”µ½¹½É…´ÍÕÉÙ¥Ù•Ì½¹±ä™½È±¥¹•ÌÑ¡”¥½¸($¼¼…Ñ…±½œ‘½•Ì¹½Ðµ…À€¡É…ÜÑ•áÐ¤°Í¼Õ¹­¹½Ý¸½µµ…¹‘Ì­••ÀÉ•…‘¥¹œ($¼¼¥¹ÍÑ•…½˜½¥¹œ‰±…¹¬¸(%½¹ÍÐEMÑÉ¥¹œ‰…‘•%½¸€ô¥±Ñ•É…É‘5½‘•°èé‰…‘•%½¹I•Í½ÕÉ”¡‘•ÍÉ¥ÁÑ½È¹ÑåÁ”°‘•ÍÉ¥ÁÑ½È¹‰…‘”¤ì(%¥˜€¡‰…‘•%½¸¹¥ÍµÁÑä ¤¤(%ì($%ÑåÁ•	…‘”´ùÍ•ÑA¥áµ…À¡EA¥áµ…À ¤¤ì($%ÑåÁ•	…‘”´ùÍ•ÑQ•áÐ¡‘•ÍÉ¥ÁÑ½È¹‰…‘”¤ì(%ô(%•±Í”(%ì($%ÑåÁ•	…‘”´ùÍ•ÑQ•áÐ¡EMÑÉ¥¹œ ¤¤ì($%ÑåÁ•	…‘”´ùÍ•ÑA¥áµ…À¡‰…‘•A¥Ñ½É…´¡‰…‘•%½¸°‰…‘•QÉ•…Ñµ•¹Ð¹¥¹¬°€ÄØ°‘•Ù¥•A¥á•±I…Ñ¥½ ¤¤¤ì(%ô(%Ñ¥Ñ±•1…‰•°´ùÍ•ÑÕ±±Q•áÐ¡‘•ÍÉ¥ÁÑ½È¹Ñ¥Ñ±”¤ì(%ÍÕµµ…Éå1…‰•°´ùÍ•ÑÕ±±Q•áÐ¡‘•ÍÉ¥ÁÑ½È¹ÍÕµµ…Éä¤ì($¼¼±¥¹”Ñ¡”•¹¥¹”½Õ±¹½ÐÕÍ”Í…åÌÝ¡ä½¸¡½Ù•È¸Q¡”…¹…±åÍ¥ÌÉÕ¸¥ÌÝ¡…Ð($¼¼ÁÉ½‘Õ•ÌÑ¡”É•…Í½¸°Í¼Ñ¡¥Ì¥Ì•µÁÑäÕ¹Ñ¥°½¹”¡…Ì¡…ÁÁ•¹•…¹½•ÌÍÑ…±”($¼¼½¸•‘¥Ð°±¥­”•Ù•Éä½Ñ¡•È±½…™…Ð¸(%½¹ÍÐEMÑÉ¥¹œÁ…ÉÍ•ÉÉ½È€ôÕÉÉ•¹ÑI½Ý%¹™¼ ¤¹Á…ÉÍ•ÉÉ½Èì(%ÍÕµµ…Éå1…‰•°´ùÍ•ÑQ½½±Q¥À¡Á…ÉÍ•ÉÉ½È¹¥ÍµÁÑä ¤€ü‘•ÍÉ¥ÁÑ½È¹ÍÕµµ…Éä($$èÑÈ ‰Q¡¥Ì±¥¹”Ý…Ì¹½Ð…ÁÁ±¥•è€”Äˆ¤¹…Éœ¡Á…ÉÍ•ÉÉ½È¤¤ì($¼¼Q¡”Ñ•áÐÍÑ…åÌÕÉÉ•¹Ð•Ù•¸Ý¡¥±”Ñ¡”±…‰•°¥Ì¡¥‘‘•¸èÍ­¥¹Ìµ…äÉ•…($¼¼¥Ð…ÌÑ¡”±¥Ù”É…ÜµÍÁ•ŒÍ½ÕÉ”¥¹ÍÑ•…½˜Í¡½Ý¥¹œÑ¡”±…‰•°¥ÑÍ•±˜($¼¼€¡5…ÑÉ¥áI½Ý…ÁÑ¥½¸Ì…ÁÑ¥½¸ÍÑÉ¥À‘½•Ì¤¸(%É…ÝAÉ•Ù¥•Ý1…‰•°´ùÍ•ÑQ•áÐ¡ÑÈ ‰I…Üˆ¤€¬EMÑÉ¥¹1¥Ñ•É…° ˆ€€ˆ¤€¬¥Ñ•´´ùÑ•áÐ¤ì(%½¹ÍÐM­¥¹Q½­•¹Ì˜Ñ½­•¹Ì€ôM­¥¹5…¹…•Èèé¥¹ÍÑ…¹” ¤´ùÑ½­•¹Ì ¤ì(%É…ÝAÉ•Ù¥•Ý1…‰•°´ùÍ•ÑY¥Í¥‰±”¡Ñ½­•¹Ì¹Í¡½ÝI…ÝAÉ•Ù¥•Ü¤ì($¼¼M­¥¹ÌÝ¥Ñ¡½ÕÐ„É…ÜÁÉ•Ù¥•Ü¹•Ù•ÈÍ¡½ÜÑ¡”±…‰•°°…¹É½ÝÌ…É”É•‰Õ¥±Ð($¼¼½¸•Ù•ÉäÍ­¥¸ÍÝ¥Ñ €´Í­¥ÀÑ¡”Á•ÈµÝ¥‘•ÐÍÑå±•Í¡••Ð™½ÈÑ¡•´¸(%¥˜€¡Ñ½­•¹Ì¹Í¡½ÝI…ÝAÉ•Ù¥•Ü¤(%ì($%½¹ÍÐEMÑÉ¥¹œÁÉ•Ù¥•ÝMÑå±”€ôEMÑÉ¥¹1¥Ñ•É…° ‰E1…‰•°¥±Ñ•É…É‘I…ÝAÉ•Ù¥•Üì‰…­É½Õ¹è€”Äì½±½Èè€”Èì‰½É‘•ÈµÑ½Àè€ÅÁàÍ½±¥€”ÌìÁ…‘‘¥¹œè€ÑÁà€ÄÉÁàì™½¹Ðµ™…µ¥±äèpˆ”Ñpˆì™½¹ÐµÍ¥é”è€åÁÐìôˆ¤($$$¹…Éœ¡Ñ½­•¹Ì¹ÍÕÉ™…•MÕ¹­•¸°Ñ½­•¹Ì¹µÕÑ•‘Q•áÐ°Ñ½­•¹Ì¹‰½É‘•È°Ñ½­•¹Ì¹µ½¹½½¹Ñ…µ¥±ä¤ì($%¥˜€¡É…ÝAÉ•Ù¥•Ý1…‰•°´ùÍÑå±•M¡••Ð ¤€„ôÁÉ•Ù¥•ÝMÑå±”¤($$%É…ÝAÉ•Ù¥•Ý1…‰•°´ùÍ•ÑMÑå±•M¡••Ð¡ÁÉ•Ù¥•ÝMÑå±”¤ì(%ô(%•¹…‰±•‘	ÕÑÑ½¸´ù‰±½­M¥¹…±Ì¡ÑÉÕ”¤ì(%•¹…‰±•‘	ÕÑÑ½¸´ùÍ•Ñ¡•­•¡‘•ÍÉ¥ÁÑ½È¹•¹…‰±•¤ì(%•¹…‰±•‘	ÕÑÑ½¸´ùÍ•Ñ%½¸¡E%½¸¡‘•ÍÉ¥ÁÑ½È¹•¹…‰±•€üEMÑÉ¥¹1¥Ñ•É…° ˆè½¥½¹Ì½Á½Ý•É}½¸¹ÍÙœˆ¤€èEMÑÉ¥¹1¥Ñ•É…° ˆè½¥½¹Ì½Á½Ý•É}½™˜¹ÍÙœˆ¤¤¤ì(%•¹…‰±•‘	ÕÑÑ½¸´ù‰±½­M¥¹…±Ì¡™…±Í”¤ì(%•¹…‰±•‘	ÕÑÑ½¸´ùÍ•ÑY¥Í¥‰±”¡‘•ÍÉ¥ÁÑ½È¹…¹Q½±•¹…‰±•¤ì($¼¼¥Í…‰±”½¹±äÑ¡”‰½‘ä•‘¥Ñ½ÈÝ¡•¸Ñ¡”±¥¹”¥Ì½µµ•¹Ñ•½ÕÐ¸Q¡¥Ì­••ÁÌ($¼¼Ñ¡”…É™É…µ”€¡…¹¥ÑÌ¡•…‘•È‰ÕÑÑ½¹Ì€´¥¹±Õ‘¥¹œÑ¡”•¹…‰±”Ñ½±”…¹($¼¼Ñ¡”É…Üµ•‘¥Ð…™™½É‘…¹”¤¥¹Ñ•É…Ñ¥Ù”Í¼Ñ¡”ÕÍ•È…¸™±¥ÀÑ¡”±¥¹”‰…¬½¸¸($¼¼ÁÕÉ”½µµ•¹ÐÉ½Ü¥Ì€‰‘¥Í…‰±•ˆ‰ä‘•™¥¹¥Ñ¥½¸€¡Ñ¡”±¥¹”ÍÑ…ÉÑÌÝ¥Ñ €œŒœ¤°($¼¼‰ÕÐ¥ÑÌ‰½‘ä%LÑ¡”¹½Ñ”•‘¥Ñ½È€´­••À¥Ð•‘¥Ñ…‰±”¸(%¥˜€¡Õ¤€„ô¹Õ±±ÁÑÈ¤($%Õ¤´ùÍ•Ñ¹…‰±•¡‘•ÍÉ¥ÁÑ½È¹•¹…‰±•ñð‘•ÍÉ¥ÁÑ½È¹ÑåÁ”€ôôEMÑÉ¥¹1¥Ñ•É…° ‰½µµ•¹Ðˆ¤¤ì($¼¼É½ÜÌ½Ý¸¡…¹¹•°±¥ÍÐ€¡Ñ¡”¡…¹¹•°…ÉÌÍ•±•Ñ¥½¸°½ÁäÌ($¼¼‘•ÍÑ¥¹…Ñ¥½¹Ì¤Ý¥¹Ì¸=Ñ¡•ÈÉ½ÝÌ¥¹Í¥‘”„¡…¹¹•°èÍ•±•Ñ¥½¸¥¹¡•É¥ÐÑ¡”($¼¼Í•±•Ñ¥½¸Ì‰…‘•Ì°Í¼Ñ¡”É½ÕÀÌÉ•… ¥ÌÉ•…‘…‰±”½¸•Ù•Éäµ•µ‰•È($¼¼É½Ü¥¹ÍÑ•…½˜½¹±ä½¸¥ÑÌ¡•…€´‰ÕÐ½¹±ä™½ÈÉ½ÜÑåÁ•ÌÑ¡”•¹¥¹”($¼¼…ÑÕ…±±ä¹…ÉÉ½ÝÌÑ¼Ñ¡”Í•±•Ñ¥½¸ì½¹ÑÉ½°É½ÝÌ°¹½Ñ•Ì…¹É…ÜÑ•áÐ($¼¼Ý½Õ±±…¥´…¸¥¹™±Õ•¹”Ñ¡•ä‘¼¹½Ð¡…Ù”¸(%EMÑÉ¥¹1¥ÍÐ‰…‘•¡…¹¹•±Ì€ô‘•ÍÉ¥ÁÑ½È¹¡…¹¹•±	…‘•Ìì(%¥˜€¡‰…‘•¡…¹¹•±Ì¹¥ÍµÁÑä ¤€˜˜¡…¹¹•±M•±•Ñ¥½¹…Ñ•ÍQåÁ”¡‘•ÍÉ¥ÁÑ½È¹ÑåÁ”¤¤($%‰…‘•¡…¹¹•±Ì€ô‘•ÍÉ¥ÁÑ½È¹Í½Á•¡…¹¹•±Ìì(%‰Õ¥±‘¡…¹¹•±	…‘•Ì¡‰…‘•¡…¹¹•±Ì¤ì(%Íå¹Y¥ÍÕ…±MÑ…Ñ” ¤ì(%ÕÁ‘…Ñ” ¤ì)ô()Ù½¥¥±Ñ•É…É‘I½Üèé‰Õ¥±‘¡…¹¹•±	…‘•Ì¡½¹ÍÐEMÑÉ¥¹1¥ÍÐ˜¡…¹¹•±Ì¤)ì(%¥˜€¡¡…¹¹•±Ì€ôôÉ•¹‘•É•‘¡…¹¹•±	…‘•Ì¤($%É•ÑÕÉ¸ì(%É•¹‘•É•‘¡…¹¹•±	…‘•Ì€ô¡…¹¹•±Ìì((%Ý¡¥±”€¡E1…å½ÕÑ%Ñ•´¨¡¥±€ô¡…¹¹•±	…‘•1…å½ÕÐ´ùÑ…­•Ð À¤¤(%ì($%‘•±•Ñ”¡¥±´ùÝ¥‘•Ð ¤ì($%‘•±•Ñ”¡¥±ì(%ô((%™½È€¡½¹ÍÐEMÑÉ¥¹œ˜¡…¹¹•°€è¡…¹¹•±Ì¹µ¥ À°€à¤¤($%¡…¹¹•±	…‘•1…å½ÕÐ´ù…‘‘]¥‘•Ð¡¹•Ü¡	…‘”¡¡…¹¹•°°¡…¹¹•±	…‘•½¹Ñ…¥¹•È¤¤ì(%¡…¹¹•±	…‘•½¹Ñ…¥¹•È´ùÍ•ÑY¥Í¥‰±” …¡…¹¹•±Ì¹¥ÍµÁÑä ¤¤ì)ô()Ù½¥¥±Ñ•É…É‘I½ÜèéÕÁ‘…Ñ•5½‘•° ¤)ì(%%¥±Ñ•ÉU$¨Í•¹‘•ÉÕ¤€ôÅ½‰©•Ñ}…ÍÐñ%¥±Ñ•ÉU$¨ø¡E=‰©•ÐèéÍ•¹‘•È ¤¤ì(%¥˜€¡Í•¹‘•ÉÕ¤€ôô¹Õ±±ÁÑÈ¤($%É•ÑÕÉ¸ì((%EMÑÉ¥¹œ½µµ…¹ì(%EMÑÉ¥¹œÁ…É…µ•Ñ•ÉÌì(%Í•¹‘•ÉÕ¤´ùÍÑ½É”¡½µµ…¹°Á…É…µ•Ñ•ÉÌ¤ì($¼¼€ˆŒˆ¥ÌÑ¡”½µµ•¹Ð…ÉÌÍ•¹Ñ¥¹•°è„ÁÕÉ”½µµ•¹Ð±¥¹”¡…Ì¹¼½±½¸°Í¼($¼¼¥Ð¥ÌÉ•…ÍÍ•µ‰±•…Ì€ˆŒ€ñÑ•áÐøˆ€¡„‰…É”€ˆŒˆÝ¡•¸Ñ¡”¹½Ñ”¥Ì•µÁÑä¤¸(%¥˜€¡½µµ…¹€ôôEMÑÉ¥¹1¥Ñ•É…° ˆŒˆ¤¤($%¥Ñ•´´ùÑ•áÐ€ôÁ…É…µ•Ñ•ÉÌ¹¥ÍµÁÑä ¤€üEMÑÉ¥¹1¥Ñ•É…° ˆŒˆ¤€èEMÑÉ¥¹1¥Ñ•É…° ˆŒ€ˆ¤€¬Á…É…µ•Ñ•ÉÌì(%•±Í”($%¥Ñ•´´ùÑ•áÐ€ô½µµ…¹€¬EMÑÉ¥¹1¥Ñ•É…° ˆè€ˆ¤€¬Á…É…µ•Ñ•ÉÌì(%É•‰Õ¥±‘MÕµµ…Éä ¤ì(%Ñ…‰±”´ùÕÁ‘…Ñ•5½‘•° ¤ì)ô()Ù½¥¥±Ñ•É…É‘I½ÜèéÉ½ÕÑ¥¹‘¥Ñ• ¤)ì(%¥˜€¡É½ÕÑ¥¹Y¥•Ü€ôô¹Õ±±ÁÑÈ¤($%É•ÑÕÉ¸ì((%½¹ÍÐEMÑÉ¥¹œÁ…É…µ•Ñ•ÉÌ€ô½ÁåI½ÕÑ¥¹‘…ÁÑ•ÈèéÍ•É¥…±¥é”¡É½ÕÑ¥¹Y¥•Ü´ù…ÍÍ¥¹µ•¹ÑÌ ¤¤ì(%¥Ñ•´´ùÑ•áÐ€ôEMÑÉ¥¹1¥Ñ•É…° ‰½Áäè€ˆ¤€¬Á…É…µ•Ñ•ÉÌì(%É•‰Õ¥±‘MÕµµ…Éä ¤ì(%Ñ…‰±”´ùÕÁ‘…Ñ•5½‘•° ¤ì)ô()Ù½¥¥±Ñ•É…É‘I½Üèé…‘‘‰½Ù” ¤)ì(%¥±Ñ•ÉQ•µÁ±…Ñ”™¥±Ñ•ÉQ•µÁ±…Ñ”ì(%¥˜€¡Ñ…‰±”´ù¡½½Í•¥±Ñ•ÉQ•µÁ±…Ñ” ™™¥±Ñ•ÉQ•µÁ±…Ñ”°…‘‘	ÕÑÑ½¸´ùµ…ÁQ½±½‰…°¡EA½¥¹Ð À°…‘‘	ÕÑÑ½¸´ù¡•¥¡Ð ¤¤¤¤¤(%ì($$¼¼…É¡•…‘•ÈÌ€¬‰•±½¹ÌÑ¼Ñ¡…Ð…ÉÌ±•…‘¥¹œ•‘”è…‘‘1¥¹”Ñ…­•Ì($$¼¼…¸¥¹Í•ÉÐµ‰•™½É”…¹¡½È°Í¼Ñ¡¥ÌÉ½Ü¥ÑÍ•±˜¥ÌÑ¡”‘•Í¥É•…¹¡½È¸($%Ñ…‰±”´ù…‘‘1¥¹”¡™¥±Ñ•ÉQ•µÁ±…Ñ”¹•Ñ1¥¹” ¤°¥Ñ•´¤ì($%¥±Ñ•ÉQ…‰±”¨Ñ…É•ÑQ…‰±”€ôÑ…‰±”ì($%EQ¥µ•ÈèéÍ¥¹±•M¡½Ð À°Ñ…É•ÑQ…‰±”°mÑ…É•ÑQ…‰±•t ¤ì($$%Ñ…É•ÑQ…‰±”´ùÕÁ‘…Ñ•Õ¥Ì ¤ì($%ô¤ì(%ô)ô()Ù½¥¥±Ñ•É…É‘I½ÜèéÉ•µ½Ù•Q¡¥Ì ¤)ì(%¥±Ñ•ÉQ…‰±”¨Ñ…É•ÑQ…‰±”€ôÑ…‰±”ì(%¥±Ñ•ÉQ…‰±”èé%Ñ•´¨Ñ…É•Ñ%Ñ•´€ô¥Ñ•´ì(%EQ¥µ•ÈèéÍ¥¹±•M¡½Ð À°Ñ…É•ÑQ…‰±”°mÑ…É•ÑQ…‰±”°Ñ…É•Ñ%Ñ•µt ¤ì($%Ñ…É•ÑQ…‰±”´ùÉ•µ½Ù•%Ñ•´¡Ñ…É•Ñ%Ñ•´¤ì($%Ñ…É•ÑQ…‰±”´ùÕÁ‘…Ñ•Õ¥Ì ¤ì(%ô¤ì)ô()Ù½¥¥±Ñ•É…É‘I½Üèé•‘¥ÑQ•áÑQ½±•¡‰½½°¡•­•¤)ì(%Í•Ñ‘¥Ñ¥¹œ¡¡•­•¤ì)ô()Ù½¥¥±Ñ•É…É‘I½ÜèéÍ•Ñ‘¥Ñ¥¹œ¡‰½½°•‘¥Ñ¥¹œ¤)ì(%¥˜€¡•‘¥Ñ¥¹œ¤(%ì($%±¥¹•‘¥Ð´ùÍ•ÑQ•áÐ¡¥Ñ•´´ùÑ•áÐ¤ì($%‰½‘åMÑ…¬´ùÍ•ÑÕÉÉ•¹Ñ]¥‘•Ð¡±¥¹•‘¥Ð¤ì($%‰½‘åMÑ…¬´ùÍ•ÑY¥Í¥‰±”¡ÑÉÕ”¤ì($%•áÁ…¹‘	ÕÑÑ½¸´ùÍ•Ñ¡•­•¡ÑÉÕ”¤ì($%±¥¹•‘¥Ð´ùÍ•Ñ½ÕÌ ¤ì($%±¥¹•‘¥Ð´ùÍ•±•Ñ±° ¤ì(%ô(%•±Í”¥˜€¡Õ¤€„ô¹Õ±±ÁÑÈ¤(%ì($%‰½‘åMÑ…¬´ùÍ•ÑÕÉÉ•¹Ñ%¹‘•à Ä¤ì(%ô(%•±Í”¥˜€¡‰½‘åMÑ…¬´ù½Õ¹Ð ¤€ø€Ä¤(%ì($%‰½‘åMÑ…¬´ùÍ•ÑÕÉÉ•¹Ñ%¹‘•à Ä¤ì(%ô)ô()Ù½¥¥±Ñ•É…É‘I½Üèé±¥¹•‘¥Ñ¥¹¥¹¥Í¡• ¤)ì(%¥˜€¡‰½‘åMÑ…¬´ùÕÉÉ•¹Ñ]¥‘•Ð ¤€ôô±¥¹•‘¥Ð€˜˜€…•‘¥Ñ¥¹½¹”¤(%ì($%•‘¥Ñ¥¹½¹”€ôÑÉÕ”ì($%¥˜€¡±¥¹•‘¥Ð´ùÑ•áÐ ¤€„ô¥Ñ•´´ùÑ•áÐ¤($%ì($$%¥Ñ•´´ùÑ•áÐ€ô±¥¹•‘¥Ð´ùÑ•áÐ ¤ì($$%Ñ…‰±”´ùÕÁ‘…Ñ•5½‘•° ¤ì($$%•‘¥Ñ¥¹½¹”€ô™…±Í”ì($$%¥±Ñ•ÉQ…‰±”¨Ñ…É•ÑQ…‰±”€ôÑ…‰±”ì($$%EQ¥µ•ÈèéÍ¥¹±•M¡½Ð À°Ñ…É•ÑQ…‰±”°mÑ…É•ÑQ…‰±•t ¤ì($$$%Ñ…É•ÑQ…‰±”´ùÕÁ‘…Ñ•Õ¥Ì ¤ì($$%ô¤ì($$%É•ÑÕÉ¸ì($%ô($%•‘¥Ñ	ÕÑÑ½¸´ùÍ•Ñ¡•­•¡™…±Í”¤ì($%•‘¥Ñ¥¹½¹”€ô™…±Í”ì(%ô)ô()EMÑÉ¥¹œ¥±Ñ•É…É‘I½ÜèéÕ¹½µµ•¹Ñ•‘1¥¹” ¤½¹ÍÐ)ì(%EI•Õ±…ÉáÁÉ•ÍÍ¥½¸½µµ•¹ÑAÉ•™¥à¡EMÑÉ¥¹1¥Ñ•É…° ‰x¡qqÌ¨¤qqÌüˆ¤¤ì(%EI•Õ±…ÉáÁÉ•ÍÍ¥½¹5…Ñ µ…Ñ €ô½µµ•¹ÑAÉ•™¥à¹µ…Ñ ¡¥Ñ•´´ùÑ•áÐ¤ì(%¥˜€¡µ…Ñ ¹¡…Í5…Ñ  ¤¤($%É•ÑÕÉ¸µ…Ñ ¹…ÁÑÕÉ• Ä¤€¬¥Ñ•´´ùÑ•áÐ¹µ¥¡µ…Ñ ¹…ÁÑÕÉ•‘¹ À¤¤ì(%É•ÑÕÉ¸¥Ñ•´´ùÑ•áÐì)ô()Ù½¥¥±Ñ•É…É‘I½Üèé•¹…‰±•‘Q½±•¡‰½½°¡•­•¤)ì(%¥˜€ …‘•ÍÉ¥ÁÑ½È¹…¹Q½±•¹…‰±•¤($%É•ÑÕÉ¸ì((%¥˜€¡•¹…‰±•‘	ÕÑÑ½¸€„ô¹Õ±±ÁÑÈ¤($%•¹…‰±•‘	ÕÑÑ½¸´ùÍ•Ñ%½¸¡E%½¸¡¡•­•€üEMÑÉ¥¹1¥Ñ•É…° ˆè½¥½¹Ì½Á½Ý•É}½¸¹ÍÙœˆ¤€èEMÑÉ¥¹1¥Ñ•É…° ˆè½¥½¹Ì½Á½Ý•É}½™˜¹ÍÙœˆ¤¤¤ì((%EMÑÉ¥¹œÑÉ¥µµ•€ô¥Ñ•´´ùÑ•áÐ¹ÑÉ¥µµ• ¤ì(%¥˜€¡¡•­•€˜˜ÑÉ¥µµ•¹ÍÑ…ÉÑÍ]¥Ñ  œŒœ¤¤($%¥Ñ•´´ùÑ•áÐ€ôÕ¹½µµ•¹Ñ•‘1¥¹” ¤ì(%•±Í”¥˜€ …¡•­•€˜˜€…ÑÉ¥µµ•¹ÍÑ…ÉÑÍ]¥Ñ  œŒœ¤¤($%¥Ñ•´´ùÑ•áÐ€ôEMÑÉ¥¹1¥Ñ•É…° ˆŒ€ˆ¤€¬¥Ñ•´´ùÑ•áÐì((%Ñ…‰±”´ùÕÁ‘…Ñ•5½‘•° ¤ì(%¥±Ñ•ÉQ…‰±”¨Ñ…É•ÑQ…‰±”€ôÑ…‰±”ì(%¥±Ñ•ÉQ…‰±”èé%Ñ•´¨Ñ…É•Ñ%Ñ•´€ô¥Ñ•´ì($¼¼%¸µÁ±…”É•™É•Í è½¹±äÑ¡”Ñ½±•É½ÜÌU$¹••‘ÌÑ¼¡…¹”¸™Õ±°($¼¼ÕÁ‘…Ñ•Õ¥Ì ¤½¸„€ÔÀÀ¬É½Ü½¹™¥œ¥ÌÑ¡”‘½µ¥¹…¹ÐÍ½ÕÉ”½˜Ñ½±”($¼¼±…Ñ•¹ä¸(%EQ¥µ•ÈèéÍ¥¹±•M¡½Ð À°Ñ…É•ÑQ…‰±”°mÑ…É•ÑQ…‰±”°Ñ…É•Ñ%Ñ•µt ¤ì($%Ñ…É•ÑQ…‰±”´ùÕÁ‘…Ñ•M¥¹±•I½ÝÕ¤¡Ñ…É•Ñ%Ñ•´¤ì(%ô¤ì)ô()Ù½¥¥±Ñ•É…É‘I½Üèé•áÁ…¹‘•‘Q½±•¡‰½½°¡•­•¤)ì(%•áÁ…¹‘	ÕÑÑ½¸´ùÍ•ÑQ•áÐ¡¡•­•€üEMÑÉ¥¹1¥Ñ•É…° ‰Øˆ¤€èEMÑÉ¥¹1¥Ñ•É…° ˆøˆ¤¤ì(%‰½‘åMÑ…¬´ùÍ•ÑY¥Í¥‰±”¡¡•­•¤ì)ô()Ù½¥¥±Ñ•É…É‘I½ÜèéÍå¹Y¥ÍÕ…±MÑ…Ñ” ¤)ì(%¥˜€¡…É‘É…µ”€ôô¹Õ±±ÁÑÈ¤($%É•ÑÕÉ¸ì((%Ù¥ÍÕ…±%¹™¼€ôÕÉÉ•¹ÑI½Ý%¹™¼ ¤ì(%…É‘É…µ”´ù…ÁÁ±åI½Ý%¹™¼¡Ù¥ÍÕ…±%¹™¼°¡•…‘•É]¥‘•Ð¤ì)ô