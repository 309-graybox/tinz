#include "Table.h"

#include "Canvas.h"

#include <UnigineGame.h>

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(Table);

void Table::init()
{
	if (element_initialized)
		return;

	Element::init();

	// apply parameters
	applyPropertyChanges();
}

void Table::applyPropertyChanges()
{
	Element::applyPropertyChanges();

	arrange();
}

void Table::setMaxColumns(int value)
{
	max_columns = value;
	arrange();
}

void Table::setMinCellSize(const Unigine::Math::vec2 &size)
{
	min_cell_size = size;
	arrange();
}

void Table::setMaxCellSize(const Unigine::Math::vec2 &size)
{
	max_cell_size = size;
	arrange();
}

void Table::setSpacing(float value)
{
	spacing = value;
	arrange();
}

void Table::useScreenSpacing(bool value)
{
	use_screen_spacing = value;
	arrange();
}

int Table::getNumColumns() const
{
	int n = getNumChildren();
	return Math::min(n, max_columns.get());
}

int Table::getNumRows() const
{
	int n = getNumChildren();
	return Math::ceilInt(itof(n) / max_columns.get());
}

void Table::arrange_hierarchy()
{
	if (!lock_arrange)
		Element::arrange_hierarchy();
}

void Table::arrange()
{
	if (!canvas || lock_arrange)
		return;

	vec4 table_pos = getWorldPosition();
	float table_width = 0;
	float table_height = 0;

	vec2 stored_table_pos = table_pos.xy;

	bool screen_spacing = isScreenSpacing();
	if (screen_spacing)
	{
		// perfectly align to screen pixels
		table_pos.x = canvas->convertScreenToCanvas(get_screen_x());
		table_pos.y = canvas->convertScreenToCanvas(get_screen_y());
	}

	vec2 min_size = min_cell_size;
	vec2 max_size = max_cell_size;
	float space = screen_spacing ? canvas->convertScreenToCanvas(ftoi(spacing)) : spacing;

	vec2 cursor = table_pos.xy;
	int i = 0;
	const int num_rows = getNumRows();
	const int num_columns = getNumColumns();
	for (int y = 0; y < num_rows; y++)
	{
		float max_height = min_size.y;
		for (int x = 0; x < num_columns; x++)
		{
			if (i >= getNumChildren())
				break;

			if (x != 0)
				cursor.x += space;

			UI::Element *element = getChild(i);
			vec4 p = element->getWorldPosition(false);
			float e_w = element->isFixedWidth() ? element->getWidth() : p.z - p.x;
			float e_h = element->isFixedHeight() ? element->getHeight() : p.w - p.y;
			e_w = clamp(e_w, min_size.x, max_size.x);
			e_h = clamp(e_h, min_size.y, max_size.y);
			p.z = cursor.x + e_w;
			p.w = cursor.y + e_h;
			p.x = cursor.x;
			p.y = cursor.y;
			element->setWorldPosition(p);
			if (element->isFixedWidth())
				element->setWidth(e_w);
			if (element->isFixedHeight())
				element->setHeight(e_h);

			float diff_x = 0;
			float diff_y = 0;
			if (screen_spacing)
			{
				diff_x = e_w - canvas->convertScreenToCanvas(canvas->convertCanvasToScreen(e_w));
				diff_y = e_h - canvas->convertScreenToCanvas(canvas->convertCanvasToScreen(e_h));
			}
			cursor.x += e_w - diff_x;
			max_height = Math::max(max_height, e_h - diff_y);
			i++;
		}
		table_width = Math::max(table_width, cursor.x - table_pos.x);
		table_height += max_height;
		cursor.x = table_pos.x;
		cursor.y += max_height;
		if (y < num_rows - 1)
		{
			table_height += space;
			cursor.y += space;
		}
	}

	// refresh position and bound
	lock_arrange = true;
	setWorldPosition(stored_table_pos.x, stored_table_pos.y, stored_table_pos.x + table_width,
		stored_table_pos.y + table_height);
	update_bound();
	lock_arrange = false;
}
