/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2016 John-Mark Bell <jmb@netsurf-browser.org>
 */

/* https://drafts.csswg.org/mediaqueries/ */

#include <libcss/fpmath.h>

#include "bytecode/bytecode.h"
#include "parse/mq.h"
#include "parse/properties/utils.h"
#include "utils/utils.h"

static css_error mq_parse_ratio(css_language *c,
		const parserutils_vector *vector, int *ctx,
		const css_token *numerator, css_fixed *ratio)
{
	const css_token *token;
	css_fixed num, den;
	size_t num_len, den_len;

	/* NUMBER ws* '/' ws* NUMBER */

	/* numerator, ws* already consumed */

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, '/') == false) {
		return CSS_INVALID;
	}

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || token->type != CSS_TOKEN_NUMBER) {
		return CSS_INVALID;
	}

	num = css__number_from_lwc_string(numerator->idata, true, &num_len);
	den = css__number_from_lwc_string(token->idata, true, &den_len);

	*ratio = css_divide_fixed(num, den);

	return CSS_OK;
}

static css_error mq_create_feature(
		lwc_string *name,
		css_mq_feature **feature)
{
	css_mq_feature *f;

	f = malloc(sizeof(*f));
	if (f == NULL) {
		return CSS_NOMEM;
	}

	memset(f, 0, sizeof(*f));

	f->name = lwc_string_ref(name);

	*feature = f;

	return CSS_OK;
}

static css_error mq_populate_value(css_mq_value *value,
		const css_token *token)
{
	if (token->type == CSS_TOKEN_NUMBER) {
		size_t num_len;
		value->type = CSS_MQ_VALUE_TYPE_NUM;
		value->data.num_or_ratio = css__number_from_lwc_string(
				token->idata, false, &num_len);
	} else if (token->type == CSS_TOKEN_DIMENSION) {
		size_t len = lwc_string_length(token->idata);
		const char *data = lwc_string_data(token->idata);
		uint32_t unit = UNIT_PX;
		size_t consumed;

		value->type == CSS_MQ_VALUE_TYPE_DIM;
		value->data.dim.len = css__number_from_lwc_string(
				token->idata, false, &consumed);
		error = css__parse_unit_keyword(data + consumed, len - consumed,
				&unit);
		if (error != CSS_OK) {
			return error;
		}
		value->data.dim.unit = temp_unit;
	} else if (token->type == CSS_TOKEN_IDENT) {
		value->type = CSS_MQ_VALUE_TYPE_IDENT;
		value->data.ident = lwc_string_ref(token->idata);
	}

	return CSS_OK;
}

static css_error mq_parse_op(const css_token *token,
		css_mq_feature_op *op)
{
	size_t len;
	const char *data;

	if (token == NULL || token->type != CSS_TOKEN_CHAR)
		return CSS_INVALID;

	len = lwc_string_length(token->idata);
	data = lwc_string_data(token->idata);

	if (len == 2) {
		if (strncasecmp(data, "<=", 2) == 0)
			*op = CSS_MQ_FEATURE_OP_LTE;
		else if (strncasecmp(data, ">=", 2) == 0)
			*op = CSS_MQ_FEATURE_OP_GTE;
		else
			return CSS_INVALID;
	} else if (len == 1) {
		if (*data == '<')
			*op = CSS_MQ_FEATURE_OP_LT;
		else if (*data == '=')
			*op = CSS_MQ_FEATURE_OP_EQ;
		else if (*data == '>')
			*op = CSS_MQ_FEATURE_OP_GT;
		else
			return CSS_INVALID;
	} else {
		return CSS_INVALID;
	}

	return CSS_OK;
}

static css_error mq_parse_range(css_language *c,
		const parserutils_vector *vector, int *ctx,
		const css_token *name_or_value,
		css_mq_feature **feature)
{
	const css_token *token, *value_or_name, *name = NULL, *value2 = NULL;
	css_mq_feature *result;
	css_mq_feature_op op, op2;
	css_fixed ratio, ratio2;
	bool name_first = false, value_is_ratio = false, value2_is_ratio = false, match;
	css_error error;

	/* <mf-range> = <mf-name> [ '<' | '>' ]? '='? <mf-value>
	 *            | <mf-value> [ '<' | '>' ]? '='? <mf-name>
	 *            | <mf-value> '<' '='? <mf-name> '<' '='? <mf-value>
	 *            | <mf-value> '>' '='? <mf-name> '>' '='? <mf-value>
	 */

	if (name_or_value == NULL || (name_or_value->type != CSS_TOKEN_NUMBER &&
			name_or_value->type != CSS_TOKEN_DIMENSION &&
			name_or_value->type != CSS_TOKEN_IDENT)) {
		return CSS_INVALID;
	}

	consumeWhitespace(vector, ctx);

	/* Name-or-value */
	if (name_or_value->type == CSS_TOKEN_NUMBER &&
			tokenIsChar(parserutils_vector_peek(vector, *ctx), '/')) {
		/* ratio */
		error = mq_parse_ratio(c, vector, ctx, token, &ratio);
		if (error != CSS_OK) {
			return error;
		}

		consumeWhitespace(vector, ctx);

		value_is_ratio = true;
	} else if (name_or_value->type == CSS_TOKEN_IDENT &&
			lwc_string_caseless_isequal(name_or_value->idata,
				c->strings[INFINITE], &match) == lwc_error_ok && 
			match == false) {
		/* The only ident permitted for mf-value is 'infinite', thus must have name */
		name = name_or_value;
		name_first = true;
	}

	/* Op */
	token = parserutils_vector_iterate(vector, ctx);
	error = mq_parse_op(token, &op);
	if (error != CSS_OK) {
		return error;
	}

	consumeWhitespace(vector, ctx);

	/* Value-or-name */
	value_or_name = parserutils_vector_iterate(vector, ctx);
	if (value_or_name == NULL || (value_or_name->type != CSS_TOKEN_NUMBER &&
			value_or_name->type != CSS_TOKEN_DIMENSION &&
			value_or_name->type != CSS_TOKEN_IDENT)) {
		return CSS_INVALID;
	}

	if (name == NULL) {
		if (value_or_name->type != CSS_TOKEN_IDENT) {
			return CSS_INVALID;
		} else {
			name = value_or_name;
		}
	}

	consumeWhitespace(vector, ctx);

	if (value_or_name->type == CSS_TOKEN_NUMBER &&
			tokenIsChar(parserutils_vector_peek(vector, *ctx), '/')) {
		/* ratio */
		error = mq_parse_ratio(c, vector, ctx, token, &ratio);
		if (error != CSS_OK) {
			return error;
		}

		consumeWhitespace(vector, ctx);

		value_is_ratio = true;
	}

	token = parserutils_vector_peek(vector, *ctx);
	if (name_first == false && token != NULL && tokenIsChar(token, ')') == false) {
		/* Op2 */
		token = parserutils_vector_iterate(vector, ctx);
		error = mq_parse_op(token, &op2);
		if (error != CSS_OK) {
			return error;
		}

		consumeWhitespace(vector, ctx);

		/* Validate operators: must both be LT(E) or GT(E) */
		if (op == CSS_MQ_FEATURE_OP_LT || op == CSS_MQ_FEATURE_OP_LTE) {
			if (op2 != CSS_MQ_FEATURE_OP_LT && op2 != CSS_MQ_FEATURE_OP_LTE) {
				return CSS_INVALID;
			}
		} else if (op == CSS_MQ_FEATURE_OP_GT || op == CSS_MQ_FEATURE_OP_GTE) {
		       if (op2 != CSS_MQ_FEATURE_OP_GT && op2 != CSS_MQ_FEATURE_OP_GTE) {
				return CSS_INVALID;
			}
		} else {
			return CSS_INVALID;
		}

		/* Value2 */
		value2 = parserutils_vector_iterate(vector, ctx);
		if (value2 == NULL || (value2->type != CSS_TOKEN_NUMBER &&
				value2->type != CSS_TOKEN_DIMENSION &&
				value2->type != CSS_TOKEN_IDENT)) {
			return CSS_INVALID;
		}

		consumeWhitespace(vector, ctx);

		if (value_or_name->type == CSS_TOKEN_NUMBER &&
				tokenIsChar(parserutils_vector_peek(vector, *ctx), '/')) {
			/* ratio */
			error = mq_parse_ratio(c, vector, ctx, token, &ratio2);
			if (error != CSS_OK) {
				return error;
			}

			consumeWhitespace(vector, ctx);

			value2_is_ratio = true;
		}
	}

	error = mq_create_feature(name->idata, &result);
	if (error != CSS_OK) {
		return error;
	}
	if (name_first) {
		/* Invert operator */
		if (op == CSS_MQ_FEATURE_OP_LT) {
			op = CSS_MQ_FEATURE_OP_GTE;
		} else if (op == CSS_MQ_FEATURE_OP_LTE) {
			op = CSS_MQ_FEATURE_OP_GT;
		} else if (op == CSS_MQ_FEATURE_OP_GT) {
			op = CSS_MQ_FEATURE_OP_LTE;
		} else if (op == CSS_MQ_FEATURE_OP_GTE) {
			op = CSS_MQ_FEATURE_OP_LT;
		}
	}
	result->op = op;
	if (value_is_ratio) {
		result->value.type = CSS_MQ_VALUE_TYPE_RATIO;
		result->value.data.num_or_ratio = ratio;
	} else {
		/* num/dim/ident */
		error = mq_populate_value(&result->value, token);
		if (error != CSS_OK) {
			free(result);
			return error;
		}
	}
	if (value2 != NULL) {
		result->op2 = op2;
		if (value2_is_ratio) {
			result->value2.type = CSS_MQ_VALUE_TYPE_RATIO;
			result->value2.data.num_or_ratio = ratio;
		} else {
			/* num/dim/ident */
			error = mq_populate_value(&result->value2, token);
			if (error != CSS_OK) {
				/* TODO: clean up result properly? */
				free(result);
				return error;
			}
		}
	}

	*feature = result;

	return CSS_OK;
}

static css_error mq_parse_media_feature(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_mq_feature **feature)
{
	const css_token *name_or_value, *token;
	css_mq_feature *result;
	css_error error;

	/* <media-feature> = ( [ <mf-plain> | <mf-boolean> | <mf-range> ] )
	 * <mf-plain> = <mf-name> : <mf-value>
	 * <mf-boolean> = <mf-name>
	 * <mf-name> = <ident>
	 * <mf-value> = <number> | <dimension> | <ident> | <ratio>
	 */

	/* ( already consumed */

	name_or_value = parserutils_vector_iterate(vector, ctx);
	if (name_or_value == NULL)
		return CSS_INVALID;

	if (name_or_value->type == CSS_TOKEN_IDENT) {
		consumeWhitespace(vector, ctx);

		token = parserutils_vector_peek(vector, *ctx);
		if (tokenIsChar(token, ')')) {
			/* mf-boolean */
			error = mq_create_feature(name_or_value->idata, &result);
			if (error != CSS_OK) {
				return error;
			}

			result->op = CSS_MQ_FEATURE_OP_BOOL;
		} else if (tokenIsChar(token, ':')) {
			/* mf-plain */
			parserutils_vector_iterate(vector, ctx);

			consumeWhitespace(vector, ctx);

			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || (token->type != CSS_TOKEN_NUMBER &&
					token->type != CSS_TOKEN_DIMENSION &&
					token->type != CSS_TOKEN_IDENT)) {
				return CSS_INVALID;
			}

			consumeWhitespace(vector, ctx);

			error = mq_create_feature(name_or_value->idata, &result);
			if (error != CSS_OK) {
				return error;
			}
			result->op = CSS_MQ_FEATURE_OP_EQ;

			if (token->type == CSS_TOKEN_NUMBER &&
					tokenIsChar(parserutils_vector_peek(vector, *ctx), '/')) {
				/* ratio */
				css_fixed ratio;

				error = mq_parse_ratio(c, vector, ctx, token, &ratio);
				if (error != CSS_OK) {
					free(result);
					return error;
				}

				result->value.type = CSS_MQ_VALUE_TYPE_RATIO;
				result->value.data.num_or_ratio = ratio;
			} else {
				/* num/dim/ident */
				error = mq_populate_value(&result->value, token);
				if (error != CSS_OK) {
					free(result);
					return error;
				}
			}

			consumeWhitespace(vector, ctx);
		} else {
			/* mf-range */
			error = mq_parse_range(c, vector, ctx, name_or_value, &result);
			if (error != CSS_OK) {
				return error;
			}

			consumeWhitespace(vector, ctx);
		}
	} else {
		/* mf-range */
		error = mq_parse_range(c, vector, ctx, name_or_value, &result);
		if (error != CSS_OK) {
			return error;
		}

		consumeWhitespace(vector, ctx);
	}

	token = parserutils_vector_iterate(vector, ctx);
	if (tokenIsChar(token, ')') == false) {
		/* TODO: clean up result */
		return CSS_INVALID;
	}

	*feature = result;

	return CSS_OK;
}

static css_error mq_parse_media_in_parens()
{
	/* <media-in-parens> = ( <media-condition> ) | <media-feature> | <general-enclosed>
	 * <general-enclosed> = [ <function-token> <any-value> ) ] | ( <ident> <any-value> )
	 */

	//LPAREN -> condition-or-feature
	//	  "not" or LPAREN -> condition
	//	  IDENT | NUMBER | DIMENSION | RATIO -> feature

}

static css_error mq_parse_condition()
{
	/* <media-condition> = <media-not> | <media-in-parens> [ <media-and>* | <media-or>* ]
	 * <media-condition-without-or> = <media-not> | <media-in-parens> <media-and>*
	 * <media-not> = not <media-in-parens>
	 * <media-and> = and <media-in-parens>
	 * <media-or> = or <media-in-parens>
	 */
}

css_error css__mq_parse_media_list(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_mq_query **media)
{
	css_mq_query *ret = NULL;
	const css_token *token;

	/* <media-query-list> = <media-query> [ COMMA <media-query> ]*
	 * <media-query> = <media-condition>
	 *               | [ not | only ]? <media-type> [ and <media-condition-without-or> ]?
	 * <media-type> = <ident> (except "not", "and", "or", "only")
	 *
	 */

	/* if {[(, push }]) to stack
	 * if func, push ) to stack
	 * on error, scan forward until stack is empty (or EOF), popping matching tokens off stack
	 * if stack is empty, the next input token must be comma or EOF
	 * if comma, consume, and start again from the next input token
	 */

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

