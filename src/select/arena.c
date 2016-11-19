/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2015 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <string.h>

#include "select/arena.h"
#include "select/arena_hash.h"
#include "select/computed.h"

#define TU_SIZE 3037
#define TS_SIZE 5101

struct css_computed_uncommon *table_u[TU_SIZE];
struct css_computed_style *table_s[TS_SIZE];


static inline uint32_t css__arena_hash_uncommon(struct css_computed_uncommon *u)
{
	return css__arena_hash((const uint8_t *) &u->i, sizeof(u->i));
}


static inline uint32_t css__arena_hash_style(struct css_computed_style *s)
{
	return css__arena_hash((const uint8_t *) &s->i, sizeof(s->i));
}


static inline bool arena__compare_computed_page(
		const struct css_computed_page *a,
		const struct css_computed_page *b)
{
	if (a == NULL && b == NULL) {
		return true;

	} else if (a == NULL || b == NULL) {
		return false;
	}

	return memcmp(a, b, sizeof(struct css_computed_page)) == 0;
}


static inline bool arena__compare_computed_content_item(
		const struct css_computed_content_item *a,
		const struct css_computed_content_item *b)
{
	if (a == NULL && b == NULL) {
		return true;

	} else if (a == NULL || b == NULL) {
		return false;
	}

	if (a->type != b->type) {
		return false;
	}

	return memcmp(a, b, sizeof(struct css_computed_content_item)) == 0;
}


static inline bool arena__compare_css_computed_counter(
		const struct css_computed_counter *a,
		const struct css_computed_counter *b)
{
	bool match;

	if (a == NULL && b == NULL) {
		return true;

	} else if (a == NULL || b == NULL) {
		return false;
	}

	if (a->value == b->value &&
			lwc_string_isequal(a->name, b->name,
					&match) == lwc_error_ok &&
			match == true) {
		return true;
	}

	return false;
}

static inline bool arena__compare_string_list(
		lwc_string **a,
		lwc_string **b)
{
	if (a == NULL && b == NULL) {
		return true;

	} else if (a == NULL || b == NULL) {
		return false;
	}

	while (*a != NULL && *b != NULL) {
		bool match;

		if (lwc_string_isequal(*a, *b, &match) != lwc_error_ok ||
				match == false) {
			return false;
		}

		a++;
		b++;
	}

	if (*a != *b) {
		return false;
	}

	return true;
}


static inline bool css__arena_uncommon_is_equal(
		struct css_computed_uncommon *a,
		struct css_computed_uncommon *b)
{
	if (memcmp(&a->i, &b->i, sizeof(struct css_computed_uncommon_i)) != 0) {
		return false;
	}

	if (!arena__compare_css_computed_counter(
			a->counter_increment,
			b->counter_increment)) {
		return false;
	}

	if (!arena__compare_css_computed_counter(
			a->counter_reset,
			b->counter_reset)) {
		return false;
	}

	if (!arena__compare_computed_content_item(
			a->content,
			b->content)) {
		return false;
	}

	if (!arena__compare_string_list(
			a->cursor,
			b->cursor)) {
		return false;
	}

	return true;
}


static inline bool css__arena_style_is_equal(
		struct css_computed_style *a,
		struct css_computed_style *b)
{
	if (memcmp(&a->i, &b->i, sizeof(struct css_computed_style_i)) != 0) {
		return false;
	}

	if (!arena__compare_string_list(
			a->font_family,
			b->font_family)) {
		return false;
	}

	if (!arena__compare_string_list(
			a->quotes,
			b->quotes)) {
		return false;
	}

	if (!arena__compare_computed_page(
			a->page,
			b->page)) {
		return false;
	}

	return true;
}


static void css__arena_intern_uncommon(
		struct css_computed_uncommon **uncommon)
{
	struct css_computed_uncommon *u = *uncommon;
	uint32_t hash, index;

	/* Need to intern the uncommon block */
	hash = css__arena_hash_uncommon(u);
	index = hash % TU_SIZE;
	u->bin = index;

	if (table_u[index] == NULL) {
		/* Can just insert */
		table_u[index] = u;
		u->count = 1;
	} else {
		/* Check for existing */
		struct css_computed_uncommon *l = table_u[index];
		struct css_computed_uncommon *existing = NULL;

		do {
			if (css__arena_uncommon_is_equal(l, u)) {
				existing = l;
				break;
			}
			l = l->next;
		} while (l != NULL);

		if (existing != NULL) {
			css__computed_uncommon_destroy(u);
			existing->count++;
			*uncommon = existing;
		} else {
			/* Add to list */
			u->next = table_u[index];
			table_u[index] = u;
			u->count = 1;
		}
	}
}


/* Internally exported function, documented in src/select/arena.h */
css_error css__arena_intern_style(struct css_computed_style **style)
{
	struct css_computed_style *s = *style;
	uint32_t hash, index;

	/* Don't try to intern an already-interned computed style */
	if (s->count != 0) {
		return CSS_BADPARM;
	}

	if (s->i.uncommon != NULL) {
		if (s->i.uncommon->count != 0) {
			return CSS_BADPARM;
		}
		css__arena_intern_uncommon(&s->i.uncommon);
	}

	/* Need to intern the style block */
	hash = css__arena_hash_style(s);
	index = hash % TS_SIZE;
	s->bin = index;

	if (table_s[index] == NULL) {
		/* Can just insert */
		table_s[index] = s;
		s->count = 1;
	} else {
		/* Check for existing */
		struct css_computed_style *l = table_s[index];
		struct css_computed_style *existing = NULL;

		do {
			if (css__arena_style_is_equal(l, s)) {
				existing = l;
				break;
			}
			l = l->next;
		} while (l != NULL);

		if (existing != NULL) {
			s->i.uncommon = NULL;
			css_computed_style_destroy(s);
			existing->count++;
			*style = existing;
		} else {
			/* Add to list */
			s->next = table_s[index];
			table_s[index] = s;
			s->count = 1;
		}
	}

	return CSS_OK;
}


/* Internally exported function, documented in src/select/arena.h */
enum css_error css__arena_remove_style(struct css_computed_style *style)
{
	uint32_t index = style->bin;

	if (table_s[index] == NULL) {
		return CSS_BADPARM;

	} else {
		/* Check for existing */
		struct css_computed_style *l = table_s[index];
		struct css_computed_style *existing = NULL;
		struct css_computed_style *prev = NULL;

		do {
			if (css__arena_style_is_equal(l, style)) {
				existing = l;
				break;
			}
			prev = l;
			l = l->next;
		} while (l != NULL);

		if (existing != NULL) {
			if (prev != NULL) {
				prev->next = existing->next;
			} else {
				table_s[index] = existing->next;
			}
		} else {
			return CSS_BADPARM;
		}
	}

	return CSS_OK;
}


/* Internally exported function, documented in src/select/arena.h */
enum css_error css__arena_remove_uncommon_style(
		struct css_computed_uncommon *uncommon)
{
	uint32_t index = uncommon->bin;

	if (table_u[index] == NULL) {
		return CSS_BADPARM;

	} else {
		/* Check for existing */
		struct css_computed_uncommon *l = table_u[index];
		struct css_computed_uncommon *existing = NULL;
		struct css_computed_uncommon *prev = NULL;

		do {
			if (css__arena_uncommon_is_equal(l, uncommon)) {
				existing = l;
				break;
			}
			prev = l;
			l = l->next;
		} while (l != NULL);

		if (existing != NULL) {
			if (prev != NULL) {
				prev->next = existing->next;
			} else {
				table_u[index] = existing->next;
			}
		} else {
			return CSS_BADPARM;
		}
	}

	return CSS_OK;
}

