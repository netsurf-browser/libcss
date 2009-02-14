/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <assert.h>
#include <string.h>

#include <parserutils/utils/stack.h>

#include "stylesheet.h"
#include "lex/lex.h"
#include "parse/language.h"
#include "parse/parse.h"
#include "parse/propstrings.h"

#include "utils/parserutilserror.h"
#include "utils/utils.h"

typedef struct context_entry {
	css_parser_event type;		/**< Type of entry */
	void *data;			/**< Data for context */
} context_entry;

/**
 * Context for a CSS language parser
 */
struct css_language {
	css_stylesheet *sheet;		/**< The stylesheet to parse for */

#define STACK_CHUNK 32
	parserutils_stack *context;	/**< Context stack */

	enum {
		BEFORE_CHARSET,
		BEFORE_RULES,
		HAD_RULE,
	} state;			/**< State flag, for at-rule handling */

	/** \todo These should be statically allocated */
	/** Interned strings */
	const parserutils_hash_entry *strings[LAST_KNOWN];

	css_allocator_fn alloc;		/**< Memory (de)allocation function */
	void *pw;			/**< Client's private data */
};

/* Event handlers */
static css_error language_handle_event(css_parser_event type, 
		const parserutils_vector *tokens, void *pw);
static inline css_error handleStartStylesheet(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleEndStylesheet(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleStartRuleset(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleEndRuleset(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleStartAtRule(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleEndAtRule(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleStartBlock(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleEndBlock(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleBlockContent(css_language *c, 
		const parserutils_vector *vector);
static inline css_error handleDeclaration(css_language *c, 
		const parserutils_vector *vector);

/* Selector list parsing */
static inline css_error parseClass(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector_detail *specific);
static inline css_error parseAttrib(css_language *c, 
		const parserutils_vector *vector, int *ctx,
		css_selector_detail *specific);
static inline css_error parsePseudo(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector_detail *specific);
static inline css_error parseSpecific(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector **parent);
static inline css_error parseSelectorSpecifics(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector **parent);
static inline css_error parseSimpleSelector(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_selector **result);
static inline css_error parseCombinator(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_combinator *result);
static inline css_error parseSelector(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_selector **result);
static inline css_error parseSelectorList(css_language *c, 
		const parserutils_vector *vector, css_rule *rule);

/* Declaration parsing */
static inline css_error parseProperty(css_language *c,
		const css_token *property, const parserutils_vector *vector,
		int *ctx, css_rule *rule);

/* Helpers */
static inline void consumeWhitespace(const parserutils_vector *vector, 
		int *ctx);
static inline bool tokenIsChar(const css_token *token, uint8_t c);

/**
 * Create a CSS language parser
 *
 * \param sheet     The stylesheet object to parse for
 * \param parser    The core parser object to use
 * \param alloc     Memory (de)allocation function
 * \param pw        Pointer to client-specific private data
 * \param language  Pointer to location to receive parser object
 * \return CSS_OK on success,
 *         CSS_BADPARM on bad parameters,
 *         CSS_NOMEM on memory exhaustion
 */
css_error css_language_create(css_stylesheet *sheet, css_parser *parser,
		css_allocator_fn alloc, void *pw, void **language)
{
	css_language *c;
	css_parser_optparams params;
	parserutils_error perror;
	css_error error;

	if (sheet == NULL || parser == NULL || alloc == NULL || 
			language == NULL)
		return CSS_BADPARM;

	c = alloc(NULL, sizeof(css_language), pw);
	if (c == NULL)
		return CSS_NOMEM;

	perror = parserutils_stack_create(sizeof(context_entry), 
			STACK_CHUNK, (parserutils_alloc) alloc, pw, 
			&c->context);
	if (perror != PARSERUTILS_OK) {
		alloc(c, 0, pw);
		return css_error_from_parserutils_error(perror);
	}

	/* Intern all known strings */
	for (int i = 0; i < LAST_KNOWN; i++) {
		c->strings[i] = css_parser_dict_add(parser,
				(const uint8_t *) stringmap[i].data, 
				stringmap[i].len);
		if (c->strings[i] == NULL) {
			parserutils_stack_destroy(c->context);
			alloc(c, 0, pw);
			return CSS_NOMEM;
		}
	}

	params.event_handler.handler = language_handle_event;
	params.event_handler.pw = c;
	error = css_parser_setopt(parser, CSS_PARSER_EVENT_HANDLER, &params);
	if (error != CSS_OK) {
		parserutils_stack_destroy(c->context);
		alloc(c, 0, pw);
		return error;
	}

	c->sheet = sheet;
	c->state = BEFORE_CHARSET;
	c->alloc = alloc;
	c->pw = pw;

	*language = c;

	return CSS_OK;
}

/**
 * Destroy a CSS language parser
 *
 * \param language  The parser to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_language_destroy(css_language *language)
{
	if (language == NULL)
		return CSS_BADPARM;

	parserutils_stack_destroy(language->context);

	language->alloc(language, 0, language->pw);

	return CSS_OK;
}

/**
 * Handler for core parser events
 *
 * \param type    The event type
 * \param tokens  Vector of tokens read since last event, or NULL
 * \param pw      Pointer to handler context
 * \return CSS_OK on success, CSS_INVALID to indicate parse error, 
 *         appropriate error otherwise.
 */
css_error language_handle_event(css_parser_event type, 
		const parserutils_vector *tokens, void *pw)
{
	css_language *language = (css_language *) pw;

	switch (type) {
	case CSS_PARSER_START_STYLESHEET:
		return handleStartStylesheet(language, tokens);
	case CSS_PARSER_END_STYLESHEET:
		return handleEndStylesheet(language, tokens);
	case CSS_PARSER_START_RULESET:
		return handleStartRuleset(language, tokens);
	case CSS_PARSER_END_RULESET:
		return handleEndRuleset(language, tokens);
	case CSS_PARSER_START_ATRULE:
		return handleStartAtRule(language, tokens);
	case CSS_PARSER_END_ATRULE:
		return handleEndAtRule(language, tokens);
	case CSS_PARSER_START_BLOCK:
		return handleStartBlock(language, tokens);
	case CSS_PARSER_END_BLOCK:
		return handleEndBlock(language, tokens);
	case CSS_PARSER_BLOCK_CONTENT:
		return handleBlockContent(language, tokens);
	case CSS_PARSER_DECLARATION:
		return handleDeclaration(language, tokens);
	}

	return CSS_OK;
}

/******************************************************************************
 * Parser stages                                                              *
 ******************************************************************************/

css_error handleStartStylesheet(css_language *c, 
		const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry entry = { CSS_PARSER_START_STYLESHEET, NULL };

	UNUSED(vector);

	assert(c != NULL);

	perror = parserutils_stack_push(c->context, (void *) &entry);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleEndStylesheet(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry *entry;

	UNUSED(vector);

	assert(c != NULL);

	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->type != CSS_PARSER_START_STYLESHEET)
		return CSS_INVALID;

	perror = parserutils_stack_pop(c->context, NULL);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleStartRuleset(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	css_error error;
	context_entry entry = { CSS_PARSER_START_RULESET, NULL };
	css_rule *rule = NULL;

	assert(c != NULL);

	error = css_stylesheet_rule_create(c->sheet, CSS_RULE_SELECTOR, &rule);
	if (error != CSS_OK)
		return error;

	error = parseSelectorList(c, vector, rule);
	if (error != CSS_OK) {
		css_stylesheet_rule_destroy(c->sheet, rule);
		return error;
	}

	entry.data = rule;

	perror = parserutils_stack_push(c->context, (void *) &entry);
	if (perror != PARSERUTILS_OK) {
		css_stylesheet_rule_destroy(c->sheet, rule);
		return css_error_from_parserutils_error(perror);
	}

	error = css_stylesheet_add_rule(c->sheet, rule);
	if (error != CSS_OK) {
		parserutils_stack_pop(c->context, NULL);
		css_stylesheet_rule_destroy(c->sheet, rule);
		return error;
	}

	/* Flag that we've had a valid rule, so @import/@charset have 
	 * no effect. */
	c->state = HAD_RULE;

	/* Rule is now owned by the sheet, so no need to destroy it */

	return CSS_OK;
}

css_error handleEndRuleset(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry *entry;

	UNUSED(vector);

	assert(c != NULL);

	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->type != CSS_PARSER_START_RULESET)
		return CSS_INVALID;

	perror = parserutils_stack_pop(c->context, NULL);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleStartAtRule(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry entry = { CSS_PARSER_START_ATRULE, NULL };

	assert(c != NULL);

	/* vector contains: ATKEYWORD ws any0 */
	const css_token *token = NULL;
	const css_token *atkeyword = NULL;
	int32_t ctx = 0;

	atkeyword = parserutils_vector_iterate(vector, &ctx);

	consumeWhitespace(vector, &ctx);

	/* We now have an ATKEYWORD and the context for the start of any0, if 
	 * there is one */
	assert(atkeyword != NULL && atkeyword->type == CSS_TOKEN_ATKEYWORD);

	if (atkeyword->ilower == c->strings[CHARSET]) {
		if (c->state == BEFORE_CHARSET) {
			const css_token *charset;
			css_rule *rule;
			css_error error;

			/* any0 = STRING */
			if (ctx == 0)
				return CSS_INVALID;

			charset = parserutils_vector_iterate(vector, &ctx);
			if (charset == NULL || 
					charset->type != CSS_TOKEN_STRING)
				return CSS_INVALID;

			token = parserutils_vector_iterate(vector, &ctx);
			if (token != NULL)
				return CSS_INVALID;

			error = css_stylesheet_rule_create(c->sheet, 
					CSS_RULE_CHARSET, &rule);
			if (error != CSS_OK)
				return error;

			error = css_stylesheet_rule_set_charset(c->sheet, rule,
					charset->idata);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			error = css_stylesheet_add_rule(c->sheet, rule);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			/* Rule is now owned by the sheet, 
			 * so no need to destroy it */

			c->state = BEFORE_RULES;
		} else {
			return CSS_INVALID;
		}
	} else if (atkeyword->ilower == c->strings[IMPORT]) {
		if (c->state != HAD_RULE) {
			css_rule *rule;
			css_media_type media = 0;
			css_error error;

			/* any0 = (STRING | URI) ws 
			 *        (IDENT ws (',' ws IDENT ws)* )? */
			const css_token *uri = 
				parserutils_vector_iterate(vector, &ctx);
			if (uri == NULL || (uri->type != CSS_TOKEN_STRING && 
					uri->type != CSS_TOKEN_URI))
				return CSS_INVALID;

			consumeWhitespace(vector, &ctx);

			/* Parse media list */
			token = parserutils_vector_iterate(vector, &ctx);

			while (token != NULL) {
				if (token->type != CSS_TOKEN_IDENT)
					return CSS_INVALID;

				if (token->ilower == c->strings[AURAL]) {
					media |= CSS_MEDIA_AURAL;
				} else if (token->ilower == 
						c->strings[BRAILLE]) {
					media |= CSS_MEDIA_BRAILLE;
				} else if (token->ilower ==
						c->strings[EMBOSSED]) {
					media |= CSS_MEDIA_EMBOSSED;
				} else if (token->ilower ==
						c->strings[HANDHELD]) {
					media |= CSS_MEDIA_HANDHELD;
				} else if (token->ilower ==
						c->strings[PRINT]) {
					media |= CSS_MEDIA_PRINT;
				} else if (token->ilower ==
						c->strings[PROJECTION]) {
					media |= CSS_MEDIA_PROJECTION;
				} else if (token->ilower ==
						c->strings[SCREEN]) {
					media |= CSS_MEDIA_SCREEN;
				} else if (token->ilower ==
						c->strings[SPEECH]) {
					media |= CSS_MEDIA_SPEECH;
				} else if (token->ilower == c->strings[TTY]) {
					media |= CSS_MEDIA_TTY;
				} else if (token->ilower == c->strings[TV]) {
					media |= CSS_MEDIA_TV;
				} else if (token->ilower == c->strings[ALL]) {
					media |= CSS_MEDIA_ALL;
				} else
					return CSS_INVALID;

				consumeWhitespace(vector, &ctx);

				token = parserutils_vector_iterate(vector, 
						&ctx);
				if (token != NULL && tokenIsChar(token, ',') == 
						false)
					return CSS_INVALID;

				consumeWhitespace(vector, &ctx);
			}

			error = css_stylesheet_rule_create(c->sheet, 
					CSS_RULE_IMPORT, &rule);
			if (error != CSS_OK)
				return error;

			error = css_stylesheet_rule_set_nascent_import(c->sheet,
					rule, uri->idata, media);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			error = css_stylesheet_add_rule(c->sheet, rule);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			/* Rule is now owned by the sheet, 
			 * so no need to destroy it */

			c->state = BEFORE_RULES;
		} else {
			return CSS_INVALID;
		}
#if 0
	/** \todo these depend on nested block support, so we'll disable them
	 * until we have such a thing. This means that we'll ignore the entire
	 * at-rule until then */
	} else if (atkeyword->ilower == c->strings[MEDIA]) {
		/** \todo any0 = IDENT ws (',' ws IDENT ws)* */
		c->state = HAD_RULE;
	} else if (atkeyword->ilower == c->strings[PAGE]) {
		/** \todo any0 = (':' IDENT)? ws */
		c->state = HAD_RULE;
#endif
	} else {
		return CSS_INVALID;
	}

	entry.data = (void *) atkeyword->ilower;

	perror = parserutils_stack_push(c->context, (void *) &entry);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleEndAtRule(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry *entry;

	UNUSED(vector);

	assert(c != NULL);

	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->type != CSS_PARSER_START_ATRULE)
		return CSS_INVALID;

	perror = parserutils_stack_pop(c->context, NULL);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleStartBlock(css_language *c, const parserutils_vector *vector)
{
	UNUSED(c);
	UNUSED(vector);

	/* We don't care about blocks. In CSS2.1 they're always attached to 
	 * rulesets or at-rules. */

	return CSS_OK;
}

css_error handleEndBlock(css_language *c, const parserutils_vector *vector)
{
	UNUSED(c);
	UNUSED(vector);

	/* We don't care about blocks. In CSS 2.1 they're always attached to 
	 * rulesets or at-rules. */

	return CSS_OK;
}

css_error handleBlockContent(css_language *c, const parserutils_vector *vector)
{
	UNUSED(c);
	UNUSED(vector);

	/* In CSS 2.1, block content comprises either declarations (if the 
	 * current block is associated with @page or a selector), or rulesets 
	 * (if the current block is associated with @media). */

	/** \todo implement nested blocks */

	return CSS_OK;
}

css_error handleDeclaration(css_language *c, const parserutils_vector *vector)
{
	css_error error;
	const css_token *token, *ident;
	int ctx = 0;
	context_entry *entry;
	css_rule *rule;

	/* Locations where declarations are permitted:
	 *
	 * + In @page
	 * + In ruleset
	 */
	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || (entry->type != CSS_PARSER_START_RULESET &&
			entry->type != CSS_PARSER_START_ATRULE))
		return CSS_INVALID;

	rule = entry->data;
	if (rule == NULL || (rule->type != CSS_RULE_SELECTOR && 
			rule->type != CSS_RULE_PAGE))
		return CSS_INVALID;

	/* IDENT ws ':' ws value 
	 * 
	 * In CSS 2.1, value is any1, so '{' or ATKEYWORD => parse error
	 */
	ident = parserutils_vector_iterate(vector, &ctx);
	if (ident == NULL || ident->type != CSS_TOKEN_IDENT)
		return CSS_INVALID;

	consumeWhitespace(vector, &ctx);

	token = parserutils_vector_iterate(vector, &ctx);
	if (token == NULL || tokenIsChar(token, ':') == false)
		return CSS_INVALID;

	consumeWhitespace(vector, &ctx);

	error = parseProperty(c, ident, vector, &ctx, rule);
	if (error != CSS_OK)
		return error;

	return CSS_OK;
}

/******************************************************************************
 * Selector list parsing functions                                            *
 ******************************************************************************/

css_error parseClass(css_language *c, const parserutils_vector *vector, 
		int *ctx, css_selector_detail *specific)
{
	const css_token *token;

	/* class     -> '.' IDENT */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, '.') == false)
		return CSS_INVALID;

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || token->type != CSS_TOKEN_IDENT)
		return CSS_INVALID;

	return css_stylesheet_selector_detail_init(c->sheet, 
			CSS_SELECTOR_CLASS, token->idata, NULL, specific);
}

css_error parseAttrib(css_language *c, const parserutils_vector *vector, 
		int *ctx, css_selector_detail *specific)
{
	const css_token *token, *name, *value = NULL;
	css_selector_type type = CSS_SELECTOR_ATTRIBUTE;

	/* attrib    -> '[' ws IDENT ws [
	 *                     [ '=' | INCLUDES | DASHMATCH ] ws
	 *                     [ IDENT | STRING ] ws ]? ']'
	 */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, '[') == false)
		return CSS_INVALID;

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || token->type != CSS_TOKEN_IDENT)
		return CSS_INVALID;

	name = token;

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL)
		return CSS_INVALID;

	if (tokenIsChar(token, ']') == false) {
		if (tokenIsChar(token, '='))
			type = CSS_SELECTOR_ATTRIBUTE_EQUAL;
		else if (token->type == CSS_TOKEN_INCLUDES)
			type = CSS_SELECTOR_ATTRIBUTE_INCLUDES;
		else if (token->type == CSS_TOKEN_DASHMATCH)
			type = CSS_SELECTOR_ATTRIBUTE_DASHMATCH;
		else
			return CSS_INVALID;

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);
		if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
				token->type != CSS_TOKEN_STRING))
			return CSS_INVALID;

		value = token;

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);
		if (token == NULL || tokenIsChar(token, ']') == false)
			return CSS_INVALID;
	}

	return css_stylesheet_selector_detail_init(c->sheet, type, 
			name->idata, value != NULL ? value->idata : NULL,
			specific);
}

css_error parsePseudo(css_language *c, const parserutils_vector *vector, 
		int *ctx, css_selector_detail *specific)
{
	const css_token *token, *name, *value = NULL;
	css_selector_type type;

	/* pseudo    -> ':' [ IDENT | FUNCTION ws IDENT? ws ')' ] */

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, ':') == false)
		return CSS_INVALID;

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT && 
			token->type != CSS_TOKEN_FUNCTION))
		return CSS_INVALID;

	name = token;

	if (token->type == CSS_TOKEN_FUNCTION) {
		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);

		if (token != NULL && token->type == CSS_TOKEN_IDENT) {
			value = token;

			consumeWhitespace(vector, ctx);

			token = parserutils_vector_iterate(vector, ctx);
		}

		if (token == NULL || tokenIsChar(token, ')') == false)
			return CSS_INVALID;
	}

	if (name->ilower == c->strings[FIRST_CHILD] ||
			name->ilower == c->strings[LINK] || 
			name->ilower == c->strings[VISITED] || 
			name->ilower == c->strings[HOVER] || 
			name->ilower == c->strings[ACTIVE] || 
			name->ilower == c->strings[FOCUS] || 
			name->ilower == c->strings[LANG] || 
			name->ilower == c->strings[LEFT] || 
			name->ilower == c->strings[RIGHT] || 
			name->ilower == c->strings[FIRST])
		type = CSS_SELECTOR_PSEUDO_CLASS;
	else if (name->ilower == c->strings[FIRST_LINE] ||
			name->ilower == c->strings[FIRST_LETTER] ||
			name->ilower == c->strings[BEFORE] ||
			name->ilower == c->strings[AFTER])
		type = CSS_SELECTOR_PSEUDO_ELEMENT;
	else
		return CSS_INVALID;

	return css_stylesheet_selector_detail_init(c->sheet, 
			type, name->idata, value != NULL ? value->idata : NULL,
			specific);
}

css_error parseSpecific(css_language *c, 
		const parserutils_vector *vector, int *ctx,
		css_selector **parent)
{
	css_error error;
	const css_token *token;
	css_selector_detail specific;

	/* specific  -> [ HASH | class | attrib | pseudo ] */

	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL)
		return CSS_INVALID;

	if (token->type == CSS_TOKEN_HASH) {
		error = css_stylesheet_selector_detail_init(c->sheet,
				CSS_SELECTOR_ID, token->idata, NULL, &specific);
		if (error != CSS_OK)
			return error;

		parserutils_vector_iterate(vector, ctx);
	} else if (tokenIsChar(token, '.')) {
		error = parseClass(c, vector, ctx, &specific);
		if (error != CSS_OK)
			return error;
	} else if (tokenIsChar(token, '[')) {
		error = parseAttrib(c, vector, ctx, &specific);
		if (error != CSS_OK)
			return error;
	} else if (tokenIsChar(token, ':')) {
		error = parsePseudo(c, vector, ctx, &specific);
		if (error != CSS_OK)
			return error;
	} else {
		return CSS_INVALID;
	}

	return css_stylesheet_selector_append_specific(c->sheet, parent, 
			&specific);
}

css_error parseSelectorSpecifics(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector **parent)
{
	css_error error;
	const css_token *token;

	/* specifics -> specific* */
	while ((token = parserutils_vector_peek(vector, *ctx)) != NULL &&
			token->type != CSS_TOKEN_S && 
			tokenIsChar(token, '+') == false &&
			tokenIsChar(token, '>') == false &&
			tokenIsChar(token, ',') == false) {
		error = parseSpecific(c, vector, ctx, parent);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

css_error parseSimpleSelector(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_selector **result)
{
	css_error error;
	const css_token *token;
	css_selector *selector;

	/* simple_selector -> element_name specifics
	 *                 -> specific specifics
	 * element_name    -> IDENT | '*'
	 */

	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL)
		return CSS_INVALID;

	if (token->type == CSS_TOKEN_IDENT || tokenIsChar(token, '*')) {
		/* Have element name */
		error = css_stylesheet_selector_create(c->sheet,
				token->idata, &selector);
		if (error != CSS_OK)
			return error;

		parserutils_vector_iterate(vector, ctx);
	} else {
		/* Universal selector */
		error = css_stylesheet_selector_create(c->sheet,
				c->strings[UNIVERSAL], &selector);
		if (error != CSS_OK)
			return error;

		/* Ensure we have at least one specific selector */
		error = parseSpecific(c, vector, ctx, &selector);
		if (error != CSS_OK) {
			css_stylesheet_selector_destroy(c->sheet, selector);
			return error;
		}
	}

	error = parseSelectorSpecifics(c, vector, ctx, &selector);
	if (error != CSS_OK) {
		css_stylesheet_selector_destroy(c->sheet, selector);
		return error;
	}

	*result = selector;

	return CSS_OK;
}

css_error parseCombinator(css_language *c, const parserutils_vector *vector,
		int *ctx, css_combinator *result)
{
	const css_token *token;
	css_combinator comb = CSS_COMBINATOR_NONE;

	/* combinator      -> ws '+' ws | ws '>' ws | ws1 */

	UNUSED(c);

	while ((token = parserutils_vector_peek(vector, *ctx)) != NULL) {
		if (tokenIsChar(token, '+'))
			comb = CSS_COMBINATOR_SIBLING;
		else if (tokenIsChar(token, '>'))
			comb = CSS_COMBINATOR_PARENT;
		else if (token->type == CSS_TOKEN_S)
			comb = CSS_COMBINATOR_ANCESTOR;
		else
			break;

		parserutils_vector_iterate(vector, ctx);

		/* If we've seen a '+' or '>', we're done. */
		if (comb != CSS_COMBINATOR_ANCESTOR)
			break;
	}

	/* No valid combinator found */
	if (comb == CSS_COMBINATOR_NONE)
		return CSS_INVALID;

	/* Consume any trailing whitespace */
	consumeWhitespace(vector, ctx);

	*result = comb;

	return CSS_OK;
}

css_error parseSelector(css_language *c, const parserutils_vector *vector, 
		int *ctx, css_selector **result)
{
	css_error error;
	const css_token *token = NULL;
	css_selector *selector = NULL;

	/* selector -> simple_selector [ combinator simple_selector ]* ws
	 * 
	 * Note, however, that, as combinator can be wholly whitespace,
	 * there's an ambiguity as to whether "ws" has been reached. We 
	 * resolve this by attempting to extract a combinator, then 
	 * recovering when we detect that we've reached the end of the
	 * selector.
	 */

	error = parseSimpleSelector(c, vector, ctx, &selector);
	if (error != CSS_OK)
		return error;
	*result = selector;

	while ((token = parserutils_vector_peek(vector, *ctx)) != NULL &&
			tokenIsChar(token, ',') == false) {
		css_combinator comb = CSS_COMBINATOR_NONE;
		css_selector *other = NULL;

		error = parseCombinator(c, vector, ctx, &comb);
		if (error != CSS_OK)
			return error;

		/* In the case of "html , body { ... }", the whitespace after
		 * "html" and "body" will be considered an ancestor combinator.
		 * This clearly is not the case, however. Therefore, as a 
		 * special case, if we've got an ancestor combinator and there 
		 * are no further tokens, or if the next token is a comma,
		 * we ignore the supposed combinator and continue. */
		if (comb == CSS_COMBINATOR_ANCESTOR && 
				((token = parserutils_vector_peek(vector, 
					*ctx)) == NULL || 
				tokenIsChar(token, ',')))
			continue;

		error = parseSimpleSelector(c, vector, ctx, &other);
		if (error != CSS_OK)
			return error;

		*result = other;

		error = css_stylesheet_selector_combine(c->sheet,
				comb, selector, other);
		if (error != CSS_OK)
			return error;

		selector = other;
	}

	return CSS_OK;
}

css_error parseSelectorList(css_language *c, const parserutils_vector *vector,
		css_rule *rule)
{
	css_error error;
	const css_token *token = NULL;
	css_selector *selector = NULL;
	int ctx = 0;

	/* selector_list   -> selector [ ',' ws selector ]* */

	error = parseSelector(c, vector, &ctx, &selector);
	if (error != CSS_OK) {
		if (selector != NULL)
			css_stylesheet_selector_destroy(c->sheet, selector);
		return error;
	}

	assert(selector != NULL);

	error = css_stylesheet_rule_add_selector(c->sheet, rule, selector);
	if (error != CSS_OK) {
		css_stylesheet_selector_destroy(c->sheet, selector);
		return error;
	}

	while ((token = parserutils_vector_peek(vector, ctx)) != NULL) {
		token = parserutils_vector_iterate(vector, &ctx);
		if (tokenIsChar(token, ',') == false)
			return CSS_INVALID;

		consumeWhitespace(vector, &ctx);

		selector = NULL;

		error = parseSelector(c, vector, &ctx, &selector);
		if (error != CSS_OK) {
			if (selector != NULL) {
				css_stylesheet_selector_destroy(c->sheet, 
						selector);
			}
			return error;
		}

		assert(selector != NULL);

		error = css_stylesheet_rule_add_selector(c->sheet, rule, 
				selector);
		if (error != CSS_OK) {
			css_stylesheet_selector_destroy(c->sheet, selector);
			return error;
		}
	}

	return CSS_OK;
}

/******************************************************************************
 * Property parsing functions                                                 *
 ******************************************************************************/

#include "parse/properties.c"

css_error parseProperty(css_language *c, const css_token *property, 
		const parserutils_vector *vector, int *ctx, css_rule *rule)
{
	css_error error;
	css_prop_handler handler = NULL;
	int i = 0;
	css_style *style = NULL;

	/* Find property index */
	/** \todo improve on this linear search */
	for (i = FIRST_PROP; i <= LAST_PROP; i++) {
		if (property->ilower == c->strings[i])
			break;
	}
	if (i == LAST_PROP + 1)
		return CSS_INVALID;

	/* Get handler */
	handler = property_handlers[i - FIRST_PROP];
	assert(handler != NULL);

	/* Call it */
	error = handler(c, vector, ctx, &style);
	if (error != CSS_OK)
		return error;

	assert (style != NULL);

	/* Append style to rule */
	error = css_stylesheet_rule_append_style(c->sheet, rule, style);
	if (error != CSS_OK) {
		css_stylesheet_style_destroy(c->sheet, style);
		return error;
	}

	/* Style owned or destroyed by stylesheet, so forget about it */

	return CSS_OK;
}

/******************************************************************************
 * Helper functions                                                           *
 ******************************************************************************/

/**
 * Consume all leading whitespace tokens
 *
 * \param vector  The vector to consume from
 * \param ctx     The vector's context
 */
void consumeWhitespace(const parserutils_vector *vector, int *ctx)
{
	const css_token *token = NULL;

	while ((token = parserutils_vector_peek(vector, *ctx)) != NULL &&
			token->type == CSS_TOKEN_S)
		token = parserutils_vector_iterate(vector, ctx);
}

/**
 * Determine if a token is a character
 *
 * \param token  The token to consider
 * \param c      The character to match (lowercase ASCII only)
 * \return True if the token matches, false otherwise
 */
bool tokenIsChar(const css_token *token, uint8_t c)
{
	return token != NULL && token->type == CSS_TOKEN_CHAR && 
			token->ilower->len == 1 && token->ilower->data[0] == c;
}


