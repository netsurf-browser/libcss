/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2021 Vincent Sanders <vince@netsurf-browser.org>
 */

#include "select/propget.h"
#include "utils/utils.h"

#define SYMBOL_SIZE 5
typedef char symbol_t[SYMBOL_SIZE];

struct list_counter_style {
	const char *name; /**< style name for debug purposes */
	struct {
		const int start; /**< first acceptable value for this style */
		const int end; /**< last acceptable value for this style */
	} range;
	struct {
		const unsigned int length;
		const symbol_t value;
	} pad;
	const char *prefix;
	const char *postfix;
	const symbol_t *symbols; /**< array of symbols which represent this style */
	const int *weights; /**< symbol weights for additive schemes */
	const size_t items; /**< items in symbol and weight table */
	size_t (*calc)(uint8_t *ares, const size_t alen, int value, const struct list_counter_style *cstyle); /**< function to calculate the system */
};

/**
 * Copy a null-terminated UTF-8 string to buffer at offset, if there is space
 *
 * \param[in] buf    The output buffer
 * \param[in] buflen The length of \a buf
 * \param[in] pos    Current position in \a buf
 * \param[in] str    The string to copy into \a buf
 * \return The number of bytes needed in the output buffer which may be
 *         larger than \a buflen but the buffer will not be overrun
 */
static inline size_t
copy_string(char *buf, const size_t buflen, size_t pos, const char *str)
{
	size_t sidx = 0; /* current string index */

	while (str[sidx] != '\0') {
		if (pos < buflen) {
			buf[pos] = str[sidx];
		}
		pos++;
		sidx++;
	}

	return sidx;
}

/**
 * Copy a UTF-8 symbol to buffer at offset, if there is space
 *
 * \param[in] buf    The output buffer
 * \param[in] buflen The length of \a buf
 * \param[in] pos    Current position in \a buf
 * \param[in] symbol The symbol to copy into \a buf
 * \return The number of bytes needed in the output buffer which may be
 *         larger than \a buflen but the buffer will not be overrun
 */
static inline size_t
copy_symbol(char *buf, const size_t buflen, size_t pos, const symbol_t symbol)
{
	size_t sidx = 0; /* current symbol index */

	while ((sidx < sizeof(symbol_t)) && (symbol[sidx] != '\0')) {
		if (pos < buflen) {
			buf[pos] = symbol[sidx];
		}
		pos++;
		sidx++;
	}

	return sidx;
}

/**
 * maps alphabet values to output values with a symbol table
 *
 * Takes a list of alphabet values and for each one outputs the
 *   compete symbol (in utf8) to an output buffer.
 *
 * \param buf The oputput buffer
 * \param buflen the length of \a buf
 * \param aval array of alphabet values
 * \param alen The number of values in \a alen
 * \param symtab The symbol table
 * \param symtablen The number of symbols in \a symtab
 * \return The number of bytes needed in the output buffer whichmay be
 *         larger than \a buflen but the buffer will not be overrun
 */
static size_t
map_aval_to_symbols(char *buf, const size_t buflen,
		    const uint8_t *aval, const size_t alen,
		    const struct list_counter_style *cstyle)
{
	size_t oidx = 0;
	size_t aidx; /* numeral index */
	const symbol_t postfix = "."; /* default postfix string */

	/* add padding if required */
	if (alen < cstyle->pad.length) {
		size_t pidx; /* padding index */
		for (pidx=cstyle->pad.length - alen; pidx > 0; pidx--) {
			oidx += copy_symbol(buf, buflen, oidx,
					cstyle->pad.value);
		}
	}

	/* map symbols */
	for (aidx=0; aidx < alen; aidx++) {
		oidx += copy_symbol(buf, buflen, oidx,
				cstyle->symbols[aval[aidx]]);
	}

	/* postfix */
	oidx += copy_string(buf, buflen, oidx,
			(cstyle->postfix != NULL) ?
					cstyle->postfix : postfix);

	return oidx;
}


/**
 * generate numeric symbol values
 *
 * fills array with numeric values that represent the input value
 *
 * \param ares Buffer to recive the converted values
 * \param alen the length of \a ares buffer
 * \param value The value to convert
 * \param cstyle The counter style in use
 * \return The length a complete conversion which may be larger than \a alen
 */
static size_t
calc_numeric_system(uint8_t *ares,
		    const size_t alen,
		    int value,
		    const struct list_counter_style *cstyle)
{
	size_t idx = 0;
	uint8_t *first;
	uint8_t *last;

	/* generate alphabet values in ascending order */
	while (value > 0) {
		if (idx < alen) {
			ares[idx] = value % cstyle->items;
		}
		idx++;
		value = value / cstyle->items;
	}

	/* put the values in decending order */
	first = ares;
	if (idx < alen) {
		last = first + (idx - 1);
	} else {
		last = first + (alen - 1);
	}
	while (first < last) {
		*first ^= *last;
		*last ^= *first;
		*first ^= *last;
		first++;
		last--;
	}

	return idx;
}


/**
 * generate cyclic symbol values
 *
 * fills array with cyclic values that represent the input value
 *
 * \param ares Buffer to recive the converted values
 * \param alen the length of \a ares buffer
 * \param value The value to convert
 * \param cstyle The counter style in use
 * \return The length a complete conversion which may be larger than \a alen
 */
static size_t
calc_cyclic_system(uint8_t *ares,
		    const size_t alen,
		    int value,
		    const struct list_counter_style *cstyle)
{
	if (alen == 0) {
		return 0;
	}
	if (cstyle->items == 1) {
		/* there is only one symbol so select it */
		ares[0] = 0;
	} else {
		ares[0] = (value - 1) % cstyle->items;
	}
	return 1;
}


/**
 * generate addative symbol values
 *
 * fills array with numeric values that represent the input value
 *
 * \param ares Buffer to recive the converted values
 * \param alen the length of \a ares buffer
 * \param value The value to convert
 * \param wlen The number of weights
 * \return The length a complete conversion which may be larger than \a alen
 */
static size_t
calc_additive_system(uint8_t *ares,
		     const size_t alen,
		     int value,
		     const struct list_counter_style *cstyle)
{
	size_t widx; /* weight index */
	size_t aidx = 0;
	size_t idx;
	size_t times; /* number of times a weight occours */

	/* iterate over the available weights */
	for (widx = 0; widx < cstyle->items;widx++) {
		times = value / cstyle->weights[widx];
		if (times > 0) {
			for (idx=0;idx < times;idx++) {
				if (aidx < alen) {
					ares[aidx] = widx;
				}
				aidx++;
			}

			value -= times * cstyle->weights[widx];
		}
	}

	return aidx;
}


/**
 * generate alphabet symbol values for latin and greek labelling
 *
 * fills array with alphabet values suitable for the input value
 *
 * \param ares Buffer to recive the converted values
 * \param alen the length of \a ares buffer
 * \param value The value to convert
 * \param slen The number of symbols in the alphabet
 * \return The length a complete conversion which may be larger than \a alen
 */
static size_t
calc_alphabet_system(uint8_t *ares,
		     const size_t alen,
		     int value,
		     const struct list_counter_style *cstyle)
{
	size_t idx = 0;
	uint8_t *first;
	uint8_t *last;

	/* generate alphabet values in ascending order */
	while (value > 0) {
		--value;
		if (idx < alen) {
			ares[idx] = value % cstyle->items;
		}
		idx++;
		value = value / cstyle->items;
	}

	/* put the values in decending order */
	first = ares;
	if (idx < alen) {
		last = first + (idx - 1);
	} else {
		last = first + (alen - 1);
	}
	while (first < last) {
		*first ^= *last;
		*last ^= *first;
		*first ^= *last;
		first++;
		last--;
	}

	return idx;
}


/**
 * Roman numeral conversion
 *
 * \return The number of numerals that are nesesary for full output
 */
static size_t
calc_roman_system(uint8_t *buf,
		  const size_t maxlen,
		  int value,
		  const struct list_counter_style *cstyle)
{
	const int S[]  = {    0,   2,   4,   2,   4,   2,   4 };
	size_t k = 0; /* index into output buffer */
	unsigned int i = 0; /* index into maps */
	int r;
	int r2 = 0;
	size_t L;

	assert(cstyle->items == 7);

	/* ensure value is within acceptable range of this system */
	if ((value < cstyle->range.start) || (value > cstyle->range.end)) {
		return 0;
	}

	L = cstyle->items - 1;

	while (value > 0) {
		if (cstyle->weights[i] <= value) {
			r = value / cstyle->weights[i];
			value = value - (r * cstyle->weights[i]);
			if (i < L) {
				/* lookahead */
				r2 = value / cstyle->weights[i+1];
			}
			if (i < L && r2 >= S[i+1]) {
				/* will violate repeat boundary on next pass */
				value = value - (r2 * cstyle->weights[i+1]);
				if (k < maxlen) buf[k++] = i+1;
				if (k < maxlen) buf[k++] = i-1;
			} else if (S[i] && r >= S[i]) {
				/* violated repeat boundary on this pass */
				if (k < maxlen) buf[k++] = i;
				if (k < maxlen) buf[k++] = i-1;
			} else {
				while (r-- > 0 && k < maxlen) {
					buf[k++] = i;
				}
			}
		}
		i++;
	}

	return k;
}


/* tables for all the counter styles */


static const symbol_t georgian_symbols[] = {
	                                        "ჵ",
	"ჰ", "ჯ", "ჴ", "ხ", "ჭ", "წ", "ძ", "ც", "ჩ",
	"შ", "ყ", "ღ", "ქ", "ფ", "ჳ", "ტ", "ს", "რ",
	"ჟ", "პ", "ო", "ჲ", "ნ", "მ", "ლ", "კ", "ი",
	"თ", "ჱ", "ზ", "ვ", "ე", "დ", "გ", "ბ", "ა",
};
static const int georgian_weights[] = {
	                                                10000,
	9000, 8000, 7000, 6000, 5000, 4000, 3000, 2000, 1000,
	900,  800,  700,  600,  500,  400,  300,  200,  100,
	90,   80,   70,   60,   50,   40,   30,   20,   10,
	9,    8,    7,    6,    5,    4,    3,    2,    1
};
static const struct list_counter_style lcs_georgian =	{
	.name="georgian",
	.range = {
		.start = 1,
		.end = 19999,},
	.symbols = georgian_symbols,
	.weights = georgian_weights,
	.items = (sizeof(georgian_symbols) / SYMBOL_SIZE),
	.calc = calc_additive_system,
};


static const symbol_t upper_armenian_symbols[] = {
	"Ք", "Փ", "Ւ", "Ց", "Ր", "Տ", "Վ", "Ս", "Ռ",
	"Ջ", "Պ", "Չ", "Ո", "Շ", "Ն", "Յ", "Մ", "Ճ",
	"Ղ", "Ձ", "Հ", "Կ", "Ծ", "Խ", "Լ", "Ի", "Ժ",
	"Թ", "Ը", "Է", "Զ", "Ե", "Դ", "Գ", "Բ", "Ա"
};
static const int armenian_weights[] = {
	9000, 8000, 7000, 6000, 5000, 4000, 3000, 2000, 1000,
	900,  800,  700,  600,  500,  400,  300,  200,  100,
	90,   80,   70,   60,   50,   40,   30,   20,   10,
	9,    8,    7,    6,    5,    4,    3,    2,    1
};
static const struct list_counter_style lcs_upper_armenian = {
	.name = "upper-armenian",
	.range = {
		.start = 1,
		.end = 9999,},
	.symbols = upper_armenian_symbols,
	.weights = armenian_weights,
	.items = (sizeof(upper_armenian_symbols) / SYMBOL_SIZE),
	.calc = calc_additive_system,
};


static const symbol_t lower_armenian_symbols[] = {
	"ք", "փ", "ւ", "ց", "ր", "տ", "վ", "ս", "ռ",
	"ջ", "պ", "չ", "ո", "շ", "ն", "յ", "մ", "ճ",
	"ղ", "ձ", "հ", "կ", "ծ", "խ", "լ", "ի", "ժ",
	"թ", "ը", "է", "զ", "ե", "դ", "գ", "բ", "ա"
};
static const struct list_counter_style lcs_lower_armenian = {
	.name = "lower-armenian",
	.range = {
		.start = 1,
		.end = 9999,},
	.symbols = lower_armenian_symbols,
	.weights = armenian_weights,
	.items = (sizeof(lower_armenian_symbols) / SYMBOL_SIZE),
	.calc = calc_additive_system,
};


static const symbol_t decimal_symbols[] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
};
static const struct list_counter_style lcs_decimal = {
	.name = "decimal",
	.symbols = decimal_symbols,
	.items = (sizeof(decimal_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};


static const struct list_counter_style lcs_decimal_leading_zero = {
	.name = "decimal-leading-zero",
	.pad = {
		.length = 2,
		.value = "0",},
	.symbols = decimal_symbols,
	.items = (sizeof(decimal_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};


static const symbol_t lower_greek_symbols[] = {
	"α", "β", "γ", "δ", "ε", "ζ", "η", "θ", "ι", "κ",
	"λ", "μ", "ν", "ξ", "ο", "π", "ρ", "σ", "τ", "υ",
	"φ", "χ", "ψ", "ω"
};
static const struct list_counter_style lcs_lower_greek = {
	.name = "lower-greek",
	.symbols = lower_greek_symbols,
	.items = (sizeof(lower_greek_symbols) / SYMBOL_SIZE),
	.calc = calc_alphabet_system,
};


static const symbol_t upper_alpha_symbols[] = {
	"A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
	"K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
	"U", "V", "W", "X", "Y", "Z"
};
static const struct list_counter_style lcs_upper_alpha = {
	.name = "upper-alpha",
	.symbols = upper_alpha_symbols,
	.items = (sizeof(upper_alpha_symbols) / SYMBOL_SIZE),
	.calc = calc_alphabet_system,
};


static const symbol_t lower_alpha_symbols[] = {
	"a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
	"k", "l", "m", "n", "o", "p", "q", "r", "s", "t",
	"u", "v", "w", "x", "y", "z"
};
static struct list_counter_style lcs_lower_alpha = {
	.name = "lower-alpha",
	.symbols = lower_alpha_symbols,
	.items = (sizeof(lower_alpha_symbols) / SYMBOL_SIZE),
	.calc = calc_alphabet_system,
};


static const int roman_weights[] = {
	1000, 500, 100,  50,  10,   5,   1
};
static const symbol_t upper_roman_symbols[] = {
	"M", "D", "C", "L", "X", "V", "I"
};
static const struct list_counter_style lcs_upper_roman = {
	.name = "upper-roman",
	.range = {
		.start = 1,
		.end = 3999,},
	.symbols = upper_roman_symbols,
	.weights = roman_weights,
	.items = (sizeof(upper_roman_symbols) / SYMBOL_SIZE),
	.calc = calc_roman_system,
};


static const symbol_t lower_roman_symbols[] = {
	"m", "d", "c", "l", "x", "v", "i"
};
static const struct list_counter_style lcs_lower_roman = {
	.name = "lower-roman",
	.range = {
		.start = 1,
		.end = 3999,},
	.symbols = lower_roman_symbols,
	.weights = roman_weights,
	.items = (sizeof(lower_roman_symbols) / SYMBOL_SIZE),
	.calc = calc_roman_system,
};

static const symbol_t disc_symbols[] = { "\xE2\x80\xA2"}; /* 2022 BULLET */
static const struct list_counter_style lcs_disc = {
	.name = "disc",
	.symbols = disc_symbols,
	.items = (sizeof(disc_symbols) / SYMBOL_SIZE),
	.postfix = " ",
	.calc = calc_cyclic_system,
};

static const symbol_t circle_symbols[] = { "\342\227\213"}; /* 25CB WHITE CIRCLE */
static const struct list_counter_style lcs_circle = {
	.name = "circle",
	.symbols = circle_symbols,
	.items = (sizeof(circle_symbols) / SYMBOL_SIZE),
	.postfix = " ",
	.calc = calc_cyclic_system,
};

static const symbol_t square_symbols[] = { "\342\226\252"}; /* 25AA BLACK SMALL SQUARE */
static const struct list_counter_style lcs_square = {
	.name = "square",
	.symbols = square_symbols,
	.items = (sizeof(square_symbols) / SYMBOL_SIZE),
	.postfix = " ",
	.calc = calc_cyclic_system,
};

static const symbol_t binary_symbols[] = { "0", "1" };
static const struct list_counter_style lcs_binary = {
	.name = "binary",
	.symbols = binary_symbols,
	.items = (sizeof(binary_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t octal_symbols[] = {
	"0", "1", "2", "3", "4", "5", "6", "7"
};
static const struct list_counter_style lcs_octal = {
	.name = "octal",
	.symbols = octal_symbols,
	.items = (sizeof(octal_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};


static const symbol_t lower_hexadecimal_symbols[] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"a", "b", "c", "d", "e", "f"
};
static const struct list_counter_style lcs_lower_hexadecimal = {
	.name = "lower-hexadecimal",
	.symbols = lower_hexadecimal_symbols,
	.items = (sizeof(lower_hexadecimal_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t upper_hexadecimal_symbols[] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"a", "b", "c", "d", "e", "f"
};
static const struct list_counter_style lcs_upper_hexadecimal = {
	.name = "upper-hexadecimal",
	.symbols = upper_hexadecimal_symbols,
	.items = (sizeof(upper_hexadecimal_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t arabic_indic_symbols[] = {
	"\xd9\xa0", "\xd9\xa1", "\xd9\xa2", "\xd9\xa3", "\xd9\xa4", "\xd9\xa5", "\xd9\xa6", "\xd9\xa7", "\xd9\xa8", "\xd9\xa9"
};
static const struct list_counter_style lcs_arabic_indic = {
	.name = "arabic-indic",
	.symbols = arabic_indic_symbols,
	.items = (sizeof(arabic_indic_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t bengali_symbols[] = {
	"০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯"
};
static const struct list_counter_style lcs_bengali = {
	.name = "bengali",
	.symbols = bengali_symbols,
	.items = (sizeof(bengali_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t cambodian_symbols[] = {
	"០", "១", "២", "៣", "៤", "៥", "៦", "៧", "៨", "៩"
};
static const struct list_counter_style lcs_cambodian = {
	.name = "cambodian",
	.symbols = cambodian_symbols,
	.items = (sizeof(cambodian_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t cjk_decimal_symbols[] = {
	"〇", "一", "二", "三", "四", "五", "六", "七", "八", "九"
};
static const struct list_counter_style lcs_cjk_decimal = {
	.name = "cjk-decimal",
	.symbols = cjk_decimal_symbols,
	.postfix = "、",
	.items = (sizeof(cjk_decimal_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t devanagari_symbols[] = {
	"०", "१", "२", "३", "४", "५", "६", "७", "८", "९"
};
static const struct list_counter_style lcs_devanagari = {
	.name = "devanagari",
	.symbols = devanagari_symbols,
	.items = (sizeof(devanagari_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t gujarati_symbols[] = {
	"૦", "૧", "૨", "૩", "૪", "૫", "૬", "૭", "૮", "૯"
};
static const struct list_counter_style lcs_gujarati = {
	.name = "gujarati",
	.symbols = gujarati_symbols,
	.items = (sizeof(gujarati_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t gurmukhi_symbols[] = {
	"੦", "੧", "੨", "੩", "੪", "੫", "੬", "੭", "੮", "੯"
};
static const struct list_counter_style lcs_gurmukhi = {
	.name = "gurmukhi",
	.symbols = gurmukhi_symbols,
	.items = (sizeof(gurmukhi_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const int hebrew_weights[] = {
				                                          10000,
	9000, 8000, 7000, 6000, 5000, 4000, 3000, 2000,                   1000,
	                              400,  300,  200,                    100,
	90,   80,   70,   60,   50,   40,   30,   20, 19, 18, 17, 16, 15, 10,
	9,    8,    7,    6,    5,    4,    3,    2,                      1
};
static const symbol_t hebrew_symbols[] = {
	"\xD7\x99\xD7\xB3",
	"\xD7\x98\xD7\xB3", "\xD7\x97\xD7\xB3", "\xD7\x96\xD7\xB3", "\xD7\x95\xD7\xB3", "\xD7\x94\xD7\xB3", "\xD7\x93\xD7\xB3", "\xD7\x92\xD7\xB3", "\xD7\x91\xD7\xB3", "\xD7\x90\xD7\xB3",
	"\xD7\xAA", "\xD7\xA9", "\xD7\xA8", "\xD7\xA7",
	"\xD7\xA6", "\xD7\xA4", "\xD7\xA2", "\xD7\xA1", "\xD7\xA0", "\xD7\x9E", "\xD7\x9C", /* 20 */"\xD7\x9B", "\xD7\x99\xD7\x98", "\xD7\x99\xD7\x97", "\xD7\x99\xD7\x96", "\xD7\x98\xD7\x96", "\xD7\x98\xD7\x95", "\xD7\x99",
	"\xD7\x98", "\xD7\x97", "\xD7\x96", "\xD7\x95", "\xD7\x94", "\xD7\x93", "\xD7\x92", "\xD7\x91", "\xD7\x90"
};
static const struct list_counter_style lcs_hebrew = {
	.name = "hebrew",
	.range = {
		.start = 1,
		.end = 10999,},
	.symbols = hebrew_symbols,
	.weights = hebrew_weights,	
	.items = (sizeof(hebrew_symbols) / SYMBOL_SIZE),
	.calc = calc_additive_system,
};

static const symbol_t kannada_symbols[] = {
	"\xE0\xB3\xA6", "\xE0\xB3\xA7", "\xE0\xB3\xA8", "\xE0\xB3\xA9", "\xE0\xB3\xAA", "\xE0\xB3\xAB", "\xE0\xB3\xAC", "\xE0\xB3\xAD", "\xE0\xB3\xAE", "\xE0\xB3\xAF"
};
static const struct list_counter_style lcs_kannada = {
	.name = "kannada",
	.symbols = kannada_symbols,
	.items = (sizeof(kannada_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t lao_symbols[] = {
	"໐", "໑", "໒", "໓", "໔", "໕", "໖", "໗", "໘", "໙"
};
static const struct list_counter_style lcs_lao = {
	.name = "lao",
	.symbols = lao_symbols,
	.items = (sizeof(lao_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t malayalam_symbols[] = {
	"൦", "൧", "൨", "൩", "൪", "൫", "൬", "൭", "൮", "൯"
};
static const struct list_counter_style lcs_malayalam = {
	.name = "malayalam",
	.symbols = malayalam_symbols,
	.items = (sizeof(malayalam_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t mongolian_symbols[] = {
	"᠐", "᠑", "᠒", "᠓", "᠔", "᠕", "᠖", "᠗", "᠘", "᠙"
};
static const struct list_counter_style lcs_mongolian = {
	.name = "mongolian",
	.symbols = mongolian_symbols,
	.items = (sizeof(mongolian_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t myanmar_symbols[] = {
	"၀", "၁", "၂", "၃", "၄", "၅", "၆", "၇", "၈", "၉"
};
static const struct list_counter_style lcs_myanmar = {
	.name = "myanmar",
	.symbols = myanmar_symbols,
	.items = (sizeof(myanmar_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t oriya_symbols[] = {
	"୦", "୧", "୨", "୩", "୪", "୫", "୬", "୭", "୮", "୯"
};
static const struct list_counter_style lcs_oriya = {
	.name = "oriya",
	.symbols = oriya_symbols,
	.items = (sizeof(oriya_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t persian_symbols[] = {
	"۰", "۱", "۲", "۳", "۴", "۵", "۶", "۷", "۸", "۹"
};
static const struct list_counter_style lcs_persian = {
	.name = "persian",
	.symbols = persian_symbols,
	.items = (sizeof(persian_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t tamil_symbols[] = {
	"௦", "௧", "௨", "௩", "௪", "௫", "௬", "௭", "௮", "௯"
};
static const struct list_counter_style lcs_tamil = {
	.name = "tamil",
	.symbols = tamil_symbols,
	.items = (sizeof(tamil_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t telugu_symbols[] = {
	"౦", "౧", "౨", "౩", "౪", "౫", "౬", "౭", "౮", "౯"
};
static const struct list_counter_style lcs_telugu = {
	.name = "telugu",
	.symbols = telugu_symbols,
	.items = (sizeof(telugu_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t thai_symbols[] = {
	"๐", "๑", "๒", "๓", "๔", "๕", "๖", "๗", "๘", "๙"
};
static const struct list_counter_style lcs_thai = {
	.name = "thai",
	.symbols = thai_symbols,
	.items = (sizeof(thai_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t tibetan_symbols[] = {
	"༠", "༡", "༢", "༣", "༤", "༥", "༦", "༧", "༨", "༩"
};
static const struct list_counter_style lcs_tibetan = {
	.name = "tibetan",
	.symbols = tibetan_symbols,
	.items = (sizeof(tibetan_symbols) / SYMBOL_SIZE),
	.calc = calc_numeric_system,
};

static const symbol_t cjk_earthly_branch_symbols[] = {
	"子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌", "亥"
};
static struct list_counter_style lcs_cjk_earthly_branch = {
	.name = "cjk-earthly-branch",
	.symbols = cjk_earthly_branch_symbols,
	.items = (sizeof(cjk_earthly_branch_symbols) / SYMBOL_SIZE),
	.postfix = "、",
	.calc = calc_alphabet_system,
};

static const symbol_t cjk_heavenly_stem_symbols[] = {
	"甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬", "癸"
};
static struct list_counter_style lcs_cjk_heavenly_stem = {
	.name = "cjk-heavenly-stem",
	.symbols = cjk_heavenly_stem_symbols,
	.items = (sizeof(cjk_heavenly_stem_symbols) / SYMBOL_SIZE),
	.postfix = "、",
	.calc = calc_alphabet_system,
};

static const symbol_t hiragana_symbols[] = {
	"あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ", "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と", "な", "に", "ぬ", "ね", "の", "は", "ひ", "ふ", "へ", "ほ", "ま", "み", "む", "め", "も", "や", "ゆ", "よ", "ら", "り", "る", "れ", "ろ", "わ", "ゐ", "ゑ", "を", "ん"
};
static struct list_counter_style lcs_hiragana = {
	.name = "hiragana",
	.symbols = hiragana_symbols,
	.items = (sizeof(hiragana_symbols) / SYMBOL_SIZE),
	.postfix = "、",
	.calc = calc_alphabet_system,
};

static const symbol_t hiragana_iroha_symbols[] = {
	"い", "ろ", "は", "に", "ほ", "へ", "と", "ち", "り", "ぬ", "る", "を", "わ", "か", "よ", "た", "れ", "そ", "つ", "ね", "な", "ら", "む", "う", "ゐ", "の", "お", "く", "や", "ま", "け", "ふ", "こ", "え", "て", "あ", "さ", "き", "ゆ", "め", "み", "し", "ゑ", "ひ", "も", "せ", "す"
};
static struct list_counter_style lcs_hiragana_iroha = {
	.name = "hiragana-iroha",
	.symbols = hiragana_iroha_symbols,
	.items = (sizeof(hiragana_iroha_symbols) / SYMBOL_SIZE),
	.postfix = "、",
	.calc = calc_alphabet_system,
};

static const symbol_t katakana_symbols[] = {
	"ア", "イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ", "サ", "シ", "ス", "セ", "ソ", "タ", "チ", "ツ", "テ", "ト", "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ", "ヒ", "フ", "ヘ", "ホ", "マ", "ミ", "ム", "メ", "モ", "ヤ", "ユ", "ヨ", "ラ", "リ", "ル", "レ", "ロ", "ワ", "ヰ", "ヱ", "ヲ", "ン"
};
static struct list_counter_style lcs_katakana = {
	.name = "katakana",
	.symbols = katakana_symbols,
	.items = (sizeof(katakana_symbols) / SYMBOL_SIZE),
	.postfix = "、",
	.calc = calc_alphabet_system,
};

static const symbol_t katakana_iroha_symbols[] = {
	"イ", "ロ", "ハ", "ニ", "ホ", "ヘ", "ト", "チ", "リ", "ヌ", "ル", "ヲ", "ワ", "カ", "ヨ", "タ", "レ", "ソ", "ツ", "ネ", "ナ", "ラ", "ム", "ウ", "ヰ", "ノ", "オ", "ク", "ヤ", "マ", "ケ", "フ", "コ", "エ", "テ", "ア", "サ", "キ", "ユ", "メ", "ミ", "シ", "ヱ", "ヒ", "モ", "セ", "ス"
};
static struct list_counter_style lcs_katakana_iroha = {
	.name = "katakana-iroha",
	.symbols = katakana_iroha_symbols,
	.items = (sizeof(katakana_iroha_symbols) / SYMBOL_SIZE),
	.postfix = "、",
	.calc = calc_alphabet_system,
};


/* exported interface defined in select.h */
css_error css_computed_format_list_style(
		const css_computed_style *style,
		int value,
		char *buffer,
		size_t buffer_length,
		size_t *format_length)
{
	uint8_t type = get_list_style_type(style);

	size_t alen;
	uint8_t aval[20];
	const struct list_counter_style *cstyle;

	switch (type) {
	case CSS_LIST_STYLE_TYPE_DISC:
		cstyle = &lcs_disc;
		break;

	case CSS_LIST_STYLE_TYPE_CIRCLE:
		cstyle = &lcs_circle;
		break;

	case CSS_LIST_STYLE_TYPE_SQUARE:
		cstyle = &lcs_square;
		break;

	case CSS_LIST_STYLE_TYPE_DECIMAL:
		cstyle = &lcs_decimal;
		break;

	case CSS_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
		cstyle = &lcs_decimal_leading_zero;
		break;

	case CSS_LIST_STYLE_TYPE_LOWER_ROMAN:
		cstyle = &lcs_lower_roman;
		break;

	case CSS_LIST_STYLE_TYPE_UPPER_ROMAN:
		cstyle = &lcs_upper_roman;
		break;

	case CSS_LIST_STYLE_TYPE_LOWER_GREEK:
		cstyle = &lcs_lower_greek;
		break;

	case CSS_LIST_STYLE_TYPE_LOWER_ALPHA:
	case CSS_LIST_STYLE_TYPE_LOWER_LATIN:
		cstyle = &lcs_lower_alpha;
		break;

	case CSS_LIST_STYLE_TYPE_UPPER_ALPHA:
	case CSS_LIST_STYLE_TYPE_UPPER_LATIN:
		cstyle = &lcs_upper_alpha;
		break;

	case CSS_LIST_STYLE_TYPE_UPPER_ARMENIAN:
	case CSS_LIST_STYLE_TYPE_ARMENIAN:
		cstyle = &lcs_upper_armenian;
		break;

	case CSS_LIST_STYLE_TYPE_GEORGIAN:
		cstyle = &lcs_georgian;
		break;

	case CSS_LIST_STYLE_TYPE_NONE:
		*format_length = 0;
		return CSS_OK;

	case CSS_LIST_STYLE_TYPE_BINARY:
		cstyle = &lcs_binary;
		break;

	case CSS_LIST_STYLE_TYPE_OCTAL:
		cstyle = &lcs_octal;
		break;

	case CSS_LIST_STYLE_TYPE_LOWER_HEXADECIMAL:
		cstyle = &lcs_lower_hexadecimal;
		break;

	case CSS_LIST_STYLE_TYPE_UPPER_HEXADECIMAL:
		cstyle = &lcs_upper_hexadecimal;
		break;

	case CSS_LIST_STYLE_TYPE_ARABIC_INDIC:
		cstyle = &lcs_arabic_indic;
		break;

	case CSS_LIST_STYLE_TYPE_LOWER_ARMENIAN:
		cstyle = &lcs_lower_armenian;
		break;

	case CSS_LIST_STYLE_TYPE_BENGALI:
		cstyle = &lcs_bengali;
		break;

	case CSS_LIST_STYLE_TYPE_CAMBODIAN:
	case CSS_LIST_STYLE_TYPE_KHMER:
		cstyle = &lcs_cambodian;
		break;

	case CSS_LIST_STYLE_TYPE_CJK_DECIMAL:
		cstyle = &lcs_cjk_decimal;
		break;

	case CSS_LIST_STYLE_TYPE_DEVANAGARI:
		cstyle = &lcs_devanagari;
		break;

	case CSS_LIST_STYLE_TYPE_GUJARATI:
		cstyle = &lcs_gujarati;
		break;

	case CSS_LIST_STYLE_TYPE_GURMUKHI:
		cstyle = &lcs_gurmukhi;
		break;

	case CSS_LIST_STYLE_TYPE_HEBREW:
		cstyle = &lcs_hebrew;
		break;
	case CSS_LIST_STYLE_TYPE_KANNADA:
		cstyle = &lcs_kannada;
		break;
	case CSS_LIST_STYLE_TYPE_LAO:
		cstyle = &lcs_lao;
		break;
	case CSS_LIST_STYLE_TYPE_MALAYALAM:
		cstyle = &lcs_malayalam;
		break;
	case CSS_LIST_STYLE_TYPE_MONGOLIAN:
		cstyle = &lcs_mongolian;
		break;
	case CSS_LIST_STYLE_TYPE_MYANMAR:
		cstyle = &lcs_myanmar;
		break;
	case CSS_LIST_STYLE_TYPE_ORIYA:
		cstyle = &lcs_oriya;
		break;
	case CSS_LIST_STYLE_TYPE_PERSIAN:
		cstyle = &lcs_persian;
		break;
	case CSS_LIST_STYLE_TYPE_TAMIL:
		cstyle = &lcs_tamil;
		break;
	case CSS_LIST_STYLE_TYPE_TELUGU:
		cstyle = &lcs_telugu;
		break;
	case CSS_LIST_STYLE_TYPE_THAI:
		cstyle = &lcs_thai;
		break;
	case CSS_LIST_STYLE_TYPE_TIBETAN:
		cstyle = &lcs_tibetan;
		break;
	case CSS_LIST_STYLE_TYPE_CJK_EARTHLY_BRANCH:
		cstyle = &lcs_cjk_earthly_branch;
		break;
	case CSS_LIST_STYLE_TYPE_CJK_HEAVENLY_STEM:
		cstyle = &lcs_cjk_heavenly_stem;
		break;
	case CSS_LIST_STYLE_TYPE_HIAGANA:
		cstyle = &lcs_hiragana;
		break;
	case CSS_LIST_STYLE_TYPE_HIAGANA_IROHA:
		cstyle = &lcs_hiragana_iroha;
		break;
	case CSS_LIST_STYLE_TYPE_KATAKANA:
		cstyle = &lcs_katakana;
		break;
	case CSS_LIST_STYLE_TYPE_KATAKANA_IROHA:
		cstyle = &lcs_katakana_iroha;
		break;
	default:
		return CSS_BADPARM;
	}

	alen = cstyle->calc(aval, sizeof(aval), value, cstyle);

	/* ensure it is possible to calculate with the selected system */
	if ((alen == 0) || (alen >= sizeof(aval))) {
		/* retry in decimal */
		cstyle = &lcs_decimal;

		alen = cstyle->calc(aval, sizeof(aval), value, cstyle);
		if ((alen == 0) || (alen >= sizeof(aval))) {
			/* failed in decimal, give up */
			return CSS_INVALID;
		}
	}

	*format_length = map_aval_to_symbols(buffer, buffer_length, aval, alen, cstyle);

	return CSS_OK;
}
