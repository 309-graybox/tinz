#include "Canvas.h"

#include "../../runtime_editor/RuntimeEditor.h"
#include "../editor/UIDesigner.h"

#include <UnigineConsole.h>
#include <UnigineDisplays.h>
#include <UnigineFfp.h>
#include <UnigineGame.h>
#include <UnigineViewport.h>

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(Canvas);

Canvas *Canvas::instance = nullptr;
Unigine::Vector<Canvas *> Canvas::canvases;
Unigine::Curve2dPtr Canvas::font_height_to_size;
Unigine::Vector<int> Canvas::font_size_to_height;

void Canvas::init_canvas()
{
	if (gui)
		return;

	gui = Gui::create();
	gui->setExposeSpeed(0);	   // instant show
	gui->setPosition(ivec2(0, 0));
	gui->setSize(ivec2(ftoi(reference_width), ftoi(reference_height)));

	canvases.appendUnique(this);
	if (is_render_mode_screen())
		instance = this;

	EngineWindowViewportPtr window = WindowManager::getMainWindow();
	if (is_render_mode_screen() && window)
	{
		gui->setPosition(window->getClientPosition());
		gui->setSize(window->getClientRenderSize());

		window->getViewport()->getEventEndScreen().connect(window_connections, [this]() {
			if (Renderer::isReflection() || !isEnabled())
				return;
			if (!Renderer::getTextureColor() || !Renderer::getTextureCurrentDepth())
				return;

			before_render_gui_event.run();

			RenderState::saveState();

			RenderTargetPtr rtt;
			const auto rt = Render::getTemporaryRenderTarget();
			rt->bindColorTexture(0, Renderer::getTextureColor());
			rt->bindDepthTexture(Renderer::getTextureCurrentDepth());
			rt->enable();
			{
				gui->enable();
				gui->update();
				gui->preRender();
				gui->render();
				gui->disable();
			}
			rt->disable();
			Render::releaseTemporaryRenderTarget(rt);

			RenderState::restoreState();
		});

		screen_size = window->getClientSize() - ivec2{0, window->getTitleBarHeight()};
		screen_size = ivec2(vec2(screen_size) * window->getDpiScale());
		system_dpi_scale =
			itof(Displays::getDPI(Displays::getCurrent())) / Displays::getDefaultSystemDPI();
	}
	else	// render to texture
	{
		ivec2 unit_size = gui->getSize();
		screen_size = ivec2(gui->toRenderSize(unit_size.x), gui->toRenderSize(unit_size.y));
		system_dpi_scale = 1.0f;
	}

	calculate_font_heights();
	refresh_canvas_size();

	// set default element's variables
	canvas = this;
	need_to_rearrange = true;

	init_material();

	saved_mouse_handle = Input::getMouseHandle();

	element_initialized = true;
}

void Canvas::init()
{
	// don't need to call this:
	// Element::init();
	// because we set default parameters in the init_canvas()
}

void Canvas::shutdown()
{
#ifndef EDITOR_PLUGIN
	Input::setMouseHandle(saved_mouse_handle);
#endif

	canvases.removeOne(this);

	shutdown_material();
	gui.deleteLater();
	window_connections.disconnectAll();

	Element::shutdown();
}

void Canvas::initializeElement()
{
	if (!element_initialized)
		init_canvas();
}

void Canvas::needToRearrange()
{
	need_to_rearrange = true;
}

void Canvas::updateManual(float ifps)
{
	updateManual(vec2(-FLT_MAX, -FLT_MAX), 0, 0, ivec2_zero, false, ifps);
}

void Canvas::updateManual(const vec2 &mouse_pos, int mouse_button, int mouse_scroll,
	const ivec2 &navigation_dir, bool navigation_enter, float ifps)
{
	// in case if someone added canvas as a child to other element
	if (canvas != this)
	{
		canvas = this;
		set_canvas_hierarchy(canvas);
	}

	// check resize application window
	if (is_render_mode_screen())
	{
		Unigine::Math::ivec2 window_size;
		EngineWindowPtr window = WindowManager::getMainWindow();
		if (window)
		{
			window_size = window->getClientRenderSize();
			gui->setPosition(window->getClientPosition());
			gui->setSize(window_size);
		}
		else
		{
			ivec2 unit_size = gui->getSize();
			window_size = ivec2(gui->toRenderSize(unit_size.x), gui->toRenderSize(unit_size.y));
		}

		screen_size = window_size;
	}
	else
		screen_size = gui->getSize();

	// arrange all widgets (if the window has been resized)
	if (refresh_canvas_size())
	{
		arrange_hierarchy();

		// notify subscribers
		resize_event.run(screen_size.x, screen_size.y);
	}
	else if (need_to_rearrange)
		arrange_hierarchy();
	need_to_rearrange = false;

	// update input (mouse)
	update_cursor(mouse_pos, mouse_button, mouse_scroll);

	// update all widgets
	nav_dir = navigation_dir;
	nav_enter = navigation_enter;
	update_hierarchy(ifps);

	// update input (navigation)
	update_navigation(navigation_dir, navigation_enter);

	// render to texture (if needed)
	render_to_material();
}

void Canvas::post_update()
{
	if (is_update_mode_manual())
		return;

	if (is_render_mode_screen())
	{
		if (Console::isActive())
		{
			// update without input
			updateManual(Engine::get()->getIFps());
		}
		else
		{
			ivec2 mouse_pos_screen;
			auto window = WindowManager::getMainWindow();
			if (window)
			{
				ivec2 window_pos = window->getClientPosition();
				mouse_pos_screen = Input::getMousePosition() - window_pos;
			}
			vec2 mouse_pos = convertScreenToCanvas(mouse_pos_screen);

			int mouse_button = 0;
			if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_LEFT))
				mouse_button = 1;
			if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_MIDDLE))
				mouse_button |= (1 << 1);
			if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT))
				mouse_button |= (1 << 2);

			int mouse_scroll = Input::getMouseWheel();

			if (Input::isMouseCursorHide() && mouse_button == 0 && !isMouseButtonLeftPressed()
				&& !isMouseButtonMiddlePressed() && !isMouseButtonRightPressed())
				mouse_pos = vec2(-FLT_MAX, -FLT_MAX);

			ivec2 nav_dir;
			if (Input::isKeyDown(Input::KEY_UP))
				nav_dir.y = 1;
			else if (Input::isKeyDown(Input::KEY_DOWN))
				nav_dir.y = -1;
			if (Input::isKeyDown(Input::KEY_LEFT))
				nav_dir.x = -1;
			else if (Input::isKeyDown(Input::KEY_RIGHT))
				nav_dir.x = 1;
			bool nav_enter = Input::isKeyDown(Input::KEY_ENTER);

			updateManual(mouse_pos, mouse_button, mouse_scroll, nav_dir, nav_enter,
				Engine::get()->getIFps());
		}

#ifndef EDITOR_PLUGIN
		if (!UIDesigner::get()->isShow() && !RuntimeEditor::isEnabled())
		{
			// don't grab mouse when user clicks on elements
			if (!Input::isMouseCursorHide() && getHoveredElement())
			{
				if (Input::getMouseHandle() != Input::MOUSE_HANDLE_USER)
					saved_mouse_handle = Input::getMouseHandle();
				Input::setMouseHandle(Input::MOUSE_HANDLE_USER);
			}
			else if (Input::isMouseCursorHide())
				Input::setMouseHandle(saved_mouse_handle);
		}
#endif
	}
	else
		updateManual(Engine::get()->getIFps());
}

const TexturePtr &Canvas::render(int width, int height, const vec4 &bg_color)
{
	if (!render_texture)
	{
		render_target = RenderTarget::create();
		render_texture = Texture::create();
		render_texture->create2D(
			width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		gui->setSize(ivec2(width, height));
	}
	else if (width != render_texture->getWidth() || height != render_texture->getHeight())
	{
		render_texture->create2D(
			width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		gui->setSize(ivec2(width, height));
	}

	before_render_gui_event.run();

	RenderState::saveState();

	render_texture->clearBuffer(bg_color);
	render_target->bindColorTexture(0, render_texture);
	render_target->enable();
	{
		gui->enable();
		gui->update();
		gui->preRender();
		gui->render();
		gui->disable();
	}
	render_target->disable();
	render_target->unbindAll();

	RenderState::restoreState();

	return render_texture;
}

const TexturePtr &Canvas::render(int width, int height, const TexturePtr &bg_texture)
{
	if (!render_texture)
	{
		render_target = RenderTarget::create();
		render_texture = Texture::create();
		render_texture->create2D(
			width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		gui->setSize(ivec2(width, height));
	}
	else if (width != render_texture->getWidth() || height != render_texture->getHeight())
	{
		render_texture->create2D(
			width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		gui->setSize(ivec2(width, height));
	}

	before_render_gui_event.run();

	RenderState::saveState();

	render_target->bindColorTexture(0, render_texture);
	render_target->enable();
	{
		Ffp::enable();
		Ffp::setTransform(ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f));
		RenderState::setTexture(RenderState::BIND_FRAGMENT, 0, bg_texture);
		RenderState::flushStates();
		bg_texture->render2D();
		Ffp::disable();

		gui->enable();
		gui->update();
		gui->preRender();
		gui->render();
		gui->disable();
	}
	render_target->disable();
	render_target->unbindAll();

	RenderState::restoreState();

	return render_texture;
}

const TexturePtr &Canvas::render(int width, int height, const Unigine::PlayerPtr &bg_camera,
	const Unigine::Math::vec4 &bg_fade_color)
{
	if (!render_texture)
	{
		render_target = RenderTarget::create();
		render_texture = Texture::create();
		render_texture->create2D(
			width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		gui->setSize(ivec2(width, height));
	}
	else if (width != render_texture->getWidth() || height != render_texture->getHeight())
	{
		render_texture->create2D(
			width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		gui->setSize(ivec2(width, height));
	}

	before_render_gui_event.run();

	static ViewportPtr viewport;
	if (!viewport)
	{
		viewport = Viewport::create();
		viewport->setSkipFlags(Viewport::SKIP_VELOCITY_BUFFER | Viewport::SKIP_VISUALIZER
							   | Viewport::SKIP_FORMAT_RG11B10);
	}

	RenderState::saveState();
	RenderState::setViewport(0, 0, width, height);

	if (!bg_camera)
		render_texture->clearBuffer(vec4(bg_fade_color.xyz, 1));

	render_target->bindColorTexture(0, render_texture);
	render_target->enable();
	{
		if (bg_camera)
		{
			vec4 fade = Render::getFadeColor();
			Render::setFadeColor(bg_fade_color);
			viewport->render(bg_camera->getCamera());
			Render::setFadeColor(fade);
		}

		gui->enable();
		gui->update();
		gui->preRender();
		gui->render();
		gui->disable();
	}
	render_target->disable();
	render_target->unbindAll();

	RenderState::restoreState();

	return render_texture;
}

float Canvas::getCanvasPixelSize() const
{
	// returns "how many canvas pixels 1 real pixel contains"

	if (canvas_mode.get() == 1 /*Reference Resolution*/)
	{
		// NOTE: I have a bug in rectangle window case, need deep research
		// return max(
		//	canvas_reference_width / getScreenWidth(), canvas_reference_height / getScreenHeight());
		return max(canvas_width / getScreenWidth(), canvas_height / getScreenHeight());
	}
	else	// Constant Pixel Mode
		return 1.0f / (system_dpi_scale * pixel_scale);
}

int Canvas::convertCanvasToScreen(float canvas_position) const
{
	return ftoi(canvas_position / getCanvasPixelSize());
}

ivec2 Canvas::convertCanvasToScreen(const vec2 &canvas_position) const
{
	float pixel_size = getCanvasPixelSize();
	return ivec2(ftoi(canvas_position.x / pixel_size), ftoi(canvas_position.y / pixel_size));
}

float Canvas::convertScreenToCanvas(int screen_position) const
{
	return screen_position * getCanvasPixelSize();
}

vec2 Canvas::convertScreenToCanvas(const ivec2 &screen_position) const
{
	return vec2(screen_position) * getCanvasPixelSize();
}

float Canvas::getCanvasFontHeight(int size) const
{
	int height_px = font_size_to_height[size];

	if (canvas_mode.get() == 1 /*Reference Resolution*/)
	{
		return itof(height_px)
			   * min(reference_width / getScreenWidth(), reference_height / getScreenHeight());
	}
	else	// Constant Pixel Size
		return itof(height_px) * system_dpi_scale * pixel_scale;
}

float Canvas::getScreenFontHeight(int size) const
{
	return itof(font_size_to_height[size]);
}

int Canvas::getFontSize(float height) const
{
	float h = height;
	if (canvas_mode.get() == 1 /*Reference Resolution*/)
		h = height * min(getScreenWidth() / reference_width, getScreenHeight() / reference_height);
	else	// Constant Pixel Size
		h = height * system_dpi_scale * pixel_scale;

	return ftoi(font_height_to_size->evaluate(h));
}

int Canvas::getFontSize(int screen_height) const
{
	return ftoi(font_height_to_size->evaluate(itof(screen_height)));
}

Element *Canvas::getElement(int screen_x, int screen_y) const
{
	VectorStack<Element *> all_hovered_elements;
	std::function<void(Element *)> find_elements = [&](const Element *e) {
		for (int i = 0; i < e->getNumChildren(); i++)
		{
			Element *c = e->getChild(i);
			if (!c->isEnabled())
				continue;

			if (c->isInteractable() && c->isHover(screen_x, screen_y))
				all_hovered_elements.append(c);

			find_elements(c);
		}
	};
	find_elements(const_cast<Canvas *>(this));

	if (all_hovered_elements.size() == 0)
		return nullptr;

	// sort
	quickSort(all_hovered_elements.begin(), all_hovered_elements.end(), [](Element *a, Element *b) {
		int order_a = a->getOrder();
		int order_b = b->getOrder();
		if (order_a == order_b)
		{
			// prefer to select more smaller elements
			vec4 ap = vec4(a->getNormalizedBoundMin(), a->getNormalizedBoundMax());
			vec4 bp = vec4(b->getNormalizedBoundMin(), b->getNormalizedBoundMax());
			float a_area = (ap.z - ap.x) * (ap.w - ap.y);
			float b_area = (bp.z - bp.x) * (bp.w - bp.y);
			return a_area < b_area;
		}
		return order_a > order_b;
	});

	return all_hovered_elements[0];
}

Element *Canvas::getElement(const vec2 &canvas_pos) const
{
	ivec2 screen_pos = convertCanvasToScreen(canvas_pos);
	return getElement(screen_pos.x, screen_pos.y);
}

ElementFocusable *Canvas::getFocus() const
{
	if (focused_element && focused_element->isFocus())
		return focused_element.get();
	return nullptr;
}

void Canvas::calculate_font_heights()
{
	if (font_height_to_size)
		return;

	font_height_to_size = Curve2d::create();
	font_height_to_size->clear();

	WidgetLabelPtr l = WidgetLabel::create("X,");
	font_size_to_height.resize(200);
	for (int i = 1; i < font_size_to_height.size(); i++)
	{
		l->setFontSize(i);
		l->arrange();
		font_height_to_size->addKey(vec2(itof(l->getHeight()), itof(i)));
		font_size_to_height[i] = l->getHeight();
	}
	l.deleteLater();
}

bool Canvas::refresh_canvas_size()
{
	float new_canvas_width = canvas_width;
	float new_canvas_height = canvas_height;

	if (canvas_mode.get() == 0 /*Constant Pixel Size*/)
	{
		float delim = system_dpi_scale * pixel_scale;
		new_canvas_width = itof(screen_size.x) / delim;
		new_canvas_height = itof(screen_size.y) / delim;
	}
	else if (canvas_mode.get() == 1 /*Reference Resolution*/)
	{
		if (screen_size.x == screen_size.y)
		{
			new_canvas_width = new_canvas_height = reference_height;
		}
		else
		{
			float aspect_app = itof(screen_size.x) / screen_size.y;
			float aspect_canvas = reference_width / reference_height;

			const float ratio = aspect_app / aspect_canvas;
			const float iratio = aspect_canvas / aspect_app;

			new_canvas_width = reference_width * (ratio >= 1.f ? ratio : 1.f);
			new_canvas_height = reference_height * (ratio >= 1.f ? 1.f : iratio);
		}
	}

	if (new_canvas_width != canvas_width || new_canvas_height != canvas_height)
	{
		canvas_width = new_canvas_width;
		canvas_height = new_canvas_height;
		return true;	// need to arrange all elements
	}

	return false;
}

void Canvas::update_cursor(const vec2 &in_mouse_pos, int in_mouse_button, int in_mouse_scroll)
{
	vec2 prev_mouse_pos = mouse_pos;
	mouse_pos = in_mouse_pos;

	bool left_now = (in_mouse_button & 1) != 0;
	bool middle_now = (in_mouse_button & (1 << 1)) != 0;
	bool right_now = (in_mouse_button & (1 << 2)) != 0;
	bool left_prev = (mouse_button_pressed & 1) != 0;
	bool middle_prev = (mouse_button_pressed & (1 << 1)) != 0;
	bool right_prev = (mouse_button_pressed & (1 << 2)) != 0;

	mouse_button_down = 0;
	if (left_now && !left_prev)
		mouse_button_down = 1;
	if (middle_now && !middle_prev)
		mouse_button_down |= (1 << 1);
	if (right_now && !right_prev)
		mouse_button_down |= (1 << 2);

	mouse_button_up = 0;
	if (left_prev && !left_now)
		mouse_button_up = 1;
	if (middle_prev && !middle_now)
		mouse_button_up |= (1 << 1);
	if (right_prev && !right_now)
		mouse_button_up |= (1 << 2);

	mouse_button_pressed = in_mouse_button;

	mouse_scroll = in_mouse_scroll;

	// refresh mouse dragging state
	if ((mouse_button_pressed & 1) != 0 && !Input::isMouseCursorHide())	   // pressing LMB
	{
		if (mouse_pos != prev_mouse_pos)
			mouse_dragging = true;
	}
	else
		mouse_dragging = false;

	// refresh "hovered_element" variable
	Element *prev_hovered_element = hovered_element.get();
	hovered_element = UIPtr<Element>(getElement(mouse_pos));
	if (hovered_element != prev_hovered_element)
	{
		if (prev_hovered_element)
			prev_hovered_element->mouse_hover_exit_event.run(prev_hovered_element);
		if (hovered_element)
			hovered_element->mouse_hover_enter_event.run(hovered_element.get());
	}
	if (hovered_element)
	{
		if (isMouseButtonLeftDown())
			hovered_element->mouse_click_enter_event.run(hovered_element.get());
		else if (isMouseButtonLeftUp())
			hovered_element->mouse_click_exit_event.run(hovered_element.get());
	}
	else if (isMouseButtonLeftDown())
	{
		// not hover anything, down LMB: remove focus
		for (int i = 0; i < focusable_elements.size(); i++)
			focusable_elements[i]->setFocus(false);
	}
}

void Canvas::update_navigation(ivec2 nav_dir, bool nav_enter)
{
	if (Console::isActive())
		return;

	// click
	if (nav_enter)
	{
		if (focused_element)
		{
			if (focused_element->isFocus())
				focused_element->click();
			else
				focused_element->setFocus(true);
		}
		else if (ElementFocusable *first_element = get_first_focusable_element())
			first_element->setFocus(true);
		return;
	}

	// change selection
	if (focused_element)
	{
		if (!focused_element->isNavigationHorizontallyEnabled())
			nav_dir.x = 0;
		if (!focused_element->isNavigationVerticallyEnabled())
			nav_dir.y = 0;
	}

	if (nav_dir != ivec2_zero)
	{
		if (focused_element)
		{
			if (focused_element->navigation_mode.get() == 0 /*Auto*/)
			{
				ElementFocusable *cur_focus = focused_element.get();
				const vec2 &cur_bound = cur_focus->getNormalizedBoundMin();

				ElementFocusable *next_element = nullptr;
				float next_dist2 = FLT_MAX;

				for (int i = 0; i < focusable_elements.size(); i++)
				{
					auto f = focusable_elements[i];
					if (f != cur_focus && f->isEnabled() && f->isFocusable())
					{
						const vec2 &b = f->getNormalizedBoundMin();
						if (nav_dir.x < 0 && b.x >= cur_bound.x)
							continue;
						if (nav_dir.x > 0 && b.x <= cur_bound.x)
							continue;
						if (nav_dir.y > 0 && b.y >= cur_bound.y)
							continue;
						if (nav_dir.y < 0 && b.y <= cur_bound.y)
							continue;

						float d2 = length2(b - cur_bound);
						if (d2 < next_dist2)
						{
							next_dist2 = d2;
							next_element = f;
						}
					}
				}

				if (next_element)
					next_element->setFocus(true);
			}
			else	// Manual
			{
				if (nav_dir.y > 0)
				{
					if (auto *f = getComponent<ElementFocusable>(
							focused_element->navigation_up.get(), true))
						f->setFocus(true);
				}
				else if (nav_dir.y < 0)
				{
					if (auto *f = getComponent<ElementFocusable>(
							focused_element->navigation_down.get(), true))
						f->setFocus(true);
				}
				else if (nav_dir.x < 0)
				{
					if (auto *f = getComponent<ElementFocusable>(
							focused_element->navigation_left.get(), true))
						f->setFocus(true);
				}
				else if (nav_dir.x > 0)
				{
					if (auto *f = getComponent<ElementFocusable>(
							focused_element->navigation_right.get(), true))
						f->setFocus(true);
				}
			}
		}
		else if (ElementFocusable *first_element = get_first_focusable_element())
			first_element->setFocus(true);
		return;
	}
}

void Canvas::init_material()
{
	if (is_render_mode_screen() || Engine::get()->isEditorLoaded())
		return;
	if (material.get())
	{
		texture_id = material->findTexture(texture_name.get());
		if (texture_id == -1)
		{
			Log::error("Canvas::init(): material \"%s\" doesn't have texture field \"%s\"!\n",
				material->getFilePath().get(), texture_name.get());
		}
	}
}

void Canvas::render_to_material()
{
	if (is_render_mode_screen() || Engine::get()->isEditorLoaded())
		return;

	if (texture_id != -1)
		material->setTexture(texture_id, render(texture_width, texture_height));
}

void Canvas::shutdown_material()
{
	if (material.get() && texture_id != -1)
		material->resetTexture(texture_id);
}

bool Canvas::is_render_mode_screen() const
{
	if (Engine::get()->isEditorLoaded())
		return false;
	return render_mode.get() == 0;
}

bool Canvas::is_update_mode_manual() const
{
	if (Engine::get()->isEditorLoaded())
		return true;
	return update_mode.get() == 1;
}

ElementFocusable *Canvas::get_first_focusable_element() const
{
	ElementFocusable *element = nullptr;
	vec2 min_bound = vec2_one;

	for (int i = 0; i < focusable_elements.size(); i++)
	{
		auto f = focusable_elements[i];
		const vec2 &f_b = f->getNormalizedBoundMin();
		if (f->isEnabled() && f->isFocusable() && f_b.y < min_bound.y)
		{
			element = f;
			min_bound = f_b;
		}
	}

	return element;
}

void Canvas::on_add_element(Element *element)
{
	if (ElementFocusable *f = dynamic_cast<ElementFocusable *>(element))
		focusable_elements.append(f);
}

void Canvas::on_remove_element(Element *element)
{
	if (ElementFocusable *f = dynamic_cast<ElementFocusable *>(element))
		focusable_elements.removeOne(f);
}

void Canvas::on_focus_changed(ElementFocusable *element)
{
	if (!element->isFocus())
		return;

	// unfocus all elements except this one
	for (int i = 0; i < focusable_elements.size(); i++)
	{
		auto f = focusable_elements[i];
		if (f != element)
			f->setFocus(false);
	}

	// store selection
	focused_element = UIPtr<ElementFocusable>(element);
}
