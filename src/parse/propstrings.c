/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include "parse/propstrings.h"
#include "stylesheet.h"

#include <assert.h>

/** build string map entry with a string constant */
#define SMAP(s) { s, (sizeof((s)) - 1) /* -1 for '\0' */ }


typedef struct stringmap_entry {
	const char *data;
	size_t len;
} stringmap_entry;

typedef struct css__propstrings_ctx {
	uint32_t count;
	lwc_string *strings[LAST_KNOWN];
} css__propstrings_ctx;

static css__propstrings_ctx css__propstrings;

/* Must be synchronised with enum in propstrings.h */
const stringmap_entry stringmap[LAST_KNOWN] = {
	SMAP("*"),

	SMAP(":"),
	SMAP(","),
	SMAP(";"),
	SMAP("{"),
	SMAP("}"),
	SMAP("0"),

	SMAP("charset"),
	SMAP("import"),
	SMAP("media"),
	SMAP("namespace"),
	SMAP("font-face"),
	SMAP("page"),

	SMAP("aural"),
	SMAP("braille"),
	SMAP("embossed"),
	SMAP("handheld"),
	SMAP("print"),
	SMAP("projection"),
	SMAP("screen"),
	SMAP("speech"),
	SMAP("tty"),
	SMAP("tv"),
	SMAP("all"),

	SMAP("first-child"),
	SMAP("link"),
	SMAP("visited"),
	SMAP("hover"),
	SMAP("active"),
	SMAP("focus"),
	SMAP("lang"),
	SMAP("first"),
	SMAP("root"),
	SMAP("nth-child"),
	SMAP("nth-last-child"),
	SMAP("nth-of-type"),
	SMAP("nth-last-of-type"),
	SMAP("last-child"),
	SMAP("first-of-type"),
	SMAP("last-of-type"),
	SMAP("only-child"),
	SMAP("only-of-type"),
	SMAP("empty"),
	SMAP("target"),
	SMAP("enabled"),
	SMAP("disabled"),
	SMAP("checked"),
	SMAP("not"),

	SMAP("first-line"),
	SMAP("first-letter"),
	SMAP("before"),
	SMAP("after"),

	SMAP("align-content"),
	SMAP("align-items"),
	SMAP("align-self"),
	SMAP("azimuth"),
	SMAP("background"),
	SMAP("background-attachment"),
	SMAP("background-color"),
	SMAP("background-image"),
	SMAP("background-position"),
	SMAP("background-repeat"),
	SMAP("border"),
	SMAP("border-bottom"),
	SMAP("border-bottom-color"),
	SMAP("border-bottom-style"),
	SMAP("border-bottom-width"),
	SMAP("border-collapse"),
	SMAP("border-color"),
	SMAP("border-left"),
	SMAP("border-left-color"),
	SMAP("border-left-style"),
	SMAP("border-left-width"),
	SMAP("border-right"),
	SMAP("border-right-color"),
	SMAP("border-right-style"),
	SMAP("border-right-width"),
	SMAP("border-spacing"),
	SMAP("border-style"),
	SMAP("border-top"),
	SMAP("border-top-color"),
	SMAP("border-top-style"),
	SMAP("border-top-width"),
	SMAP("border-width"),
	SMAP("bottom"),
	SMAP("box-sizing"),
	SMAP("break-after"),
	SMAP("break-before"),
	SMAP("break-inside"),
	SMAP("caption-side"),
	SMAP("clear"),
	SMAP("clip"),
	SMAP("color"),
	SMAP("columns"),
	SMAP("column-count"),
	SMAP("column-fill"),
	SMAP("column-gap"),
	SMAP("column-rule"),
	SMAP("column-rule-color"),
	SMAP("column-rule-style"),
	SMAP("column-rule-width"),
	SMAP("column-span"),
	SMAP("column-width"),
	SMAP("content"),
	SMAP("counter-increment"),
	SMAP("counter-reset"),
	SMAP("cue"),
	SMAP("cue-after"),
	SMAP("cue-before"),
	SMAP("cursor"),
	SMAP("direction"),
	SMAP("display"),
	SMAP("elevation"),
	SMAP("empty-cells"),
	SMAP("flex"),
	SMAP("flex-basis"),
	SMAP("flex-direction"),
	SMAP("flex-flow"),
	SMAP("flex-grow"),
	SMAP("flex-shrink"),
	SMAP("flex-wrap"),
	SMAP("float"),
	SMAP("font"),
	SMAP("font-family"),
	SMAP("font-size"),
	SMAP("font-style"),
	SMAP("font-variant"),
	SMAP("font-weight"),
	SMAP("height"),
	SMAP("justify-content"),
	SMAP("left"),
	SMAP("letter-spacing"),
	SMAP("line-height"),
	SMAP("list-style"),
	SMAP("list-style-image"),
	SMAP("list-style-position"),
	SMAP("list-style-type"),
	SMAP("margin"),
	SMAP("margin-bottom"),
	SMAP("margin-left"),
	SMAP("margin-right"),
	SMAP("margin-top"),
	SMAP("max-height"),
	SMAP("max-width"),
	SMAP("min-height"),
	SMAP("min-width"),
	SMAP("opacity"),
	SMAP("order"),
	SMAP("orphans"),
	SMAP("outline"),
	SMAP("outline-color"),
	SMAP("outline-style"),
	SMAP("outline-width"),
	SMAP("overflow"),
	SMAP("overflow-x"),
	SMAP("overflow-y"),
	SMAP("padding"),
	SMAP("padding-bottom"),
	SMAP("padding-left"),
	SMAP("padding-right"),
	SMAP("padding-top"),
	SMAP("page-break-after"),
	SMAP("page-break-before"),
	SMAP("page-break-inside"),
	SMAP("pause"),
	SMAP("pause-after"),
	SMAP("pause-before"),
	SMAP("pitch-range"),
	SMAP("pitch"),
	SMAP("play-during"),
	SMAP("position"),
	SMAP("quotes"),
	SMAP("richness"),
	SMAP("right"),
	SMAP("speak-header"),
	SMAP("speak-numeral"),
	SMAP("speak-punctuation"),
	SMAP("speak"),
	SMAP("speech-rate"),
	SMAP("stress"),
	SMAP("table-layout"),
	SMAP("text-align"),
	SMAP("text-decoration"),
	SMAP("text-indent"),
	SMAP("text-transform"),
	SMAP("top"),
	SMAP("unicode-bidi"),
	SMAP("vertical-align"),
	SMAP("visibility"),
	SMAP("voice-family"),
	SMAP("volume"),
	SMAP("white-space"),
	SMAP("widows"),
	SMAP("width"),
	SMAP("word-spacing"),
	SMAP("writing-mode"),
	SMAP("z-index"),

	SMAP("inherit"),
	SMAP("unset"),
	SMAP("important"),
	SMAP("none"),
	SMAP("both"),
	SMAP("fixed"),
	SMAP("scroll"),
	SMAP("transparent"),
	SMAP("no-repeat"),
	SMAP("repeat-x"),
	SMAP("repeat-y"),
	SMAP("repeat"),
	SMAP("hidden"),
	SMAP("dotted"),
	SMAP("dashed"),
	SMAP("solid"),
	SMAP("double"),
	SMAP("groove"),
	SMAP("ridge"),
	SMAP("inset"),
	SMAP("outset"),
	SMAP("thin"),
	SMAP("medium"),
	SMAP("thick"),
	SMAP("collapse"),
	SMAP("separate"),
	SMAP("auto"),
	SMAP("ltr"),
	SMAP("rtl"),
	SMAP("inline"),
	SMAP("block"),
	SMAP("list-item"),
	SMAP("run-in"),
	SMAP("inline-block"),
	SMAP("table"),
	SMAP("inline-table"),
	SMAP("table-row-group"),
	SMAP("table-header-group"),
	SMAP("table-footer-group"),
	SMAP("table-row"),
	SMAP("table-column-group"),
	SMAP("table-column"),
	SMAP("table-cell"),
	SMAP("table-caption"),
	SMAP("below"),
	SMAP("level"),
	SMAP("above"),
	SMAP("higher"),
	SMAP("lower"),
	SMAP("show"),
	SMAP("hide"),
	SMAP("xx-small"),
	SMAP("x-small"),
	SMAP("small"),
	SMAP("large"),
	SMAP("x-large"),
	SMAP("xx-large"),
	SMAP("larger"),
	SMAP("smaller"),
	SMAP("normal"),
	SMAP("italic"),
	SMAP("oblique"),
	SMAP("small-caps"),
	SMAP("bold"),
	SMAP("bolder"),
	SMAP("lighter"),
	SMAP("inside"),
	SMAP("outside"),
	SMAP("disc"),
	SMAP("circle"),
	SMAP("square"),
	SMAP("decimal"),
	SMAP("decimal-leading-zero"),
	SMAP("lower-roman"),
	SMAP("upper-roman"),
	SMAP("lower-greek"),
	SMAP("lower-latin"),
	SMAP("upper-latin"),
	SMAP("armenian"),
	SMAP("georgian"),
	SMAP("lower-alpha"),
	SMAP("upper-alpha"),
	SMAP("binary"),
	SMAP("octal"),
	SMAP("lower-hexadecimal"),
	SMAP("upper-hexadecimal"),
	SMAP("arabic-indic"),
	SMAP("lower-armenian"),
	SMAP("upper-armenian"),
	SMAP("bengali"),
	SMAP("cambodian"),
	SMAP("khmer"),
	SMAP("cjk-decimal"),
	SMAP("devanagari"),
	SMAP("gujarati"),
	SMAP("gurmukhi"),
	SMAP("hebrew"),
	SMAP("kannada"),
	SMAP("lao"),
	SMAP("malayalam"),
	SMAP("mongolian"),
	SMAP("myanmar"),
	SMAP("oriya"),
	SMAP("persian"),
	SMAP("tamil"),
	SMAP("telugu"),
	SMAP("thai"),
	SMAP("tibetan"),
	SMAP("cjk-earthly-branch"),
	SMAP("cjk-heavenly-stem"),
	SMAP("hiragana"),
	SMAP("hiragana-iroha"),
	SMAP("katakana"),
	SMAP("katakana-iroha"),
	SMAP("japanese-informal"),
	SMAP("japanese-formal"),
	SMAP("korean-hangul-formal"),
	SMAP("korean-hanja-informal"),
	SMAP("korean-hanja-formal"),
	SMAP("invert"),
	SMAP("visible"),
	SMAP("always"),
	SMAP("avoid"),
	SMAP("x-low"),
	SMAP("low"),
	SMAP("high"),
	SMAP("x-high"),
	SMAP("static"),
	SMAP("relative"),
	SMAP("absolute"),
	SMAP("once"),
	SMAP("digits"),
	SMAP("continuous"),
	SMAP("code"),
	SMAP("spell-out"),
	SMAP("x-slow"),
	SMAP("slow"),
	SMAP("fast"),
	SMAP("x-fast"),
	SMAP("faster"),
	SMAP("slower"),
	SMAP("center"),
	SMAP("justify"),
	SMAP("capitalize"),
	SMAP("uppercase"),
	SMAP("lowercase"),
	SMAP("embed"),
	SMAP("bidi-override"),
	SMAP("baseline"),
	SMAP("sub"),
	SMAP("super"),
	SMAP("text-top"),
	SMAP("middle"),
	SMAP("text-bottom"),
	SMAP("silent"),
	SMAP("x-soft"),
	SMAP("soft"),
	SMAP("loud"),
	SMAP("x-loud"),
	SMAP("pre"),
	SMAP("nowrap"),
	SMAP("pre-wrap"),
	SMAP("pre-line"),
	SMAP("leftwards"),
	SMAP("rightwards"),
	SMAP("left-side"),
	SMAP("far-left"),
	SMAP("center-left"),
	SMAP("center-right"),
	SMAP("far-right"),
	SMAP("right-side"),
	SMAP("behind"),
	SMAP("rect"),
	SMAP("open-quote"),
	SMAP("close-quote"),
	SMAP("no-open-quote"),
	SMAP("no-close-quote"),
	SMAP("attr"),
	SMAP("counter"),
	SMAP("counters"),
	SMAP("crosshair"),
	SMAP("default"),
	SMAP("pointer"),
	SMAP("move"),
	SMAP("e-resize"),
	SMAP("ne-resize"),
	SMAP("nw-resize"),
	SMAP("n-resize"),
	SMAP("se-resize"),
	SMAP("sw-resize"),
	SMAP("s-resize"),
	SMAP("w-resize"),
	SMAP("text"),
	SMAP("wait"),
	SMAP("help"),
	SMAP("progress"),
	SMAP("serif"),
	SMAP("sans-serif"),
	SMAP("cursive"),
	SMAP("fantasy"),
	SMAP("monospace"),
	SMAP("male"),
	SMAP("female"),
	SMAP("child"),
	SMAP("mix"),
	SMAP("underline"),
	SMAP("overline"),
	SMAP("line-through"),
	SMAP("blink"),
	SMAP("rgb"),
	SMAP("rgba"),
	SMAP("hsl"),
	SMAP("hsla"),
	SMAP("-libcss-left"),
	SMAP("-libcss-center"),
	SMAP("-libcss-right"),
	SMAP("currentColor"),
	SMAP("odd"),
	SMAP("even"),
	SMAP("src"),
	SMAP("local"),
	SMAP("initial"),
	SMAP("revert"),
	SMAP("format"),
	SMAP("woff"),
	SMAP("truetype"),
	SMAP("opentype"),
	SMAP("embedded-opentype"),
	SMAP("svg"),
	SMAP("column"),
	SMAP("avoid-page"),
	SMAP("avoid-column"),
	SMAP("balance"),
	SMAP("horizontal-tb"),
	SMAP("vertical-rl"),
	SMAP("vertical-lr"),
	SMAP("content-box"),
	SMAP("border-box"),
	SMAP("stretch"),
	SMAP("inline-flex"),
	SMAP("flex-start"),
	SMAP("flex-end"),
	SMAP("space-between"),
	SMAP("space-around"),
	SMAP("space-evenly"),
	SMAP("row"),
	SMAP("row-reverse"),
	SMAP("column-reverse"),
	SMAP("wrap"),
	SMAP("wrap-reverse"),
	SMAP("and"),
	SMAP("or"),
	SMAP("only"),
	SMAP("infinite"),
	SMAP("grid"),
	SMAP("inline-grid"),
	SMAP("sticky"),
	SMAP("calc"),

	SMAP("aliceblue"),
	SMAP("antiquewhite"),
	SMAP("aqua"),
	SMAP("aquamarine"),
	SMAP("azure"),
	SMAP("beige"),
	SMAP("bisque"),
	SMAP("black"),
	SMAP("blanchedalmond"),
	SMAP("blue"),
	SMAP("blueviolet"),
	SMAP("brown"),
	SMAP("burlywood"),
	SMAP("cadetblue"),
	SMAP("chartreuse"),
	SMAP("chocolate"),
	SMAP("coral"),
	SMAP("cornflowerblue"),
	SMAP("cornsilk"),
	SMAP("crimson"),
	SMAP("cyan"),
	SMAP("darkblue"),
	SMAP("darkcyan"),
	SMAP("darkgoldenrod"),
	SMAP("darkgray"),
	SMAP("darkgreen"),
	SMAP("darkgrey"),
	SMAP("darkkhaki"),
	SMAP("darkmagenta"),
	SMAP("darkolivegreen"),
	SMAP("darkorange"),
	SMAP("darkorchid"),
	SMAP("darkred"),
	SMAP("darksalmon"),
	SMAP("darkseagreen"),
	SMAP("darkslateblue"),
	SMAP("darkslategray"),
	SMAP("darkslategrey"),
	SMAP("darkturquoise"),
	SMAP("darkviolet"),
	SMAP("deeppink"),
	SMAP("deepskyblue"),
	SMAP("dimgray"),
	SMAP("dimgrey"),
	SMAP("dodgerblue"),
	SMAP("feldspar"),
	SMAP("firebrick"),
	SMAP("floralwhite"),
	SMAP("forestgreen"),
	SMAP("fuchsia"),
	SMAP("gainsboro"),
	SMAP("ghostwhite"),
	SMAP("gold"),
	SMAP("goldenrod"),
	SMAP("gray"),
	SMAP("green"),
	SMAP("greenyellow"),
	SMAP("grey"),
	SMAP("honeydew"),
	SMAP("hotpink"),
	SMAP("indianred"),
	SMAP("indigo"),
	SMAP("ivory"),
	SMAP("khaki"),
	SMAP("lavender"),
	SMAP("lavenderblush"),
	SMAP("lawngreen"),
	SMAP("lemonchiffon"),
	SMAP("lightblue"),
	SMAP("lightcoral"),
	SMAP("lightcyan"),
	SMAP("lightgoldenrodyellow"),
	SMAP("lightgray"),
	SMAP("lightgreen"),
	SMAP("lightgrey"),
	SMAP("lightpink"),
	SMAP("lightsalmon"),
	SMAP("lightseagreen"),
	SMAP("lightskyblue"),
	SMAP("lightslateblue"),
	SMAP("lightslategray"),
	SMAP("lightslategrey"),
	SMAP("lightsteelblue"),
	SMAP("lightyellow"),
	SMAP("lime"),
	SMAP("limegreen"),
	SMAP("linen"),
	SMAP("magenta"),
	SMAP("maroon"),
	SMAP("mediumaquamarine"),
	SMAP("mediumblue"),
	SMAP("mediumorchid"),
	SMAP("mediumpurple"),
	SMAP("mediumseagreen"),
	SMAP("mediumslateblue"),
	SMAP("mediumspringgreen"),
	SMAP("mediumturquoise"),
	SMAP("mediumvioletred"),
	SMAP("midnightblue"),
	SMAP("mintcream"),
	SMAP("mistyrose"),
	SMAP("moccasin"),
	SMAP("navajowhite"),
	SMAP("navy"),
	SMAP("oldlace"),
	SMAP("olive"),
	SMAP("olivedrab"),
	SMAP("orange"),
	SMAP("orangered"),
	SMAP("orchid"),
	SMAP("palegoldenrod"),
	SMAP("palegreen"),
	SMAP("paleturquoise"),
	SMAP("palevioletred"),
	SMAP("papayawhip"),
	SMAP("peachpuff"),
	SMAP("peru"),
	SMAP("pink"),
	SMAP("plum"),
	SMAP("powderblue"),
	SMAP("purple"),
	SMAP("red"),
	SMAP("rosybrown"),
	SMAP("royalblue"),
	SMAP("saddlebrown"),
	SMAP("salmon"),
	SMAP("sandybrown"),
	SMAP("seagreen"),
	SMAP("seashell"),
	SMAP("sienna"),
	SMAP("silver"),
	SMAP("skyblue"),
	SMAP("slateblue"),
	SMAP("slategray"),
	SMAP("slategrey"),
	SMAP("snow"),
	SMAP("springgreen"),
	SMAP("steelblue"),
	SMAP("tan"),
	SMAP("teal"),
	SMAP("thistle"),
	SMAP("tomato"),
	SMAP("turquoise"),
	SMAP("violet"),
	SMAP("violetred"),
	SMAP("wheat"),
	SMAP("white"),
	SMAP("whitesmoke"),
	SMAP("yellow"),
	SMAP("yellowgreen")
};


/**
 * Obtain pointer to interned propstring list
 *
 * \param sheet	    Returns pointer to propstring table
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion
 *
 * The propstring list is generated with the first call to this function and
 * destroyed when it has no more users.  Call css__propstrings_unref() when
 * finished with the propstring list.
 */
css_error css__propstrings_get(lwc_string ***strings)
{
	if (css__propstrings.count > 0) {
		css__propstrings.count++;
	} else {
		int i;
		lwc_error lerror;

		/* Intern all known strings */
		for (i = 0; i < LAST_KNOWN; i++) {
			lerror = lwc_intern_string(stringmap[i].data,
					stringmap[i].len,
					&css__propstrings.strings[i]);

			if (lerror != lwc_error_ok)
				return CSS_NOMEM;
		}
		css__propstrings.count++;
	}

	*strings = css__propstrings.strings;

	return CSS_OK;
}

/**
 * Reduce reference count for propstring list by one.
 *
 * When count hits zero, the list is destroyed.
 */
void css__propstrings_unref(void)
{
	css__propstrings.count--;

	if (css__propstrings.count == 0) {
		int i;

		for (i = 0; i < LAST_KNOWN; i++)
			lwc_string_unref(css__propstrings.strings[i]);
	}
}
