/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2017 The NetSurf Project
 */


struct css_computed_uncommon_i {
/*
 * Property                       Size (bits)     Size (bytes)
 * ---                            ---             ---
 * border_spacing                   1 + 10          8
 * break_after                      4             
 * break_before                     4             
 * break_inside                     4             
 * clip                             6 + 20         16
 * column_count                     2               4
 * column_fill                      2             
 * column_gap                       2 + 5           4
 * column_rule_color                2               4
 * column_rule_style                4             
 * column_rule_width                3 + 5           4
 * column_span                      2             
 * column_width                     2 + 5           4
 * letter_spacing                   2 + 5           4
 * outline_color                    2               4
 * outline_width                    3 + 5           4
 * word_spacing                     2 + 5           4
 * writing_mode                     2             
 * 
 * Encode content as an array of content items, terminated with a blank entry.
 * 
 * content                          2             sizeof(ptr)
 * 
 * Encode counter_increment as an array of name, value pairs, terminated with a
 * blank entry.
 * 
 * counter_increment                1             sizeof(ptr)
 * 
 * Encode counter_reset as an array of name, value pairs, terminated with a
 * blank entry.
 * 
 * counter_reset                    1             sizeof(ptr)
 * 
 * Encode cursor uri(s) as an array of string objects, terminated with a blank
 * entry
 * 
 * cursor                           5             sizeof(ptr)
 * 
 * ---                            ---             ---
 *                                118 bits         60 + 4sizeof(ptr) bytes
 *                                ===================
 *                                 75 + 4sizeof(ptr) bytes
 * 
 * Bit allocations:
 * 
 * 0  bbbbbbbbbbbccccccccoooooooouuuuu
 * border_spacing; column_rule_width; outline_width; cursor
 * 
 * 1  lllllllcccccccooooooowwwwwwwbbbb
 * letter_spacing; column_gap; column_width; word_spacing; break_after
 * 
 * 2  ccccccccccccccccccccccccccbbbboo
 * clip; break_before; column_count
 * 
 * 3  bbbbccccwwoolluunnmmte..........
 * break_inside; column_rule_style; writing_mode; column_span;
 * column_rule_color; outline_color; content; column_fill; counter_increment;
 * counter_reset
 */
	uint32_t bits[4];
	
	css_fixed border_spacing_a;
	css_fixed border_spacing_b;
	css_fixed clip_a;
	css_fixed clip_b;
	css_fixed clip_c;
	css_fixed clip_d;
	int32_t column_count;
	css_fixed column_gap;
	css_color column_rule_color;
	css_fixed column_rule_width;
	css_fixed column_width;
	css_fixed letter_spacing;
	css_color outline_color;
	css_fixed outline_width;
	css_fixed word_spacing;
};

typedef struct css_computed_uncommon {
	struct css_computed_uncommon_i i;
	
	css_computed_content_item *content;
	css_computed_counter *counter_increment;
	css_computed_counter *counter_reset;
	lwc_string **cursor;
	
	struct css_computed_uncommon *next;
	uint32_t count;
	uint32_t bin;
} css_computed_uncommon;

typedef struct css_computed_page {
/*
 * Property                       Size (bits)     Size (bytes)
 * ---                            ---             ---
 * orphans                          1               4
 * page_break_after                 3             
 * page_break_before                3             
 * page_break_inside                2             
 * widows                           1               4
 * 
 * 
 * ---                            ---             ---
 *                                 10 bits          8 bytes
 *                                ===================
 *                                 10 bytes
 * 
 * Bit allocations:
 * 
 * 0  pppaaaggwo......................
 * page_break_before; page_break_after; page_break_inside; widows; orphans
 */
	uint32_t bits[1];
	
	int32_t orphans;
	int32_t widows;
} css_computed_page;

struct css_computed_style_i {
/*
 * Property                       Size (bits)     Size (bytes)
 * ---                            ---             ---
 * align_content                    3             
 * align_items                      3             
 * align_self                       3             
 * background_attachment            2             
 * background_color                 2               4
 * background_image                 1             sizeof(ptr)
 * background_position              1 + 10          8
 * background_repeat                3             
 * border_bottom_color              2               4
 * border_bottom_style              4             
 * border_bottom_width              3 + 5           4
 * border_collapse                  2             
 * border_left_color                2               4
 * border_left_style                4             
 * border_left_width                3 + 5           4
 * border_right_color               2               4
 * border_right_style               4             
 * border_right_width               3 + 5           4
 * border_top_color                 2               4
 * border_top_style                 4             
 * border_top_width                 3 + 5           4
 * bottom                           2 + 5           4
 * box_sizing                       2             
 * caption_side                     2             
 * clear                            3             
 * color                            1               4
 * direction                        2             
 * display                          5             
 * empty_cells                      2             
 * flex_basis                       2 + 5           4
 * flex_direction                   3             
 * flex_grow                        1               4
 * flex_shrink                      1               4
 * flex_wrap                        2             
 * float                            2             
 * font_size                        4 + 5           4
 * font_style                       2             
 * font_variant                     2             
 * font_weight                      4             
 * height                           2 + 5           4
 * justify_content                  3             
 * left                             2 + 5           4
 * line_height                      2 + 5           4
 * list_style_image                 1             sizeof(ptr)
 * list_style_position              2             
 * list_style_type                  4             
 * margin_bottom                    2 + 5           4
 * margin_left                      2 + 5           4
 * margin_right                     2 + 5           4
 * margin_top                       2 + 5           4
 * max_height                       2 + 5           4
 * max_width                        2 + 5           4
 * min_height                       2 + 5           4
 * min_width                        2 + 5           4
 * opacity                          1               4
 * order                            1               4
 * outline_style                    4             
 * overflow_x                       3             
 * overflow_y                       3             
 * padding_bottom                   1 + 5           4
 * padding_left                     1 + 5           4
 * padding_right                    1 + 5           4
 * padding_top                      1 + 5           4
 * position                         3             
 * right                            2 + 5           4
 * table_layout                     2             
 * text_align                       4             
 * text_decoration                  5             
 * text_indent                      1 + 5           4
 * text_transform                   3             
 * top                              2 + 5           4
 * unicode_bidi                     2             
 * vertical_align                   4 + 5           4
 * visibility                       2             
 * white_space                      3             
 * width                            2 + 5           4
 * z_index                          2               4
 * 
 * Encode font family as an array of string objects, terminated with a blank
 * entry.
 * 
 * font_family                      3             sizeof(ptr)
 * 
 * Encode quotes as an array of string objects, terminated with a blank entry.
 * 
 * quotes                           1             sizeof(ptr)
 * 
 * ---                            ---             ---
 *                                332 bits        160 + 4sizeof(ptr) bytes
 *                                ===================
 *                                202 + 4sizeof(ptr) bytes
 * 
 * Bit allocations:
 * 
 * 0  bbbbbbbboooooooorrrrrrrrdddddddd
 * border_bottom_width; border_left_width; border_top_width; border_right_width
 * 
 * 1  mmmmmmmaaaaaaawwwwwwwfffffffbbbb
 * margin_right; max_width; width; flex_basis; border_top_style
 * 
 * 2  mmmmmmmiiiiiiilllllllhhhhhhhbbbb
 * margin_bottom; min_width; line_height; height; border_right_style
 * 
 * 3  lllllllbbbbbbbmmmmmmmiiiiiiissss
 * left; bottom; max_height; min_height; list_style_type
 * 
 * 4  rrrrrrrmmmmmmmtttttttaaaaaaaffff
 * right; margin_top; top; margin_left; font_weight
 * 
 * 5  bbbbbbbbbbbvvvvvvvvvfffffffffwww
 * background_position; vertical_align; font_size; white_space
 * 
 * 6  dddddtttttbbbbooooeeeeuuuuaaafff
 * display; text_decoration; border_bottom_style; border_left_style;
 * text_align; outline_style; background_repeat; flex_direction
 * 
 * 7  ppppppaaaaaaddddddttttttiiiiiibb
 * padding_top; padding_left; padding_right; text_indent; padding_bottom;
 * background_color
 * 
 * 8  tttfffoooaaalllpppccciiijjjvvvdd
 * text_transform; font_family; overflow_y; align_self; align_items; position;
 * clear; align_content; justify_content; overflow_x; direction
 * 
 * 9  ttbbaazzffllvvooFFeeccrrnnuuxxii
 * table_layout; border_right_color; background_attachment; z_index;
 * font_style; flex_wrap; visibility; border_top_color; float; empty_cells;
 * caption_side; border_bottom_color; font_variant; unicode_bidi; box_sizing;
 * list_style_position
 * 
 * 10 bbooaclqfrpe....................
 * border_left_color; border_collapse; background_image; color;
 * list_style_image; quotes; flex_grow; order; opacity; flex_shrink
 */
	uint32_t bits[11];
	
	css_color background_color;
	lwc_string *background_image;
	css_fixed background_position_a;
	css_fixed background_position_b;
	css_color border_bottom_color;
	css_fixed border_bottom_width;
	css_color border_left_color;
	css_fixed border_left_width;
	css_color border_right_color;
	css_fixed border_right_width;
	css_color border_top_color;
	css_fixed border_top_width;
	css_fixed bottom;
	css_color color;
	css_fixed flex_basis;
	css_fixed flex_grow;
	css_fixed flex_shrink;
	css_fixed font_size;
	css_fixed height;
	css_fixed left;
	css_fixed line_height;
	lwc_string *list_style_image;
	css_fixed margin_bottom;
	css_fixed margin_left;
	css_fixed margin_right;
	css_fixed margin_top;
	css_fixed max_height;
	css_fixed max_width;
	css_fixed min_height;
	css_fixed min_width;
	css_fixed opacity;
	int32_t order;
	css_fixed padding_bottom;
	css_fixed padding_left;
	css_fixed padding_right;
	css_fixed padding_top;
	css_fixed right;
	css_fixed text_indent;
	css_fixed top;
	css_fixed vertical_align;
	css_fixed width;
	int32_t z_index;
	
	css_computed_uncommon *uncommon;
	void *aural;
};

struct css_computed_style {
	struct css_computed_style_i i;
	
	lwc_string **font_family;
	lwc_string **quotes;
	
	css_computed_page *page;
	struct css_computed_style *next;
	uint32_t count;
	uint32_t bin;
};
