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

#include "cant.H"

CVSID("$Id: mapper_glob.C,v 1.1 2002-03-29 12:36:26 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
static gboolean
glob_new(mapper_t *ma)
{
    estring replace;
    const char *p;
    int nstar, nother;
    
    nstar = nother = 0;
    for (p = ma->from ; *p ; p++)
    {
    	if (*p == '*')
	    nstar++;
	else if (strchr("?[]", *p) != 0)
	    nother++;
    }
    if (nstar != 1 || nother != 0)
    {
    	parse_error("bad \"from\" expression \"%s\"\n", ma->from);
    	return FALSE;
    }
    if ((ma->private_data = pattern_new(ma->from, PAT_GROUPS)) == 0)
    	return FALSE;
    


    estring_init(&replace);
    nstar = nother = 0;
    for (p = ma->to ; *p ; p++)
    {
    	if (*p == '*')
	{
	    nstar++;
	    estring_append_string(&replace, "\\1");
	}
	else if (strchr("?[]", *p) != 0 ||
	    	 (p[0] == '\\' && isdigit(p[1])))
	    nother++;
	else
	    estring_append_char(&replace, *p);
    }
    if (nstar != 1 || nother != 0)
    {
    	parse_error("bad \"to\" expression \"%s\"\n", ma->to);
    	estring_free(&replace);
    	return FALSE;
    }
    /* might as well stash this in `to', it has no other use */
    g_free(ma->to);
    ma->to = replace.data;
    
    return TRUE;
}

static char *
glob_map(mapper_t *ma, const char *filename)
{
    pattern_t *pat = (pattern_t *)ma->private_data;
    const char *base;
    char *rep;
    
    base = file_basename_c(filename);
    
    if (!pattern_match(pat, base))
    	return 0;
	
    rep = pattern_replace(pat, ma->to);
    
    if (base != filename)
    {
    	char *base2 = g_strndup(filename, (base - filename));
	char *rep2 = rep;
	rep = g_strconcat(base2, rep, 0);
	g_free(rep2);
	g_free(base2);
    }
    
    return rep;
}

static void
glob_delete(mapper_t *ma)
{
    if (ma->private_data != 0)
	pattern_delete((pattern_t *)ma->private_data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_ops_t glob_ops = 
{
    "glob",
    glob_new,
    glob_map,
    glob_delete
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
