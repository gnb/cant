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
#include "string_var.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
class condition_t
{
private:
    enum _flags
    {
	/* Values for `flags' */
	COND_IF    	    =(1<<0),  	/* "if" attribute given */
	COND_UNLESS  	    =(1<<1),  	/* "unless" attribute given */
	COND_MATCHES  	    =(1<<2)  	/* "matches" or "matchesregex" attribute given */
    };

    unsigned int flags_;
    string_var property_; 	    	/* name of property, "if" or "unless" */
    pattern_t pattern_;	    	    	/* pattern, "matches" or "matchesregex" */

    void set_property(unsigned int flags, const char *property);
    gboolean set_match(unsigned int patt_flags, const char *pattern);

public:
    condition_t();
    ~condition_t();
    
    void set_if(const char *property);
    void set_unless(const char *property);
    gboolean set_matches(const char *pattern, gboolean case_sens);
    gboolean set_matches_regex(const char *regex, gboolean case_sens);

    gboolean evaluate(const props_t *props) const;

#if DEBUG
    char *describe() const;
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _condition_h_ */
