/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2026 Michael Drake <tlsa@netsurf-browser.org>
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

/**
 * Parse gap shorthand
 *
 * gap = <'row-gap'> <'column-gap'>?
 *
 * If the column-gap value is omitted, it is set to the row-gap value.
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
	int32_t end_ctx;
	const css_token *token;
	css_error error;
	css_style *row_style;
	css_style *column_style;
	enum flag_value flag_value;

	/* Firstly, handle inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL)
		return CSS_INVALID;

	flag_value = get_css_flag_value(c, token);

	if (flag_value != FLAG_VALUE__NONE) {
		error = css_stylesheet_style_flag_value(result, flag_value,
				CSS_PROP_ROW_GAP);
		if (error != CSS_OK)
			return error;

		error = css_stylesheet_style_flag_value(result, flag_value,
				CSS_PROP_COLUMN_GAP);
		if (error == CSS_OK)
			parserutils_vector_iterate(vector, ctx);

		return error;
	}

	/* Allocate for styles */
	error = css__stylesheet_style_create(c->sheet, &row_style);
	if (error != CSS_OK)
		return error;

	error = css__stylesheet_style_create(c->sheet, &column_style);
	if (error != CSS_OK) {
		css__stylesheet_style_destroy(row_style);
		return error;
	}

	/* The first value is the row-gap */
	error = css__parse_row_gap(c, vector, ctx, row_style);
	if (error != CSS_OK)
		goto css__parse_gap_cleanup;

	end_ctx = *ctx;
	consumeWhitespace(vector, ctx);

	/* An optional second value is the column-gap */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL &&
			get_css_flag_value(c, token) == FLAG_VALUE__NONE &&
			css__parse_column_gap(c, vector, ctx,
					column_style) == CSS_OK) {
		end_ctx = *ctx;
	} else {
		/* No column-gap value; reparse the row-gap value for it.
		 * Start from a fresh style, if the failed attempt
		 * left anything behind. */
		if (column_style->used != 0) {
			css__stylesheet_style_destroy(column_style);
			error = css__stylesheet_style_create(c->sheet,
					&column_style);
			if (error != CSS_OK) {
				css__stylesheet_style_destroy(row_style);
				*ctx = orig_ctx;
				return error;
			}
		}

		*ctx = orig_ctx;
		error = css__parse_column_gap(c, vector, ctx, column_style);
		if (error != CSS_OK)
			goto css__parse_gap_cleanup;

		/* The row-gap and column-gap grammars must match */
		if (*ctx != end_ctx) {
			error = CSS_INVALID;
			goto css__parse_gap_cleanup;
		}
	}

	*ctx = end_ctx;

	/* Merge styles into the result */
	error = css__stylesheet_merge_style(result, row_style);
	if (error != CSS_OK)
		goto css__parse_gap_cleanup;

	error = css__stylesheet_merge_style(result, column_style);

css__parse_gap_cleanup:
	css__stylesheet_style_destroy(row_style);
	css__stylesheet_style_destroy(column_style);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}
