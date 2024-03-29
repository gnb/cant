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

#include "pattern.H"
#include "estring.H"
#include "log.H"

CVSID("$Id: pattern.C,v 1.7 2002-04-21 03:58:33 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

pattern_t::pattern_t()
{
    memset(this, 0, sizeof(*this));
}

pattern_t::~pattern_t()
{
#if 0
    int i;

    for (i = 0 ; i < _PAT_NGROUPS ; i++)
	groups_[i] = 0;
#endif
    regfree(&regex_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
pattern_t::set_pattern(const char *pattern, unsigned flags)
{
    const char *p = pattern;
    estring restr;
    unsigned reflags = 0;
    int errcode;

    if (pattern_.data() != 0)
    {
    	int i;
    	pattern_ = (char*)0;
	for (i = 0 ; i < _PAT_NGROUPS ; i++)
	    groups_[i] = (char*)0;
    	regfree(&regex_);
    }
    
    if (pattern == 0)
	return TRUE;
        
    pattern_ = pattern;

    if (flags & PAT_REGEXP)
    {
    	restr.append_string(pattern);
	reflags |= REG_EXTENDED;
    }
    else
    {
	restr.append_char('^');

	for ( ; *p ; p++)
	{
    	    if (p[0] == '*' && p[1] == '*' &&
	    	(p == pattern || p[-1] == '/') &&
		(p[2] == '\0' || p[2] == '/'))
	    {
	    	/* A directory component consisting entirely of "**" */
		restr.append_string("\\([^/]*/\\)*");
		if (p[2] == '/')
		    p++;
		p++;
	    }
	    else if (*p == '*')
		restr.append_string("\\([^/]*\\)");
	    else if (*p == '?')
		restr.append_string("\\([^/]\\)");
	    else if (strchr(".^$\\", *p) != 0)
	    {
		restr.append_char('\\');
		restr.append_char(*p);
	    }
	    else
		restr.append_char(*p);
	}

	restr.append_char('$');
    }
        
#if DEBUG
    fprintf(stderr, "pattern_t::init: \"%s\" -> \"%s\"\n",
    	    	    	pattern, restr.data());
#endif
    if (!(flags & PAT_CASE)) reflags |= REG_ICASE;
    if (!(flags & PAT_GROUPS)) reflags |= REG_NOSUB;
    errcode = regcomp(&regex_, restr.data(), reflags);
    if (errcode != 0)
    {
	char errbuf[1024];

	regerror(errcode, &regex_, errbuf, sizeof(errbuf));
    	log::errorf("\"%s\": %s\n", pattern, errbuf);
    }
    return (errcode == 0);
}

const char *
pattern_t::get_pattern() const
{
    return pattern_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

pattern_t *
pattern_t::create(const char *pattern, unsigned flags)
{
    pattern_t *pat;
    
    pat = new pattern_t;
    
    if (!pat->set_pattern(pattern, flags))
    {
    	delete pat;
    	return 0;
    }
    
    return pat;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
pattern_t::match(const char *filename)
{
    gboolean ret;
    int i;
    regmatch_t matches[_PAT_NGROUPS];
    
    for (i = 0 ; i < _PAT_NGROUPS ; i++)
	groups_[i] = (char*)0;

    memset(matches, 0xff, sizeof(matches));
    ret = (regexec(&regex_, filename,
    	    	    _PAT_NGROUPS, matches, /*eflags*/0) == 0);
#if DEBUG
    fprintf(stderr, "pattern_t::match: pattern=\"%s\" filename=\"%s\" -> %s\n",
    	    pattern_.data(), filename, (ret ? "true" : "false"));
#endif

    if (ret)
    {
	for (i = 0 ; i < _PAT_NGROUPS && matches[i].rm_so >= 0 ; i++)
	{
	    groups_[i] = g_strndup(
	    	    	    	    filename+matches[i].rm_so,
				    matches[i].rm_eo-matches[i].rm_so);
#if DEBUG
	    fprintf(stderr, "               \\%d=\"%s\" [%d,%d]\n",
	    	    	    	i, groups_[i].data(),
				matches[i].rm_so, matches[i].rm_eo);
#endif				    
	}
    }

    return ret;
}

gboolean
pattern_t::match_c(const char *filename) const
{
    gboolean ret;
    
    ret = (regexec(&regex_, filename,
    	    	    /*nmatches*/0, /*matches*/0, /*eflags*/0) == 0);
#if DEBUG
    fprintf(stderr, "pattern_t::match_c: pattern=\"%s\" filename=\"%s\" -> %s\n",
    	    pattern_.data(), filename, (ret ? "true" : "false"));
#endif

    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
pattern_t::group(unsigned int /*0-9*/i) const
{
    return (i >= _PAT_NGROUPS ? 0 : groups_[i]);
}

char *
pattern_t::replace(const char *replace) const
{
    const char *rep = replace;
    estring e;
    
    if (rep == 0)
    	return 0;
	
    while (*rep)
    {
    	if (rep[0] == '\\' && isdigit(rep[1]))
	{
	    e.append_string(groups_[rep[1]-'0']);
	    rep += 2;
	}
	else
	    e.append_char(*rep++);
    }
    
#if DEBUG
    fprintf(stderr, "pattern_t::replace: \"%s\" -> \"%s\"\n",
    	    replace, e.data());
#endif
    return e.take();
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
