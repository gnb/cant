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

CVSID("$Id: condition.C,v 1.4 2002-04-07 04:23:25 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

condition_t::condition_t()
{
    // only useful when using condition_t auto variables
    memset(this, 0, sizeof(*this));
}

condition_t::~condition_t()
{
    strdelete(property_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
condition_t::set_property(unsigned int flags, const char *property)
{
    strassign(property_, property);
    flags_ = (flags_ & ~(COND_IF|COND_UNLESS)) | flags;
}

void
condition_t::set_if(const char *property)
{
    set_property(COND_IF, property);
}

void
condition_t::set_unless(const char *property)
{
    set_property(COND_UNLESS, property);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
condition_t::set_match(unsigned int patt_flags, const char *pattern)
{
    flags_ |= COND_MATCHES;
    return pattern_.set_pattern(pattern, patt_flags);
}

gboolean
condition_t::set_matches(const char *pattern, gboolean case_sens)
{
    return set_match((case_sens ? PAT_CASE : 0), pattern);
}

gboolean
condition_t::set_matches_regex(const char *regex, gboolean case_sens)
{
    return set_match((case_sens ? PAT_CASE : 0)|PAT_REGEXP, regex);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
condition_t::evaluate(const props_t *props) const
{
    const char *value;
    char *expvalue;
    gboolean res = TRUE;
    
    if (!(flags_ & (COND_IF|COND_UNLESS)))
    	return TRUE;	    	/* no condition -> trivially true */
	
    value = props->get(property_);
    expvalue = props->expand(value);

    if (flags_ & COND_MATCHES)
    {
    	/* match against pattern to get a boolean */
    	res = pattern_.match_c((expvalue == 0 ? "" : expvalue));
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
    
    if (flags_ & COND_UNLESS)
    	res = !res;
	
#if DEBUG
    {
    	char *desc = describe();
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
condition_t::describe() const
{
    estring e;
    
    estring_init(&e);
    
    estring_append_string(&e, "{ ");
    
    if (flags_ & COND_IF)
    	estring_append_printf(&e, "if=\"%s\"", property_);
    else if (flags_ & COND_UNLESS)
    	estring_append_printf(&e, "unless=\"%s\"", property_);

    if (flags_ & COND_MATCHES)
    	estring_append_printf(&e, " matches=\"%s\"", pattern_.pattern);
    
    estring_append_string(&e, " }");

    return e.data;
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
