/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2026 Cyril Hrubis <metan@ucw.cz>
 */

#include <assert.h>
#include <string.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

/**
 * Parse a single gap component: normal | <length>
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param normal  Pointer to location to receive "is normal" flag
 * \param length  Pointer to location to receive the length
 * \param unit	  Pointer to location to receive the length's unit
 * \return CSS_OK on success, CSS_INVALID if the input is not a gap component
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
static css_error parse_gap_component(css_language *c,
		const parserutils_vector *vector, int32_t *ctx,
		bool *normal, css_fixed *length, uint32_t *unit)
{
	int32_t orig_ctx = *ctx;
	const css_token *token;
	css_error error;
	bool match;

	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL)
		return CSS_INVALID;

	if ((token->type == CSS_TOKEN_IDENT) &&
			(lwc_string_caseless_isequal(token->idata,
			c->strings[NORMAL], &match) == lwc_error_ok &&
			match)) {
		parserutils_vector_iterate(vector, ctx);
		*normal = true;
		return CSS_OK;
	}

	error = css__parse_unit_specifier(c, vector, ctx, UNIT_PX, length, unit);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	if (*length < 0 || (*unit & UNIT_MASK_COLUMN_GAP) == 0) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	*normal = false;

	return CSS_OK;
}

/**
 * Parse gap shorthand
 *
 * gap: <row-gap> <column-gap>?  — one value sets both axes.
 *
 * LibCSS has no row-gap property (column-gap comes from multicol), so only
 * the column component reaches the cascade.  The row component is still
 * parsed and consumed, or the whole declaration would be rejected as
 * invalid and the column gap lost with it.
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error css__parse_gap(css_language *c,
		const parserutils_vector *vector, int32_t *ctx,
		css_style *result)
{
	int32_t orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	enum flag_value flag_value;
	css_fixed length = 0;
	uint32_t unit = 0;
	bool normal = false;

	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL) {
		return CSS_INVALID;
	}

	flag_value = get_css_flag_value(c, token);

	if (flag_value != FLAG_VALUE__NONE) {
		parserutils_vector_iterate(vector, ctx);

		error = css_stylesheet_style_flag_value(result, flag_value,
				CSS_PROP_COLUMN_GAP);
		if (error != CSS_OK)
			*ctx = orig_ctx;

		return error;
	}

	/* Row component — parsed only to consume it. */
	error = parse_gap_component(c, vector, ctx, &normal, &length, &unit);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	consumeWhitespace(vector, ctx);

	/* Column component, if any — it overrides the row one. */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL) {
		css_fixed col_length = 0;
		uint32_t col_unit = 0;
		bool col_normal = false;

		if (parse_gap_component(c, vector, ctx, &col_normal,
				&col_length, &col_unit) == CSS_OK) {
			length = col_length;
			unit = col_unit;
			normal = col_normal;
		}
	}

	if (normal) {
		error = css__stylesheet_style_appendOPV(result,
				CSS_PROP_COLUMN_GAP, 0, COLUMN_GAP_NORMAL);
		if (error != CSS_OK)
			*ctx = orig_ctx;

		return error;
	}

	error = css__stylesheet_style_appendOPV(result, CSS_PROP_COLUMN_GAP,
			0, COLUMN_GAP_SET);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	error = css__stylesheet_style_vappend(result, 2, length, unit);
	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}
