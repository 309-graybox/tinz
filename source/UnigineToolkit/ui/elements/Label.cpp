#include "Label.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(Label);

void Label::init()
{
	if (element_initialized)
		return;

	lock_arrange();
	Element::init();
	unlock_arrange();

	// create
	label = WidgetLabel::create(get_gui());
	label->setFontRich(1);
	get_parent_widget()->addChild(label, Gui::ALIGN_OVERLAP);

	// apply parameters
	applyPropertyChanges();
	arrange();
	apply_order_to_widgets();
}

void Label::shutdown()
{
	label.deleteLater();

	Element::shutdown();
}

void Label::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setFont(font_file);
	setText(text);
	setFontHeight(font_height);
	setFontColor(font_color);
	setTextAlign(
		static_cast<HORIZONTAL_ALIGN>(h_align.get()), static_cast<VERTICAL_ALIGN>(v_align.get()));
	setFontHSpacing(h_spacing);
	setFontVSpacing(v_spacing);
	setWordWrap(word_wrap.get() == 1);
	// setFontRich(font_rich.get() == 1); - it doesn't needed, stay here as a notice
	setFontOutline(font_outline.get() == 1);

	if (unlock_arrange())
		arrange();
}

void Label::setText(const char *in_text)
{
	text = in_text;
	refresh_text();
	arrange();
}

void Label::setHorizontalAlign(HORIZONTAL_ALIGN align)
{
	h_align = static_cast<int>(align);
	switch (align)
	{
	case HORIZONTAL_ALIGN::LEFT:
		label->setTextAlign(Gui::ALIGN_LEFT);
		break;
	case HORIZONTAL_ALIGN::CENTER:
		label->setTextAlign(Gui::ALIGN_CENTER);
		break;
	case HORIZONTAL_ALIGN::RIGHT:
		label->setTextAlign(Gui::ALIGN_RIGHT);
		break;
	}
	arrange();
}

Label::HORIZONTAL_ALIGN Label::getHorizontalAlign() const
{
	return static_cast<Label::HORIZONTAL_ALIGN>(h_align.get());
}

void Label::setVerticalAlign(VERTICAL_ALIGN align)
{
	v_align = static_cast<int>(align);
	arrange();
}

Label::VERTICAL_ALIGN Label::getVerticalAlign() const
{
	return static_cast<Label::VERTICAL_ALIGN>(v_align.get());
}

void Label::setTextAlign(HORIZONTAL_ALIGN h, VERTICAL_ALIGN v)
{
	h_align = static_cast<int>(h);
	switch (h)
	{
	case HORIZONTAL_ALIGN::LEFT:
		label->setTextAlign(Gui::ALIGN_LEFT);
		break;
	case HORIZONTAL_ALIGN::CENTER:
		label->setTextAlign(Gui::ALIGN_CENTER);
		break;
	case HORIZONTAL_ALIGN::RIGHT:
		label->setTextAlign(Gui::ALIGN_RIGHT);
		break;
	}
	v_align = static_cast<int>(v);
	arrange();
}

void Label::setFont(const char *font)
{
	if (font_file_stored == font)
		return;

	font_file = font;
	font_file_stored = font;
	label->setFont(font);
}

void Label::setFontHeight(float height)
{
	font_height = height;
	arrange();
}

void Label::setWordWrap(bool wrap)
{
	word_wrap = wrap ? 1 : 0;
	label->setFontWrap(wrap);
	arrange();
}

void Label::setFontRich(bool rich)
{
	font_rich = rich ? 1 : 0;
	refresh_text();
}

void Label::setFontVSpacing(float spacing)
{
	v_spacing = spacing;
	arrange();
}

void Label::setFontHSpacing(float spacing)
{
	h_spacing = spacing;
	arrange();
}

void Label::setFontOutline(bool outline)
{
	font_outline = outline ? 1 : 0;
	label->setFontOutline(outline ? 1 : 0);
}

void Label::setFontColor(const vec4 &color)
{
	font_color = color;
	label->setFontColor(font_color);
}

Unigine::Math::ivec2 Label::getTextRenderSize() const
{
	return getTextRenderSize(label->getText());
}

Unigine::Math::ivec2 Label::getTextRenderSize(const char *text) const
{
	return label->getTextRenderSize(text) + ivec2(4, 0);	// 4 - from engine
}

Unigine::Math::vec2 Label::getTextCanvasSize() const
{
	return canvas->convertScreenToCanvas(getTextRenderSize(label->getText()));
}

Unigine::Math::vec2 Label::getTextCanvasSize(const char *text) const
{
	return canvas->convertScreenToCanvas(getTextRenderSize(text));
}

void Label::resizeByText()
{
	setSize(getTextCanvasSize());
}

void Label::resizeByText(float in_min_width, float in_max_width)
{
	vec2 size = getTextCanvasSize();
	size.x = clamp(size.x, in_min_width, in_max_width);
	setSize(size);
}

void Label::on_enable()
{
	Element::on_enable();
	if (label)
		label->setHidden(false);
}

void Label::on_disable()
{
	if (label)
		label->setHidden(true);
	Element::on_disable();
}

void Label::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	int lx = get_screen_x();
	int ly = get_screen_y();
	int lw = get_screen_width();
	int lh = get_screen_height();

	// auto resize
	if (auto_resize.get() == 2 /*Element Size*/)
		lw = canvas->convertCanvasToScreen(max_width);

	// calculate and change font size, word wrap
	label->setFontSize(canvas->getFontSize(font_height));
	label->setFontVOffset(ftoi(label->getFontSize() * 0.1f));	 // fix middle of text lane
	label->setFontHSpacing(canvas->convertCanvasToScreen(h_spacing));
	label->setFontVSpacing(canvas->convertCanvasToScreen(v_spacing));
	label->setWidth(isWordWrap() ? lw : 0);

	auto arrange_multilines_text = [this](int max_steps = 1000) {
		ivec2 prev_text_sz = getTextRenderSize();
		for (int i = 0; i < max_steps; i++)
		{
			label->arrange();
			ivec2 text_sz = getTextRenderSize();
			if (text_sz == prev_text_sz)
				break;
			else
				prev_text_sz = text_sz;
		}
	};
	arrange_multilines_text();

	// auto resize
	if (auto_resize.get() == 1 /*Font Height*/)
	{
		int label_w = label->getWidth();
		int label_h = label->getHeight();
		int element_w = get_screen_width();
		int element_h = get_screen_height();
		float factor = Math::min(itof(element_w) / label_w, itof(element_h) / label_h);
		label->setFontSize(canvas->getFontSize(font_height * factor));
		label->arrange();
	}

	// calculate alignments
	int x = lx;
	int y = ly;

	switch (h_align.get())
	{
	case static_cast<int>(HORIZONTAL_ALIGN::CENTER):
		x = lx + lw / 2 - label->getWidth() / 2;
		break;
	case static_cast<int>(HORIZONTAL_ALIGN::RIGHT):
		x = lx + lw - label->getWidth();
		break;
	default:	// nothing to do
		break;
	}
	switch (v_align.get())
	{
	case static_cast<int>(VERTICAL_ALIGN::MIDDLE):
		y = ly + lh / 2 - label->getHeight() / 2;
		break;
	case static_cast<int>(VERTICAL_ALIGN::BOTTOM):
		y = ly + lh - label->getHeight();
		break;
	default:	// nothing to do
		break;
	}

	// apply position
	label->setPosition(x, y);

	// auto resize
	if (auto_resize.get() == 2 /*Element Size*/)
	{
		vec2 size = getTextCanvasSize();
		size.x = clamp(size.x, min_width, max_width);
		lock_arrange();
		setSize(size);
		unlock_arrange();
	}
}

void Label::apply_order_to_widgets()
{
	if (!label)
		return;
	label->setOrder(getOrder());
	WidgetPtr parent = label->getParent();
	parent->removeChild(label);
	parent->addChild(label);
}

void Label::set_gui(const Unigine::GuiPtr &gui)
{
	if (!label)
		return;
	if (gui != label->getParentGui())
		gui->addChild(label);
}

void Label::refresh_text()
{
	StringStack<> t = Localization::get(text);
	if (font_rich.get() == 0)
	{
		switch (getHorizontalAlign())
		{
		case HORIZONTAL_ALIGN::LEFT:
			t = t.replace("\\n", "\n");
			break;
		case HORIZONTAL_ALIGN::CENTER:
			t = "<p align=center>" + t.replace("\\n", "</p><p align=center>") + "</p>";
			break;
		case HORIZONTAL_ALIGN::RIGHT:
			t = "<p align=right>" + t.replace("\\n", "</p><p align=right>") + "</p>";
			break;
		}
	}
	else
		t = t.replace("\\n", "\n");
	label->setText(t);
}
