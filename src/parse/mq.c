/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2016 John-Mark Bell <jmb@netsurf-browser.org>
 */

/* https://drafts.csswg.org/mediaqueries/ */

#include "parse/mq.h"

css_error css__mq_parse_media_list(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_mq_query **media)
{
	css_mq_query *ret = NULL;
	const css_token *token;

	/* (IDENT ws (',' ws IDENT ws)* )? */

	UNUSED(c);

	token = parserutils_vector_iterate(vector, ctx);

	while (token != NULL) {
		if (token->type != CSS_TOKEN_IDENT)
			return CSS_INVALID;

#if 0
		if (lwc_string_caseless_isequal(token->idata, c->strings[AURAL], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_AURAL;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[BRAILLE], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_BRAILLE;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[EMBOSSED], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_EMBOSSED;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[HANDHELD], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_HANDHELD;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[PRINT], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_PRINT;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[PROJECTION], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_PROJECTION;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[SCREEN], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_SCREEN;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[SPEECH], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_SPEECH;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[TTY], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_TTY;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[TV], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_TV;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[ALL], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_ALL;
		} else
			return CSS_INVALID;
#endif
		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);
		if (token != NULL && tokenIsChar(token, ',') == false)
			return CSS_INVALID;

		consumeWhitespace(vector, ctx);
	}

#if 0
	/* If, after parsing the media list, we still have no media, 
	 * then it must be ALL. */
	if (ret == 0)
		ret = CSS_MEDIA_ALL;
#endif

	*media = ret;

	return CSS_OK;
}

