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

CVSID("$Id: mapper_glob.C,v 1.7 2002-04-13 12:30:42 gnb Exp $");

class mapper_glob_t : public mapper_t
{
private:
    pattern_t pattern_;

public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_glob_t()
{
}

~mapper_glob_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
init()
{
    estring replace;
    const char *p;
    int nstar, nother;
    
    nstar = nother = 0;
    for (p = from_ ; *p ; p++)
    {
    	if (*p == '*')
	    nstar++;
	else if (strchr("?[]", *p) != 0)
	    nother++;
    }
    if (nstar != 1 || nother != 0)
    {
    	log::errorf("Bad \"from\" expression \"%s\"\n", from_.data());
    	return FALSE;
    }
    if (!pattern_.set_pattern(from_, PAT_GROUPS))
    	return FALSE;
    


    nstar = nother = 0;
    for (p = to_ ; *p ; p++)
    {
    	if (*p == '*')
	{
	    nstar++;
	    replace.append_string("\\1");
	}
	else if (strchr("?[]", *p) != 0 ||
	    	 (p[0] == '\\' && isdigit(p[1])))
	    nother++;
	else
	    replace.append_char(*p);
    }
    if (nstar != 1 || nother != 0)
    {
    	log::errorf("Bad \"to\" expression \"%s\"\n", to_.data());
    	return FALSE;
    }
    /* might as well stash this in `to', it has no other use */
    to_ = replace.take();
    
    return TRUE;
}

char *
map(const char *filename)
{
    const char *base;
    char *rep;
    
    base = file_basename_c(filename);
    
    if (!pattern_.match(base))
    	return 0;
	
    rep = pattern_.replace(to_);
    
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

MAPPER_DEFINE_CLASS(glob);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
