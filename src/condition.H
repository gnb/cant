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

#ifndef _condition_h_
#define _condition_h_ 1

#include "common.H"
#include "pattern.H"
#include "props.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
typedef struct condition_s    	condition_t;

/* Values for `flags' */
#define COND_IF    	    (1<<0)  	/* "if" attribute given */
#define COND_UNLESS  	    (1<<1)  	/* "unless" attribute given */
#define COND_MATCHES  	    (1<<2)  	/* "matches" or "matchesregex" attribute given */

struct condition_s
{
    unsigned int flags;
    char *property; 	    	    	/* name of property, "if" or "unless" */
    pattern_t pattern;	    	    	/* pattern, "matches" or "matchesregex" */
};

void condition_init(condition_t *);
void condition_free(condition_t *);
#if 0
condition_t *condition_new(void);
void condition_delete(condition_t *);
#endif
void condition_set_if(condition_t *, const char *property);
void condition_set_unless(condition_t *, const char *property);
void condition_set_matches(condition_t *, const char *pattern,
    	    	    	   gboolean case_sens);
void condition_set_matches_regex(condition_t *, const char *regex,
    	    	    	    	 gboolean case_sens);
gboolean condition_evaluate(const condition_t *, const props_t *props);
#if DEBUG
char *condition_describe(const condition_t *);
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _condition_h_ */
