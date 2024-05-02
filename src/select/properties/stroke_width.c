/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_stroke_width(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_length(opv, style, state, set_stroke_width);
}

css_error css__set_stroke_width_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_stroke_width(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error css__initial_stroke_width(css_select_state *state)
{
	return set_stroke_width(state->computed, CSS_STROKE_WIDTH_SET, INTTOFIX(1), CSS_UNIT_PX);
}

css_error css__copy_stroke_width(
		const css_computed_style *from,
		css_computed_style *to)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_stroke_width(from, &length, &unit);

	if (from == to) {
		return CSS_OK;
	}

	return set_stroke_width(to, type, length, unit);
}

css_error css__compose_stroke_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_stroke_width(child, &length, &unit);

	return css__copy_stroke_width(
			type == CSS_STROKE_WIDTH_INHERIT ? parent : child,
			result);
}

