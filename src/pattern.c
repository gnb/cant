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

#include "pattern.h"
#include "estring.h"

CVSID("$Id: pattern.c,v 1.6 2001-11-13 04:08:05 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
pattern_init(pattern_t *pat, const char *pattern, unsigned flags)
{
    const char *p = pattern;
    estring restr;
    unsigned reflags = 0;
    
#if DEBUG
    strassign(pat->pattern, pattern);
#endif

    estring_init(&restr);
    
    if (flags & PAT_REGEXP)
    {
    	estring_append_string(&restr, pattern);
	reflags |= REG_EXTENDED;
    }
    else
    {
	estring_append_char(&restr, '^');

	for ( ; *p ; p++)
	{
    	    if (p[0] == '*' && p[1] == '*')
	    {
		estring_append_string(&restr, "\\(.*\\)");
		p++;
	    }
	    else if (*p == '*')
		estring_append_string(&restr, "\\([^/]*\\)");
	    else if (*p == '?')
		estring_append_string(&restr, "\\([^/]\\)");
	    else if (strchr(".^$\\", *p) != 0)
	    {
		estring_append_char(&restr, '\\');
		estring_append_char(&restr, *p);
	    }
	    else
		estring_append_char(&restr, *p);
	}

	estring_append_char(&restr, '$');
    }
        
#if DEBUG
    fprintf(stderr, "pattern_init: \"%s\" -> \"%s\"\n",
    	    	    	pattern, restr.data);
#endif
    if (!(flags & PAT_CASE)) reflags |= REG_ICASE;
    if (!(flags & PAT_GROUPS)) reflags |= REG_NOSUB;
    regcomp(&pat->regex, restr.data, reflags);
    estring_free(&restr);
}

void
pattern_free(pattern_t *pat)
{
    int i;
    
#if DEBUG
    strdelete(pat->pattern);
#endif
    for (i = 0 ; i < _PAT_NGROUPS ; i++)
	strdelete(pat->groups[i]);
    regfree(&pat->regex);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

pattern_t *
pattern_new(const char *pattern, unsigned flags)
{
    pattern_t *pat;
    
    pat = new(pattern_t);
    
    pattern_init(pat, pattern, flags);
    
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
pattern_match(pattern_t *pat, const char *filename)
{
    gboolean ret;
    int i;
    regmatch_t matches[_PAT_NGROUPS];
    
    for (i = 0 ; i < _PAT_NGROUPS ; i++)
	strdelete(pat->groups[i]);

    memset(matches, 0xff, sizeof(matches));
    ret = (regexec(&pat->regex, filename,
    	    	    _PAT_NGROUPS, matches, /*eflags*/0) == 0);
#if DEBUG
    fprintf(stderr, "pattern_match: pattern=\"%s\" filename=\"%s\" -> %s\n",
    	    pat->pattern, filename, (ret ? "true" : "false"));
#endif

    if (ret)
    {
	for (i = 0 ; i < _PAT_NGROUPS && matches[i].rm_so >= 0 ; i++)
	{
	    pat->groups[i] = g_strndup(
	    	    	    	    filename+matches[i].rm_so,
				    matches[i].rm_eo-matches[i].rm_so);
#if DEBUG
	    fprintf(stderr, "               \\%d=\"%s\" [%d,%d]\n",
	    	    	    	i, pat->groups[i],
				matches[i].rm_so, matches[i].rm_eo);
#endif				    
	}
    }

    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
pattern_group(const pattern_t *pat, unsigned int /*0-9*/i)
{
    return (i >= _PAT_NGROUPS ? 0 : pat->groups[i]);
}

char *
pattern_replace(const pattern_t *pat, const char *replace)
{
    const char *rep = replace;
    estring e;
    
    if (rep == 0)
    	return 0;
	
    estring_init(&e);
    
    while (*rep)
    {
    	if (rep[0] == '\\' && isdigit(rep[1]))
	{
	    estring_append_string(&e, pat->groups[rep[1]-'0']);
	    rep += 2;
	}
	else
	    estring_append_char(&e, *rep++);
    }
    
#if DEBUG
    fprintf(stderr, "pattern_replace: \"%s\" -> \"%s\"\n",
    	    replace, e.data);
#endif
    return e.data;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
