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
#include "log.h"

CVSID("$Id: pattern.c,v 1.9 2002-02-08 07:32:37 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * TODO: this really, really needs a more sensible approach
 *       to error reporting, so that I don't have to declare
 *       the right function to report errors.  FMEH!!
 */
extern void parse_error(const char *fmt, ...);

gboolean
pattern_init(pattern_t *pat, const char *pattern, unsigned flags)
{
    const char *p = pattern;
    estring restr;
    unsigned reflags = 0;
    int errcode;
    
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
    	    if (p[0] == '*' && p[1] == '*' &&
	    	(p == pattern || p[-1] == '/') &&
		(p[2] == '\0' || p[2] == '/'))
	    {
	    	/* A directory component consisting entirely of "**" */
		estring_append_string(&restr, "\\([^/]*/\\)*");
		if (p[2] == '/')
		    p++;
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
    errcode = regcomp(&pat->regex, restr.data, reflags);
    if (errcode != 0)
    {
	char errbuf[1024];

	regerror(errcode, &pat->regex, errbuf, sizeof(errbuf));
	parse_error("\"%s\": %s\n", pattern, errbuf);
    }
    estring_free(&restr);
    memset(&pat->groups, 0, sizeof(pat->groups));
    return (errcode == 0);
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
    
    if (!pattern_init(pat, pattern, flags))
    {
    	pattern_delete(pat);
    	return 0;
    }
    
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

gboolean
pattern_match_c(const pattern_t *pat, const char *filename)
{
    gboolean ret;
    
    ret = (regexec(&pat->regex, filename,
    	    	    /*nmatches*/0, /*matches*/0, /*eflags*/0) == 0);
#if DEBUG
    fprintf(stderr, "pattern_match_c: pattern=\"%s\" filename=\"%s\" -> %s\n",
    	    pat->pattern, filename, (ret ? "true" : "false"));
#endif

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
