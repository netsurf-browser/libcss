# This file is part of LibCSS.
# Licensed under the MIT License,
# http://www.opensource.org/licenses/mit-license.php
# Copyright 2017 Lucas Neves <lcneves@gmail.com>

copyright = '''\
/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2017 The NetSurf Project
 */
'''

include_propget = '''\

#include "select/propget.h"
'''

calc_unions = '''\

typedef union {
	css_fixed value;
	lwc_string *calc;
} css_fixed_or_calc;
'''

assets = {}

assets['computed.h'] = {}
assets['computed.h']['header'] = copyright + calc_unions
assets['computed.h']['footer'] = ''

assets['propset.h'] = {}
assets['propset.h']['header'] = copyright + include_propget
assets['propset.h']['footer'] = ''

assets['propget.h'] = {}
assets['propget.h']['header'] = copyright
assets['propget.h']['footer'] = ''

