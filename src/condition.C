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

#include "condition.H"
#if DEBUG
#include "estring.H"
#endif

CVSID("$Id: condition.C,v 1.3 2002-04-06 12:40:16 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
condition_init(condition_t *cond)
{
    memset(cond, 0, sizeof(*cond));
}

void
condition_free(condition_t *cond)
{
    strdelete(cond->property);
    cond->pattern.hacky_dtor();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if 0

condition_t *
condition_new()
{
    condition_t *cond;
    
    cond = new(condition_t);
    
    condition_init(cond);
    
    return cond;
}

void
condition_delete(condition_t *cond)
{
    condition_free(cond);	
    g_free(cond);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
condition_set_property(
    condition_t *cond,
    unsigned int flags,
    const char *property)
{
    strassign(cond->property, property);
    cond->flags = (cond->flags & ~(COND_IF|COND_UNLESS)) | flags;
}

void
condition_set_if(condition_t *cond, const char *property)
{
    condition_set_property(cond, COND_IF, property);
}

void
condition_set_unless(condition_t *cond, const char *property)
{
    condition_set_property(cond, COND_UNLESS, property);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
condition_set_match(
    condition_t *cond,
    unsigned int patt_flags,
    const char *pattern)
{
    if (cond->flags & COND_MATCHES)
	cond->pattern.hacky_dtor();
    cond->pattern.init(pattern, patt_flags);
    cond->flags |= COND_MATCHES;
}

void
condition_set_matches(
    condition_t *cond,
    const char *pattern,
    gboolean case_sens)
{
    condition_set_match(cond,
    	    		(case_sens ? PAT_CASE : 0),
			 pattern);
}

void
condition_set_matches_regex(
    condition_t *cond,
    const char *regex,
    gboolean case_sens)
{
    condition_set_match(cond,
    	    		(case_sens ? PAT_CASE : 0)|PAT_REGEXP,
			 regex);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
condition_evaluate(const condition_t *cond, const props_t *props)
{
    const char *value;
    char *expvalue;
    gboolean res = TRUE;
    
    if (!(cond->flags & (COND_IF|COND_UNLESS)))
    	return TRUE;	    	/* no condition -> trivially true */
	
    value = props->get(cond->property);
    expvalue = props->expand(value);

    if (cond->flags & COND_MATCHES)
    {
    	/* match against pattern to get a boolean */
    	res = cond->pattern.match_c((expvalue == 0 ? "" : expvalue));
    }
    else
    {
    	/*
    	 * Treat value as a boolean:
	 * undefined -> false
	 * empty -> false
	 * "false", "off", "no", "0" -> false
	 * any other value -> true
	 */
	if (value == 0 || *value == '\0')
	    res = FALSE;
	else
	    res = strbool(expvalue, TRUE);
    }
    
    /* at this point `res' is true if the condition matched */
    
    if (cond->flags & COND_UNLESS)
    	res = !res;
	
#if DEBUG
    {
    	char *desc = condition_describe(cond);
	fprintf(stderr, "Condition %s -> %s\n", desc, (res ? "true" : "false"));
	g_free(desc);
    }
#endif
    
    strdelete(expvalue);
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if DEBUG

char *
condition_describe(const condition_t *cond)
{
    estring e;
    
    estring_init(&e);
    
    estring_append_string(&e, "{ ");
    
    if (cond->flags & COND_IF)
    	estring_append_printf(&e, "if=\"%s\"", cond->property);
    else if (cond->flags & COND_UNLESS)
    	estring_append_printf(&e, "unless=\"%s\"", cond->property);

    if (cond->flags & COND_MATCHES)
    	estring_append_printf(&e, " matches=\"%s\"", cond->pattern.pattern);
    
    estring_append_string(&e, " }");

    return e.data;
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
