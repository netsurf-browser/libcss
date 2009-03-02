/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include "select/dispatch.h"
#include "select/properties.h"

/**
 * Dispatch table for properties, indexed by opcode
 */
struct prop_table prop_dispatch[N_OPCODES] = {
	{ cascade_azimuth,               initial_azimuth,               compose_azimuth, 1, GROUP_AURAL},
	{ cascade_background_attachment, initial_background_attachment, compose_background_attachment, 0, GROUP_NORMAL },
	{ cascade_background_color,      initial_background_color,      compose_background_color, 0, GROUP_NORMAL },
	{ cascade_background_image,      initial_background_image,      compose_background_image, 0, GROUP_NORMAL },
	{ cascade_background_position,   initial_background_position,   compose_background_position, 0, GROUP_NORMAL },
	{ cascade_background_repeat,     initial_background_repeat,     compose_background_repeat, 0, GROUP_NORMAL },
	{ cascade_border_collapse,       initial_border_collapse,       compose_border_collapse, 1, GROUP_NORMAL },
	{ cascade_border_spacing,        initial_border_spacing,        compose_border_spacing, 1, GROUP_UNCOMMON },
	{ cascade_border_top_color,      initial_border_top_color,      compose_border_top_color, 0, GROUP_NORMAL },
	{ cascade_border_right_color,    initial_border_right_color,    compose_border_right_color, 0, GROUP_NORMAL },
	{ cascade_border_bottom_color,   initial_border_bottom_color,   compose_border_bottom_color, 0, GROUP_NORMAL },
	{ cascade_border_left_color,     initial_border_left_color,     compose_border_left_color, 0, GROUP_NORMAL },
	{ cascade_border_top_style,      initial_border_top_style,      compose_border_top_style, 0, GROUP_NORMAL },
	{ cascade_border_right_style,    initial_border_right_style,    compose_border_right_style, 0, GROUP_NORMAL },
	{ cascade_border_bottom_style,   initial_border_bottom_style,   compose_border_bottom_style, 0, GROUP_NORMAL },
	{ cascade_border_left_style,     initial_border_left_style,     compose_border_left_style, 0, GROUP_NORMAL },
	{ cascade_border_top_width,      initial_border_top_width,      compose_border_top_width, 0, GROUP_NORMAL },
	{ cascade_border_right_width,    initial_border_right_width,    compose_border_right_width, 0, GROUP_NORMAL },
	{ cascade_border_bottom_width,   initial_border_bottom_width,   compose_border_bottom_width, 0, GROUP_NORMAL },
	{ cascade_border_left_width,     initial_border_left_width,     compose_border_left_width, 0, GROUP_NORMAL },
	{ cascade_bottom,                initial_bottom,                compose_bottom, 0, GROUP_NORMAL },
	{ cascade_caption_side,          initial_caption_side,          compose_caption_side, 1, GROUP_NORMAL },
	{ cascade_clear,                 initial_clear,                 compose_clear, 0, GROUP_NORMAL },
	{ cascade_clip,                  initial_clip,                  compose_clip, 0, GROUP_UNCOMMON },
	{ cascade_color,                 initial_color,                 compose_color, 1, GROUP_NORMAL },
	{ cascade_content,               initial_content,               compose_content, 0, GROUP_UNCOMMON },
	{ cascade_counter_increment,     initial_counter_increment,     compose_counter_increment, 0, GROUP_UNCOMMON },
	{ cascade_counter_reset,         initial_counter_reset,         compose_counter_reset, 0, GROUP_UNCOMMON },
	{ cascade_cue_after,             initial_cue_after,             compose_cue_after, 0, GROUP_AURAL },
	{ cascade_cue_before,            initial_cue_before,            compose_cue_before, 0, GROUP_AURAL },
	{ cascade_cursor,                initial_cursor,                compose_cursor, 1, GROUP_UNCOMMON },
	{ cascade_direction,             initial_direction,             compose_direction, 1, GROUP_NORMAL },
	{ cascade_display,               initial_display,               compose_display, 0, GROUP_NORMAL },
	{ cascade_elevation,             initial_elevation,             compose_elevation, 1, GROUP_AURAL },
	{ cascade_empty_cells,           initial_empty_cells,           compose_empty_cells, 1, GROUP_NORMAL },
	{ cascade_float,                 initial_float,                 compose_float, 0, GROUP_NORMAL },
	{ cascade_font_family,           initial_font_family,           compose_font_family, 1, GROUP_NORMAL },
	{ cascade_font_size,             initial_font_size,             compose_font_size, 1, GROUP_NORMAL },
	{ cascade_font_style,            initial_font_style,            compose_font_style, 1, GROUP_NORMAL },
	{ cascade_font_variant,          initial_font_variant,          compose_font_variant, 1, GROUP_NORMAL },
	{ cascade_font_weight,           initial_font_weight,           compose_font_weight, 1, GROUP_NORMAL },
	{ cascade_height,                initial_height,                compose_height, 0, GROUP_NORMAL },
	{ cascade_left,                  initial_left,                  compose_left, 0, GROUP_NORMAL },
	{ cascade_letter_spacing,        initial_letter_spacing,        compose_letter_spacing, 1, GROUP_UNCOMMON },
	{ cascade_line_height,           initial_line_height,           compose_line_height, 1, GROUP_NORMAL },
	{ cascade_list_style_image,      initial_list_style_image,      compose_list_style_image, 1, GROUP_NORMAL },
	{ cascade_list_style_position,   initial_list_style_position,   compose_list_style_position, 1, GROUP_NORMAL },
	{ cascade_list_style_type,       initial_list_style_type,       compose_list_style_type, 1, GROUP_NORMAL },
	{ cascade_margin_top,            initial_margin_top,            compose_margin_top, 0, GROUP_NORMAL },
	{ cascade_margin_right,          initial_margin_right,          compose_margin_right, 0, GROUP_NORMAL },
	{ cascade_margin_bottom,         initial_margin_bottom,         compose_margin_bottom, 0, GROUP_NORMAL },
	{ cascade_margin_left,           initial_margin_left,           compose_margin_left, 0, GROUP_NORMAL },
	{ cascade_max_height,            initial_max_height,            compose_max_height, 0, GROUP_NORMAL },
	{ cascade_max_width,             initial_max_width,             compose_max_width, 0, GROUP_NORMAL },
	{ cascade_min_height,            initial_min_height,            compose_min_height, 0, GROUP_NORMAL },
	{ cascade_min_width,             initial_min_width,             compose_min_width, 0, GROUP_NORMAL },
	{ cascade_orphans,               initial_orphans,               compose_orphans, 1, GROUP_PAGE },
	{ cascade_outline_color,         initial_outline_color,         compose_outline_color, 0, GROUP_UNCOMMON },
	{ cascade_outline_style,         initial_outline_style,         compose_outline_style, 0, GROUP_NORMAL },
	{ cascade_outline_width,         initial_outline_width,         compose_outline_width, 0, GROUP_UNCOMMON },
	{ cascade_overflow,              initial_overflow,              compose_overflow, 0, GROUP_NORMAL },
	{ cascade_padding_top,           initial_padding_top,           compose_padding_top, 0, GROUP_NORMAL },
	{ cascade_padding_right,         initial_padding_right,         compose_padding_right, 0, GROUP_NORMAL },
	{ cascade_padding_bottom,        initial_padding_bottom,        compose_padding_bottom, 0, GROUP_NORMAL },
	{ cascade_padding_left,          initial_padding_left,          compose_padding_left, 0, GROUP_NORMAL },
	{ cascade_page_break_after,      initial_page_break_after,      compose_page_break_after, 0, GROUP_PAGE },
	{ cascade_page_break_before,     initial_page_break_before,     compose_page_break_before, 0, GROUP_PAGE },
	{ cascade_page_break_inside,     initial_page_break_inside,     compose_page_break_inside, 1, GROUP_PAGE },
	{ cascade_pause_after,           initial_pause_after,           compose_pause_after, 0, GROUP_AURAL },
	{ cascade_pause_before,          initial_pause_before,          compose_pause_before, 0, GROUP_AURAL },
	{ cascade_pitch_range,           initial_pitch_range,           compose_pitch_range, 1, GROUP_AURAL },
	{ cascade_pitch,                 initial_pitch,                 compose_pitch, 1, GROUP_AURAL },
	{ cascade_play_during,           initial_play_during,           compose_play_during, 0, GROUP_AURAL },
	{ cascade_position,              initial_position,              compose_position, 0, GROUP_NORMAL },
	{ cascade_quotes,                initial_quotes,                compose_quotes, 1, GROUP_UNCOMMON },
	{ cascade_richness,              initial_richness,              compose_richness, 1, GROUP_AURAL },
	{ cascade_right,                 initial_right,                 compose_right, 0, GROUP_NORMAL },
	{ cascade_speak_header,          initial_speak_header,          compose_speak_header, 1, GROUP_AURAL },
	{ cascade_speak_numeral,         initial_speak_numeral,         compose_speak_numeral, 1, GROUP_AURAL },
	{ cascade_speak_punctuation,     initial_speak_punctuation,     compose_speak_punctuation, 1, GROUP_AURAL },
	{ cascade_speak,                 initial_speak,                 compose_speak, 1, GROUP_AURAL },
	{ cascade_speech_rate,           initial_speech_rate,           compose_speech_rate, 1, GROUP_AURAL },
	{ cascade_stress,                initial_stress,                compose_stress, 1, GROUP_AURAL },
	{ cascade_table_layout,          initial_table_layout,          compose_table_layout, 0, GROUP_NORMAL },
	{ cascade_text_align,            initial_text_align,            compose_text_align, 1, GROUP_NORMAL },
	{ cascade_text_decoration,       initial_text_decoration,       compose_text_decoration, 0, GROUP_NORMAL },
	{ cascade_text_indent,           initial_text_indent,           compose_text_indent, 1, GROUP_NORMAL },
	{ cascade_text_transform,        initial_text_transform,        compose_text_transform, 1, GROUP_NORMAL },
	{ cascade_top,                   initial_top,                   compose_top, 0, GROUP_NORMAL },
	{ cascade_unicode_bidi,          initial_unicode_bidi,          compose_unicode_bidi, 0, GROUP_NORMAL },
	{ cascade_vertical_align,        initial_vertical_align,        compose_vertical_align, 0, GROUP_NORMAL },
	{ cascade_visibility,            initial_visibility,            compose_visibility, 1, GROUP_NORMAL },
	{ cascade_voice_family,          initial_voice_family,          compose_voice_family, 1, GROUP_AURAL },
	{ cascade_volume,                initial_volume,                compose_volume, 1, GROUP_AURAL },
	{ cascade_white_space,           initial_white_space,           compose_white_space, 1, GROUP_NORMAL },
	{ cascade_widows,                initial_widows,                compose_widows, 1, GROUP_PAGE },
	{ cascade_width,                 initial_width,                 compose_width, 0, GROUP_NORMAL },
	{ cascade_word_spacing,          initial_word_spacing,          compose_word_spacing, 1, GROUP_UNCOMMON },
	{ cascade_z_index,               initial_z_index,               compose_z_index, 0, GROUP_NORMAL }
};

