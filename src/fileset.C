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

#include "fileset.H"
#include "estring.H"
#include "tok.H"
#include "mapper.H"
#include "log.H"
#include <dirent.h>

CVSID("$Id: fileset.C,v 1.8 2002-04-07 05:28:50 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t::fileset_t()
{
    refcount_ = 1;
    directory_ = g_strdup(".");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t::~fileset_t()
{
    specs_.delete_all();
    strdelete(id_);
    strdelete(directory_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_t::ref()
{
    refcount_++;
}

void
fileset_t::unref()
{
    if (--refcount_ == 0)
    	delete this;
}

void
unref(fileset_t *fs)
{
    fs->unref();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_t::set_id(const char *s)
{
    strassign(id_, s);
}

void
fileset_t::set_directory(const char *dir)
{
    strassign(directory_, dir);
}

void
fileset_t::set_default_excludes(gboolean b)
{
    default_excludes_ = b;
}

void
fileset_t::set_case_sensitive(gboolean b)
{
    case_sensitive_ = b;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t::spec_t *
fileset_t::spec_add(
    unsigned flags,
    const char *pattern,
    const char *filename)
{
    spec_t *fss;
    
    fss = new spec_t;
    
    fss->flags_ = flags;
    
    if (pattern != 0)
    	fss->pattern_.set_pattern(pattern,
	    	    	(case_sensitive_ ? PAT_CASE : 0));
    
    strassign(fss->filename_, filename);

    specs_.append(fss);
        
    return fss;
}

fileset_t::spec_t::~spec_t()
{
    strdelete(filename_);
}

fileset_t::spec_t *
fileset_t::add_include(const char *pattern)
{
    return spec_add(FS_INCLUDE, /*pattern*/0, pattern);
}

fileset_t::spec_t *
fileset_t::add_include_file(const char *filename)
{
    return spec_add(FS_INCLUDE|FS_FILE, /*pattern*/0, filename);
}

fileset_t::spec_t *
fileset_t::add_exclude(const char *pattern)
{
    return spec_add(FS_EXCLUDE, pattern, /*filename*/0);
}

fileset_t::spec_t *
fileset_t::add_exclude_file(const char *filename)
{
    return spec_add(FS_EXCLUDE|FS_FILE, /*pattern*/0, filename);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    char *basedir;
    list_t<char> filenames;
    list_t<char> pending;
    gboolean case_sens;
    gboolean check_exists;
} fs_glob_state_t;

/*
 * Replace the single list item `baselink' with a list of
 * files matching the glob-directory-component `globpart'.
 */
static void
glob_part(
    fs_glob_state_t *state,
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
	pat.set_pattern(globpart, (state->case_sens ? PAT_CASE : 0));

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
    		fprintf(stderr, "glob_part(\"%s\", \"%s\") -> \"%s\"\n",
		    	base, globpart, newpath);
#endif
		results.append(newpath);
	    }
	}

	closedir(dir);
    }


    baselink.splice_after(&results);
    g_free(base);
    state->pending.remove(baselink);
}


static void
glob_recurse(
    fs_glob_state_t *state,
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
	    	glob_recurse(state, pat, newpath);
		g_free(newpath);
	    }
    	    else if (pat->match_c(newpath))
	    {
#if DEBUG
    		fprintf(stderr, "glob_recurse(\"%s\") -> \"%s\"\n",
		    	    	pat->pattern, newpath);
#endif
    	    	state->pending.append(newpath);
	    }
	    else
	    	g_free(newpath);
	}

	closedir(dir);
    }
}

/*
 * Update all of the pending list with  files matching
 * the glob-directory-sequence `globpath'.
 */
static void
glob_path(
    fs_glob_state_t *state,
    char *globpath)
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
	    
    	    pat.set_pattern(globpath, (state->case_sens ? PAT_CASE : 0));

    	    oldpending.take(&state->pending);
	    char *base;
	    while ((base = oldpending.remove_head()) != 0)
	    {
		glob_recurse(state, &pat, base);
		g_free(base);
	    }
	    // TODO: is `pat' leaked here?
	    break;
	}
	else if (strpbrk(part, "*?[]"))
	{
	    state->check_exists = TRUE;
	    for (iter = state->pending.first() ; iter != 0 ; iter = next)
	    {
	    	next = iter.peek_next();
		glob_part(state, part, iter);
	    }
	}
	else
	{
	    for (iter = state->pending.first() ; iter != 0 ; iter = next)
	    {
	    	char *fn = *iter;
	    	next = iter.peek_next();
		
	    	fn = (char *)g_realloc(fn, strlen(fn)+1+strlen(part)+1);
		if (*fn != '\0')
		    strcat(fn, "/");
		strcat(fn, part);
		iter.replace(fn);
		
		if (state->check_exists && file_exists(fn) < 0)
		{
		    g_free(fn);
		    state->pending.remove(iter);
		}
	    }
	}
    }

    g_free(globpath);
}

static void
fs_include(
    fs_glob_state_t *state,
    char *glob)
{
#if DEBUG
    char *glob_saved = g_strdup(glob);
#endif

    assert(state->pending.head() == 0);
    state->pending.append(g_strdup(*glob == '/' ? "/" : ""));
    state->check_exists = FALSE;
    
    glob_path(state, glob);

    char *h = state->pending.head();
    if (h == 0 || h[0] == '\0')
    {
#if DEBUG
    	fprintf(stderr, "fs_include(\"%s\") matched no files\n", glob_saved);
#endif
    	state->pending.apply_remove(strfree);
    }
    else
    {
#if DEBUG
    	list_iterator_t<char> iter;
    	fprintf(stderr, "fs_include(\"%s\") adding", glob_saved);
	for (iter = state->pending.first() ; iter != 0 ; ++iter)
	    fprintf(stderr, " \"%s\"", *iter);
	fprintf(stderr, "\n");
#endif
	state->filenames.concat(&state->pending);
    }
    assert(state->pending.head() == 0);

#if DEBUG
    g_free(glob_saved);
#endif
}


static void
fs_exclude(
    fs_glob_state_t *state,
    pattern_t *pat)
{
    list_iterator_t<char> iter, next;
    
    for (iter = state->filenames.first() ; iter != 0 ; iter = next)
    {
    	char *fn = *iter;
    	next = iter.peek_next();
	
	if (pat->match_c(fn))
	{
#if DEBUG
    	    fprintf(stderr, "fs_exclude(\"%s\"): removing \"%s\"\n",
	    	    	pat->pattern, fn);
#endif
	    g_free(fn);
	    state->filenames.remove(iter);
	}
    }
}

static void
fs_apply_file(
    fs_glob_state_t *state,
    const char *pattfile,
    gboolean include)
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

    	if (include)
	    fs_include(state, file_normalise_m(buf, state->basedir));
	else
	{
	    pattern_t pat;
	    
	    pat.set_pattern(buf, (state->case_sens ? PAT_CASE : 0));
	    fs_exclude(state, &pat);
	}
    }
    
    fclose(fp);
}


void
fileset_t::apply(
    const props_t *props,
    file_apply_proc_t func,
    void *userdata) const
{
    list_iterator_t<spec_t> iter;
    list_iterator_t<char> fniter;
    fs_glob_state_t state;

    state.basedir = props->expand(directory_);
    state.case_sens = case_sensitive_;
    
    for (iter = specs_.first() ; iter != 0 ; ++iter )
    {
    	spec_t *fss = *iter;
	
	if (!fss->condition_.evaluate(props))
	    continue;

	if (fss->flags_ & FS_FILE)
	    fs_apply_file(&state, fss->filename_, (fss->flags_ & FS_INCLUDE));
	else if (fss->flags_ & FS_INCLUDE)
	    fs_include(&state, file_normalise(fss->filename_, state.basedir));
	else
	    fs_exclude(&state, &fss->pattern_);
    }
    
    g_free(state.basedir);
    
    for (fniter = state.filenames.first() ; fniter != 0 ; ++fniter)
    {
	if (!(*func)(*fniter, userdata))
	    break;
    }

    state.filenames.apply_remove(strfree);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    const list_t<mapper_t> *mappers;
    strarray_t *strarray;
} fs_gather_rec_t;

static gboolean
fileset_gather_one(const char *filename, void *userdata)
{
    fs_gather_rec_t *rec = (fs_gather_rec_t *)userdata;

    if (rec->mappers == 0)
    {
	rec->strarray->append(filename);
    }
    else
    {
    	list_iterator_t<mapper_t> iter;
	char *mapped = 0;

	for (iter = rec->mappers->first() ; iter != 0 ; ++iter)
	{
	    if ((mapped = (*iter)->map(filename)) != 0)
		break;
	}

	if (mapped != 0)
	    rec->strarray->appendm(mapped);
    }
    
    return TRUE;   /* keep going */
}

void
fileset_t::gather_mapped(
    const props_t *props,
    strarray_t *sa,
    const list_t<mapper_t> *mappers) const
{
    fs_gather_rec_t rec;
    
    rec.mappers = mappers;
    rec.strarray = sa;
    
    apply(props, fileset_gather_one, &rec);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
