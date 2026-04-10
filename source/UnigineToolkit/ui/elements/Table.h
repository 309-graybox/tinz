#pragma once

#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Table : public Element
{
public:
	COMPONENT(Table, Element);
	PROP_NAME("UI_Table");

	// clang-format off
	PROP_GROUP("Table");
	PROP_PARAM(Int, max_columns, 1, "Max Columns");
	PROP_PARAM(Vec2, min_cell_size, Unigine::Math::vec2(0.f, 0.f), "Min Cell Size");
	PROP_PARAM(Vec2, max_cell_size, Unigine::Math::vec2(Unigine::Math::Consts::INF, Unigine::Math::Consts::INF), "Max Cell Size");
	PROP_PARAM(Float, spacing, 0, "Spacing");
	PROP_PARAM(Toggle, use_screen_spacing, true, "Use Screen Spacing");
	// clang-format on
	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<Table> getPtr() { return UIPtr<Table>(this); }

	void setMaxColumns(int value);
	int getMaxColumns() const { return max_columns; }

	void setMinCellSize(const Unigine::Math::vec2 &min_size);
	Unigine::Math::vec2 getMinCellSize() const { return min_cell_size; }

	void setMaxCellSize(const Unigine::Math::vec2 &max_size);
	Unigine::Math::vec2 getMaxCellSize() const { return max_cell_size; }

	void setSpacing(float value);
	float getSpacing() const { return spacing; }

	void useScreenSpacing(bool value);
	bool isScreenSpacing() const { return use_screen_spacing.get() > 0; }

	// info
	int getNumColumns() const;
	int getNumRows() const;

protected:
	void init() override;

	void arrange_hierarchy() override;
	void arrange() override;

	bool lock_arrange = false;
};
typedef UIPtr<Table> TablePtr;
}	 // namespace UI
