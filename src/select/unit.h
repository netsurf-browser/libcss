/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2021 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef css_select_unit_h_
#define css_select_unit_h_

/**
 * Client callback for font measuring.
 *
 * \param[in]  pw     Client data.
 * \param[in]  style  Style to measure font for, or NULL.
 * \param[in]  unit   Either CSS_UNIT_EX, or CSS_UNIT_CH.
 * \return length in CSS pixels.
 */
typedef css_fixed (*css_unit_len_measure)(
		void *pw,
		const css_computed_style *style,
		const css_unit unit);

/**
 * LibCSS unit conversion context.
 *
 * The client callback is optional.  It is used for measuring "ch"
 * (glyph '0' advance) and "ex" (height of the letter 'x') units.
 * If a NULL pointer is given, LibCSS will use a fixed scaling of
 * the "em" unit.
 */
typedef struct css_unit_len_ctx {
	/**
	 * Viewport width in CSS pixels.
	 * Used if unit is vh, vw, vi, vb, vmin, or vmax.
	 */
	css_fixed viewport_width;
	/**
	 * Viewport height in CSS pixels.
	 * Used if unit is vh, vw, vi, vb, vmin, or vmax.
	 */
	css_fixed viewport_height;
	/**
	 * Client default font size in CSS pixels.
	 */
	css_fixed font_size_default;
	/**
	 * Client minimum font size in CSS pixels.  May be zero.
	 */
	css_fixed font_size_minimum;
	/**
	 * DPI of the device the style is selected for.
	 */
	css_fixed device_dpi;
	/**
	 * Reference style.  Most of the time, this is the element's style.
	 * When converting a length for a typographical property, such as
	 * font-size, then this should be the parent node.  If the node has
	 * no parent then this may be NULL.
	 */
	const css_computed_style *ref_style;
	/**
	 * Computed style for the document root element.
	 * May be NULL if unit is not rem.
	 */
	const css_computed_style *root_style;
	/**
	 * Client private word for measure callback.
	 */
	void *pw;
	/**
	 * Client callback for font measuring.
	 */
	const css_unit_len_measure measure;
} css_unit_len_ctx;

/**
 * Convert css pixels to physical pixels.
 *
 * \param[in] css_pixels  Length in css pixels.
 * \param[in] device_dpi  Device dots per inch.
 * \return Length in device pixels.
 */
static inline css_fixed css_unit_css2device_px(
		const css_fixed css_pixels,
		const css_fixed device_dpi)
{
	return FDIV(FMUL(css_pixels, device_dpi), F_96);
}

/**
 * Convert device pixels to css pixels.
 *
 * \param[in] device_pixels  Length in physical pixels.
 * \param[in] device_dpi     Device dots per inch.
 * \return Length in css pixels.
 */
static inline css_fixed css_unit_device2css_px(
		const css_fixed device_pixels,
		const css_fixed device_dpi)
{
	return FDIV(FMUL(device_pixels, F_96), device_dpi);
}

/**
 * Convert a length to points (pt).
 *
 * \param[in]  ctx     Length unit conversion context.
 * \param[in]  length  Length to convert.
 * \param[in]  unit    Current unit of length.
 * \return A length in points.
 */
css_fixed css_unit_font_size_len2pt(
		const css_unit_len_ctx *ctx,
		const css_fixed length,
		const css_unit unit);

/**
 * Convert a length to CSS pixels.
 *
 * \param[in]  ctx     Length unit conversion context.
 * \param[in]  length  Length to convert.
 * \param[in]  unit    Current unit of length.
 * \return A length in CSS pixels.
 */
css_fixed css_unit_len2css_px(
		const css_unit_len_ctx *ctx,
		const css_fixed length,
		const css_unit unit);

/**
 * Convert a length to device pixels.
 *
 * \param[in]  ctx     Length unit conversion context.
 * \param[in]  length  Length to convert.
 * \param[in]  unit    Current unit of length.
 * \return A length in device pixels.
 */
css_fixed css_unit_len2device_px(
		const css_unit_len_ctx *ctx,
		const css_fixed length,
		const css_unit unit);

/**
 * Convert a length to CSS pixels for a media query context.
 *
 * \param[in]  media   Client media specification.
 * \param[in]  length  Length to convert.
 * \param[in]  unit    Current unit of length.
 * \return A length in CSS pixels.
 */
css_fixed css_unit_len2px_mq(
		const css_media *media,
		const css_fixed length,
		const css_unit unit);

/**
 * Convert relative font size units to absolute units.
 *
 * \param[in]     ctx   Length unit conversion context.
 * \param[in,out] size  The length to convert.
 * \return CSS_OK on success, or appropriate error otherwise.
 */
css_error css_unit_compute_absolute_font_size(
		const css_unit_len_ctx *ctx,
		css_hint *size);

#endif
