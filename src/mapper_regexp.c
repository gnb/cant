/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * Copyright (c) 2001 Greg Banks
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

#include "cant.h"

CVSID("$Id: mapper_regexp.c,v 1.1 2001-11-10 03:17:24 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: this one can actually fail, handle that */
static void
regexp_new(mapper_t *ma)
{
    ma->private = pattern_new(ma->from, PAT_GROUPS|PAT_REGEXP);
}

static char *
regexp_map(mapper_t *ma, const char *filename)
{
    pattern_t *pat = (pattern_t *)ma->private;
    
    if (!pattern_match(pat, filename))
    	return 0;
    return pattern_replace(pat, ma->to);
}

static void
regexp_delete(mapper_t *ma)
{
    pattern_delete((pattern_t *)ma->private);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_ops_t regexp_ops = 
{
    "regexp",
    regexp_new,
    regexp_map,
    regexp_delete
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
