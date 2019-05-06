/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef css_select_mq_h_
#define css_select_mq_h_

static inline bool mq_match_feature_range_length_op1(
		css_mq_feature_op op,
		const css_mq_value *value,
		const css_media_length *client_len)
{
	if (value->type != CSS_MQ_VALUE_TYPE_DIM) {
		return false;
	}
	/* TODO: convert to same units */
	if (value->data.dim.unit != client_len->unit) {
		return false;
	}

	switch (op) {
	case CSS_MQ_FEATURE_OP_BOOL:
		return false;
	case CSS_MQ_FEATURE_OP_LT:
		return value->data.dim.len <  client_len->value;
	case CSS_MQ_FEATURE_OP_LTE:
		return value->data.dim.len <= client_len->value;
	case CSS_MQ_FEATURE_OP_EQ:
		return value->data.dim.len == client_len->value;
	case CSS_MQ_FEATURE_OP_GTE:
		return value->data.dim.len >= client_len->value;
	case CSS_MQ_FEATURE_OP_GT:
		return value->data.dim.len >  client_len->value;
	default:
		return false;
	}
}

static inline bool mq_match_feature_range_length_op2(
		css_mq_feature_op op,
		const css_mq_value *value,
		const css_media_length *client_len)
{
	if (op == CSS_MQ_FEATURE_OP_UNUSED) {
		return true;
	}
	if (value->type != CSS_MQ_VALUE_TYPE_DIM) {
		return false;
	}
	/* TODO: convert to same units */
	if (value->data.dim.unit != client_len->unit) {
		return false;
	}

	switch (op) {
	case CSS_MQ_FEATURE_OP_LT:
		return client_len->value <  value->data.dim.len;
	case CSS_MQ_FEATURE_OP_LTE:
		return client_len->value <= value->data.dim.len;
	case CSS_MQ_FEATURE_OP_EQ:
		return client_len->value == value->data.dim.len;
	case CSS_MQ_FEATURE_OP_GTE:
		return client_len->value >= value->data.dim.len;
	case CSS_MQ_FEATURE_OP_GT:
		return client_len->value >  value->data.dim.len;
	default:
		return false;
	}
}

/**
 * Match media query features.
 *
 * \param[in] feat  Condition to match.
 * \param[in] media  Current media spec, to check against feat.
 * \return true if condition matches, otherwise false.
 */
static inline bool mq_match_feature(
		const css_mq_feature *feat,
		const css_media *media)
{
	/* TODO: Use interned string for comparison. */
	if (strcmp(lwc_string_data(feat->name), "width") == 0) {
		if (!mq_match_feature_range_length_op1(
				feat->op, &feat->value, &media->width)) {
			return false;
		}
		return mq_match_feature_range_length_op2(
				feat->op2, &feat->value2, &media->width);

	} else if (strcmp(lwc_string_data(feat->name), "height") == 0) {
		if (!mq_match_feature_range_length_op1(
				feat->op, &feat->value, &media->height)) {
			return false;
		}

		return mq_match_feature_range_length_op2(
				feat->op2, &feat->value2, &media->height);
	}

	/* TODO: Look at other feature names. */

	return false;
}

/**
 * Match media query conditions.
 *
 * \param[in] cond   Condition to match.
 * \param[in] media  Current media spec, to check against cond.
 * \return true if condition matches, otherwise false.
 */
static inline bool mq_match_condition(
		const css_mq_cond *cond,
		const css_media *media)
{
	bool matched = !cond->op;

	for (uint32_t i = 0; i < cond->parts->nparts; i++) {
		bool part_matched;
		if (cond->parts->parts[i]->type == CSS_MQ_FEATURE) {
			part_matched = mq_match_feature(
					cond->parts->parts[i]->data.feat,
					media);
		} else {
			assert(cond->parts->parts[i]->type == CSS_MQ_COND);
			part_matched = mq_match_condition(
					cond->parts->parts[i]->data.cond,
					media);
		}

		if (cond->op) {
			/* OR */
			matched |= part_matched;
			if (matched) {
				break; /* Short-circuit */
			}
		} else {
			/* AND */
			matched &= part_matched;
			if (!matched) {
				break; /* Short-circuit */
			}
		}
	}

	return matched != cond->negate;
}

/**
 * Test whether media query list matches current media.
 *
 * If anything in the list matches, the list matches.  If none match
 * it doesn't match.
 *
 * \param[in] m      Media query list.
 * \param[in] media  Current media spec, to check against m.
 * \return true if media query list matches media
 */
static inline bool mq__list_match(
		const css_mq_query *m,
		const css_media *media)
{
	for (; m != NULL; m = m->next) {
		/* Check type */
		if (!!(m->type & media->type) != m->negate_type) {
			if (m->cond == NULL ||
					mq_match_condition(m->cond, media)) {
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
 * \param media  Current media spec
 * \return true iff chain's rule applies for media
 */
static inline bool mq_rule_good_for_media(const css_rule *rule, const css_media *media)
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
