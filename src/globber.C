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

#include "globber.H"
#include "tok.H"
#include "log.H"
#include <dirent.h>

CVSID("$Id: globber.C,v 1.1 2002-04-07 06:22:51 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

globber_t::globber_t(const char *basedir, gboolean case_sensitive)
{
    memset(this, 0, sizeof(*this));
    strassign(basedir_, basedir);
    case_sensitive_ = case_sensitive;
}

globber_t::~globber_t()
{
    strdelete(basedir_);
    filenames_.apply_remove(strfree);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Replace the single list item `baselink' with a list of
 * files matching the glob-directory-component `globpart'.
 */
void
globber_t::glob_part(
    const char *globpart,
    list_iterator_t<char> baselink)
{
    pattern_t pat;
    DIR *dir;
    struct dirent *de;
    list_t<char> results;
    char *base = *baselink;
    char *newpath;

    if ((dir = file_opendir(*base == '\0' ? "." : base)) != 0)
    {
	pat.set_pattern(globpart, (case_sensitive_ ? PAT_CASE : 0));

	while ((de = readdir(dir)) != 0)
	{
    	    if (!strcmp(de->d_name, ".") ||
		!strcmp(de->d_name, ".."))
		continue;

    	    if (pat.match_c(de->d_name))
	    {
		newpath = (*base == '\0' ?
			    g_strdup(de->d_name) :
	    	    	    g_strconcat(base, "/", de->d_name, 0));
#if DEBUG
    		fprintf(stderr, "globber_t::glob_part(\"%s\", \"%s\") -> \"%s\"\n",
		    	base, globpart, newpath);
#endif
		results.append(newpath);
	    }
	}

	closedir(dir);
    }


    baselink.splice_after(&results);
    g_free(base);
    pending_.remove(baselink);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
globber_t::recurse(
    const pattern_t *pat,
    const char *base)
{
    DIR *dir;
    struct dirent *de;
    char *newpath;

    if ((dir = file_opendir(*base == '\0' ? "." : base)) != 0)
    {
	while ((de = readdir(dir)) != 0)
	{
    	    if (!strcmp(de->d_name, ".") ||
		!strcmp(de->d_name, ".."))
		continue;

	    newpath = (*base == '\0' ?
			g_strdup(de->d_name) :
	    	    	g_strconcat(base, "/", de->d_name, 0));

	    if (file_is_directory(newpath) == 0)
	    {
	    	recurse(pat, newpath);
		g_free(newpath);
	    }
    	    else if (pat->match_c(newpath))
	    {
#if DEBUG
    		fprintf(stderr, "globber_t::recurse(\"%s\") -> \"%s\"\n",
		    	    	pat->get_pattern(), newpath);
#endif
    	    	pending_.append(newpath);
	    }
	    else
	    	g_free(newpath);
	}

	closedir(dir);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Update all of the pending list with  files matching
 * the glob-directory-sequence `globpath'.  Writes into
 * then frees its argument.
 */
void
globber_t::glob_path_m(char *globpath)
{
    const char *part;
    list_iterator_t<char> iter, next;
    tok_t tok((const char *)globpath, "/");

    while ((part = tok.next()) != 0)
    {
	if (!strcmp(part, "**"))
	{
	    pattern_t pat;
	    list_t<char> oldpending;
	    
    	    pat.set_pattern(globpath, (case_sensitive_ ? PAT_CASE : 0));

    	    oldpending.take(&pending_);
	    char *base;
	    while ((base = oldpending.remove_head()) != 0)
	    {
		recurse(&pat, base);
		g_free(base);
	    }
	    break;
	}
	else if (strpbrk(part, "*?[]"))
	{
	    check_exists_ = TRUE;
	    for (iter = pending_.first() ; iter != 0 ; iter = next)
	    {
	    	next = iter.peek_next();
		glob_part(part, iter);
	    }
	}
	else
	{
	    for (iter = pending_.first() ; iter != 0 ; iter = next)
	    {
	    	char *fn = *iter;
	    	next = iter.peek_next();
		
	    	fn = (char *)g_realloc(fn, strlen(fn)+1+strlen(part)+1);
		if (*fn != '\0')
		    strcat(fn, "/");
		strcat(fn, part);
		iter.replace(fn);
		
		if (check_exists_ && file_exists(fn) < 0)
		{
		    g_free(fn);
		    pending_.remove(iter);
		}
	    }
	}
    }

    g_free(globpath);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
globber_t::include(const char *globc)
{
    char *glob = file_normalise(globc, basedir_);

    assert(pending_.head() == 0);
    pending_.append(g_strdup(*glob == '/' ? "/" : ""));
    check_exists_ = FALSE;
    
    glob_path_m(glob);

    char *h = pending_.head();
    if (h == 0 || h[0] == '\0')
    {
#if DEBUG
    	fprintf(stderr, "globber_t::include(\"%s\") matched no files\n", globc);
#endif
    	pending_.apply_remove(strfree);
    }
    else
    {
#if DEBUG
    	list_iterator_t<char> iter;
    	fprintf(stderr, "globber_t::include(\"%s\") adding", globc);
	for (iter = pending_.first() ; iter != 0 ; ++iter)
	    fprintf(stderr, " \"%s\"", *iter);
	fprintf(stderr, "\n");
#endif
	filenames_.concat(&pending_);
    }
    assert(pending_.head() == 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
globber_t::exclude(const pattern_t *pat)
{
    list_iterator_t<char> iter, next;
    
    for (iter = filenames_.first() ; iter != 0 ; iter = next)
    {
    	char *fn = *iter;
    	next = iter.peek_next();
	
	if (pat->match_c(fn))
	{
#if DEBUG
    	    fprintf(stderr, "globber_t::exclude(\"%s\"): removing \"%s\"\n",
	    	    	pat->get_pattern(), fn);
#endif
	    g_free(fn);
	    filenames_.remove(iter);
	}
    }
}

void
globber_t::exclude(const char *glob)
{
    pattern_t pat;

    pat.set_pattern(glob, (case_sensitive_ ? PAT_CASE : 0));
    exclude(&pat);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
globber_t::apply_file(const char *pattfile, gboolean include_flag)
{
    FILE *fp;
    char *x, buf[1024];
        
    /* read the file */
    if ((fp = fopen(pattfile, "r")) == 0)
    {
    	log_perror(pattfile);
	return;
    }
    
    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	if ((x = strchr(buf, '\n')) != 0)
	    *x = '\0';
    	if ((x = strchr(buf, '\r')) != 0)
	    *x = '\0';

    	if (include_flag)
	    include(buf);
	else
	    exclude(buf);
    }
    
    fclose(fp);
}

void
globber_t::include_file(const char *pattfile)
{
    apply_file(pattfile, /*include*/TRUE);
}

void
globber_t::exclude_file(const char *pattfile)
{
    apply_file(pattfile, /*include*/FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
