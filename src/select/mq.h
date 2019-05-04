/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef css_select_mq_h_
#define css_select_mq_h_

/**
 * Match media query conditions.
 *
 * \param[in] cond  Condition to match.
 * \return true if condition matches, otherwise false.
 */
static inline bool mq_match_condition(css_mq_cond *cond)
{
	/* TODO: Implement this. */
	(void) cond;
	return true;
}

/**
 * Test whether media query list matches current media.
 *
 * If anything in the list matches, it the list matches.  If none match
 * it doesn't match.
 *
 * \param m      Media query list.
 * \meaid media  Current media spec, to check against m.
 * \return true if media query list matches media
 */
static inline bool mq__list_match(const css_mq_query *m, uint64_t media)
{
	for (; m != NULL; m = m->next) {
		/* Check type */
		if (!!(m->type & media) != m->negate_type) {
			if (mq_match_condition(m->cond)) {
				/* We have a match, no need to look further. */
				return true;
			}
		}
	}

	return false;
}

/**
 * Test whether the rule applies for current media.
 *
 * \param rule   Rule to test
 * \param media  Current media type(s)
 * \return true iff chain's rule applies for media
 */
static inline bool mq_rule_good_for_media(const css_rule *rule, uint64_t media)
{
	bool applies = true;
	const css_rule *ancestor = rule;

	while (ancestor != NULL) {
		const css_rule_media *m = (const css_rule_media *) ancestor;

		if (ancestor->type == CSS_RULE_MEDIA) {
			applies = mq__list_match(m->media, media);
			if (applies == false) {
				break;
			}
		}

		if (ancestor->ptype != CSS_RULE_PARENT_STYLESHEET) {
			ancestor = ancestor->parent;
		} else {
			ancestor = NULL;
		}
	}

	return applies;
}

#endif
