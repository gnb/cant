/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _pattern_h_
#define _pattern_h_ 1

#include "common.h"
#include <regex.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
typedef struct pattern_s    	pattern_t;

/* Values for `flags' for pattern_init() */
#define PAT_CASE    	    (1<<0)  	/* case sensitive */
#define PAT_GROUPS  	    (1<<1)  	/* cater for groups, replace */
#define PAT_REGEXP  	    (1<<2)  	/* raw regexp, not wildcards */

#define _PAT_NGROUPS	    10

struct pattern_s
{
    regex_t regex;
    
    char *groups[_PAT_NGROUPS];
    
#if DEBUG
    char *pattern;
#endif
};

void pattern_init(pattern_t *, const char *s, unsigned flags);
void pattern_free(pattern_t *);
pattern_t *pattern_new(const char *s, unsigned flags);
void pattern_delete(pattern_t *);
/* Note that `pat' is const if the PAT_GROUPS flag was not supplied */
gboolean pattern_match(pattern_t *pat, const char *filename);
/* This function is const-only so doesn't support PAT_GROUPS */
gboolean pattern_match_c(const pattern_t *pat, const char *filename);
/* retrieve the n'th group, if PAT_GROUPS was set */
const char *pattern_group(const pattern_t *, unsigned int /*0-9*/i);
/* replace instances of \N with the N'th group, if PAT_GROUPS was set */
char *pattern_replace(const pattern_t *, const char *rep);

/*
 * Groups are automatic around all wildcard metachars, so
 * pattern_init(p, "**a/b*.c", PAT_GROUPS) will result in
 * two groups, i=1 matches "**" and i=2 matches "*".
 */
 

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _pattern_h_ */
