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
 * Test whether the rule applies for current media.
 *
 * \param rule		Rule to test
 * \meaid media		Current media type(s)
 * \return true iff chain's rule applies for media
 */
static inline bool mq_rule_good_for_media(const css_rule *rule, uint64_t media)
{
	bool applies = true;
	const css_rule *ancestor = rule;

	while (ancestor != NULL) {
		const css_rule_media *m = (const css_rule_media *) ancestor;

		if (ancestor->type == CSS_RULE_MEDIA &&
				(m->media & media) == 0) {
			applies = false;
			break;
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
