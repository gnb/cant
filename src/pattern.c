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

CVSID("$Id: pattern.c,v 1.3 2001-11-06 09:29:06 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
pattern_init(pattern_t *pat, const char *pattern, gboolean case_sens)
{
    const char *p = pattern;
    estring restr;
    
#if DEBUG
    strassign(pat->pattern, pattern);
#endif

    estring_init(&restr);
    estring_append_char(&restr, '^');
    
    for ( ; *p ; p++)
    {
    	if (p[0] == '*' && p[1] == '*')
	{
	    estring_append_string(&restr, ".*");
	    p++;
	}
	else if (*p == '*')
	    estring_append_string(&restr, "[^/]*");
	else if (*p == '?')
	    estring_append_string(&restr, "[^/]");
	else if (strchr(".[]", *p) != 0)
	{
	    estring_append_char(&restr, '\\');
	    estring_append_char(&restr, *p);
	}
	else
	    estring_append_char(&restr, *p);
    }
    
    estring_append_char(&restr, '$');
    
#if DEBUG
    fprintf(stderr, "pattern_init: \"%s\" -> \"%s\"\n",
    	    	    	pattern, restr.data);
#endif
    regcomp(&pat->regex, restr.data, (case_sens ? 0 : REG_ICASE)|REG_NOSUB);
    estring_free(&restr);
}

void
pattern_free(pattern_t *pat)
{
#if DEBUG
    strdelete(pat->pattern);
#endif
    regfree(&pat->regex);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

pattern_t *
pattern_new(const char *pattern, gboolean case_sens)
{
    pattern_t *pat;
    
    pat = new(pattern_t);
    
    pattern_init(pat, pattern, case_sens);
    
    return pat;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
pattern_delete(pattern_t *pat)
{
    pattern_free(pat);	
    g_free(pat);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
pattern_match(const pattern_t *pat, const char *filename)
{
    gboolean ret;
    
    ret = (regexec(&pat->regex, filename,
    	    	    /*nmatch*/0, /*pmatch*/0, /*eflags*/0) == 0);
#if DEBUG
    fprintf(stderr, "pattern_match: pattern=\"%s\" filename=\"%s\" -> %s\n",
    	    pat->pattern, filename, (ret ? "true" : "false"));
#endif

    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
