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

#include "fileset.h"
#include "estring.h"
#include "tok.h"
#include "mapper.h"
#include "log.h"
#include <dirent.h>

CVSID("$Id: fileset.c,v 1.12 2002-02-08 07:29:25 gnb Exp $");

static void fs_spec_delete(fs_spec_t *fss);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t *
fileset_new(void)
{
    fileset_t *fs;
    
    fs = new(fileset_t);
    fs->refcount = 1;

    return fs;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
fileset_delete(fileset_t *fs)
{
    listdelete(fs->specs, fs_spec_t, fs_spec_delete);
    strdelete(fs->id);
    strdelete(fs->directory);
	
    g_free(fs);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_ref(fileset_t *fs)
{
    fs->refcount++;
}

void
fileset_unref(fileset_t *fs)
{
    if (--fs->refcount == 0)
    	fileset_delete(fs);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_set_id(fileset_t *fs, const char *s)
{
    strassign(fs->id, s);
}

void
fileset_set_directory(fileset_t *fs, const char *dir)
{
    strassign(fs->directory, dir);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static fs_spec_t *
fs_spec_add(
    fileset_t *fs,
    unsigned flags,
    const char *pattern,
    const char *filename)
{
    fs_spec_t *fss;
    
    fss = new(fs_spec_t);
    
    fss->flags = flags;
    
    if (pattern != 0)
    	pattern_init(&fss->pattern, pattern,
	    	    	(fs->case_sensitive ? PAT_CASE : 0));
    
    strassign(fss->filename, filename);
    condition_init(&fss->condition);

    fs->specs = g_list_append(fs->specs, fss);
        
    return fss;
}

static void
fs_spec_delete(fs_spec_t *fss)
{
    strdelete(fss->filename);
    pattern_free(&fss->pattern);
    condition_free(&fss->condition);

    g_free(fss);
}

fs_spec_t *
fileset_add_include(fileset_t *fs, const char *pattern)
{
    return fs_spec_add(fs, FS_INCLUDE, /*pattern*/0, pattern);
}

fs_spec_t *
fileset_add_include_file(fileset_t *fs, const char *filename)
{
    return fs_spec_add(fs, FS_INCLUDE|FS_FILE, /*pattern*/0, filename);
}

fs_spec_t *
fileset_add_exclude(fileset_t *fs, const char *pattern)
{
    return fs_spec_add(fs, FS_EXCLUDE, pattern, /*filename*/0);
}

fs_spec_t *
fileset_add_exclude_file(fileset_t *fs, const char *filename)
{
    return fs_spec_add(fs, FS_EXCLUDE|FS_FILE, /*pattern*/0, filename);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_set_default_excludes(fileset_t *fs, gboolean b)
{
    fs->default_excludes = b;
}

void
fileset_set_case_sensitive(fileset_t *fs, gboolean b)
{
    fs->case_sensitive = b;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
list_splice(GList *after, GList *list2)
{
    GList *last2;
    
    if (list2 == 0)
    	return;
	
    last2 = g_list_last(list2);

    if (after->next != 0)
	after->next->prev = last2;
    last2->next = after->next;
    
    after->next = list2;
    list2->prev = after;
}

typedef struct
{
    char *basedir;
    GList *filenames;
    GList *pending;
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
    GList *baselink)
{
    pattern_t pat;
    DIR *dir;
    struct dirent *de;
    GList *results = 0;
    char *base = (char *)baselink->data;
    char *newpath;

    if ((dir = file_opendir(*base == '\0' ? "." : base)) != 0)
    {
	pattern_init(&pat, globpart, (state->case_sens ? PAT_CASE : 0));

	while ((de = readdir(dir)) != 0)
	{
    	    if (!strcmp(de->d_name, ".") ||
		!strcmp(de->d_name, ".."))
		continue;

    	    if (pattern_match_c(&pat, de->d_name))
	    {
		newpath = (*base == '\0' ?
			    g_strdup(de->d_name) :
	    	    	    g_strconcat(base, "/", de->d_name, 0));
#if DEBUG
    		fprintf(stderr, "glob_part(\"%s\") -> \"%s\"\n", base, newpath);
#endif
		results = g_list_append(results, newpath);
	    }
	}

	closedir(dir);
	pattern_free(&pat);
    }


    list_splice(baselink, results);
    g_free(base);
    state->pending = g_list_remove_link(state->pending, baselink);
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
    	    else if (pattern_match_c(pat, newpath))
	    {
#if DEBUG
    		fprintf(stderr, "glob_recurse(\"%s\") -> \"%s\"\n",
		    	    	pat->pattern, newpath);
#endif
    	    	state->pending = g_list_append(state->pending, newpath);
	    }
	    else
	    	g_free(newpath);
	}

	closedir(dir);
    }
}

/*
 * Replace the single list item `baselink' with a list of
 * files matching the glob-directory-sequence `globseq'.
 */
static void
glob_path(
    fs_glob_state_t *state,
    char *globpath,
    GList *baselink)
{
    const char *part;
    GList *iter, *next;
    tok_t tok;
    
    tok_init(&tok, globpath, "/");

    while ((part = tok_next(&tok)) != 0)
    {
	if (!strcmp(part, "**"))
	{
	    pattern_t pat;
	    GList *oldpending;
	    
    	    pattern_init(&pat, globpath, (state->case_sens ? PAT_CASE : 0));

    	    oldpending = state->pending;
	    state->pending = 0;
	    while (oldpending != 0)
	    {
	    	char *base = (char *)oldpending->data;
		glob_recurse(state, &pat, base);
		g_free(base);
	    	oldpending = g_list_remove_link(oldpending, oldpending);
	    }
	    break;
	}
	else if (strpbrk(part, "*?[]"))
	{
	    state->check_exists = TRUE;
	    for (iter = state->pending ; iter != 0 ; iter = next)
	    {
	    	next = iter->next;
		glob_part(state, part, iter);
	    }
	}
	else
	{
	    for (iter = state->pending ; iter != 0 ; iter = next)
	    {
	    	char *fn = (char *)iter->data;
	    	next = iter->next;
		
	    	fn = g_realloc(fn, strlen(fn)+1+strlen(part)+1);
		if (*fn != '\0')
		    strcat(fn, "/");
		strcat(fn, part);
		iter->data = fn;
		
		if (state->check_exists && file_exists(fn) < 0)
		{
		    g_free(fn);
		    state->pending = g_list_remove_link(state->pending, iter);
		}
	    }
	}
    }

    tok_free(&tok);
    g_free(globpath);
}

static void
fs_include(
    fs_glob_state_t *state,
    char *glob)
{
    state->pending = g_list_append(0, g_strdup(*glob == '/' ? "/" : ""));
    state->check_exists = FALSE;
    
    glob_path(state, glob, state->pending);

    if (((char *)state->pending->data)[0] == '\0')
    {
#if DEBUG
    	fprintf(stderr, "fs_include(\"%s\") matched no files\n", glob);
#endif
    	listdelete(state->pending, char, g_free);
    }
    else
    {
#if DEBUG
    	GList *iter;
    	fprintf(stderr, "fs_include(\"%s\") adding", glob);
	for (iter = state->pending ; iter != 0 ; iter = iter->next)
	    fprintf(stderr, " \"%s\"", (char *)iter->data);
	fprintf(stderr, "\n");
#endif
	state->filenames = g_list_concat(state->filenames, state->pending);
    }
}


static void
fs_exclude(
    fs_glob_state_t *state,
    pattern_t *pat)
{
    GList *iter, *next;
    
    for (iter = state->filenames ; iter != 0 ; iter = next)
    {
    	char *fn = (char *)iter->data;
    	next = iter->next;
	
	if (pattern_match_c(pat, fn))
	{
#if DEBUG
    	    fprintf(stderr, "fs_exclude(\"%s\"): removing \"%s\"\n",
	    	    	pat->pattern, fn);
#endif
	    g_free(fn);
	    state->filenames = g_list_remove_link(state->filenames, iter);
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
	    
	    pattern_init(&pat, buf, (state->case_sens ? PAT_CASE : 0));
	    fs_exclude(state, &pat);
	    pattern_free(&pat);
	}
    }
    
    fclose(fp);
}


void
fileset_apply(
    const fileset_t *fs,
    const props_t *props,
    file_apply_proc_t func,
    void *userdata)
{
    GList *iter;
    fs_glob_state_t state;

    state.basedir = props_expand(props, fs->directory);
    state.filenames = 0;
    state.pending = 0;    
    state.case_sens = fs->case_sensitive;
    
    for (iter = fs->specs ; iter != 0 ; iter = iter->next)
    {
    	fs_spec_t *fss = (fs_spec_t *)iter->data;
	
	if (fss->flags & FS_FILE)
	    fs_apply_file(&state, fss->filename, (fss->flags & FS_INCLUDE));
	else if (fss->flags & FS_INCLUDE)
	    fs_include(&state, file_normalise(fss->filename, state.basedir));
	else
	    fs_exclude(&state, &fss->pattern);
    }
    
    g_free(state.basedir);
    
    for (iter = state.filenames ; iter != 0 ; iter = iter->next)
    {
	if (!(*func)((char*)iter->data, userdata))
	    break;
    }

    listdelete(state.filenames, char, g_free);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    GList *mappers;
    strarray_t *strarray;
} fs_gather_rec_t;

static gboolean
fileset_gather_one(const char *filename, void *userdata)
{
    fs_gather_rec_t *rec = (fs_gather_rec_t *)userdata;

    if (rec->mappers == 0)
    {
	strarray_append(rec->strarray, filename);
    }
    else
    {
    	GList *iter;
	char *mapped = 0;

	for (iter = rec->mappers ; iter != 0 ; iter = iter->next)
	{
    	    mapper_t *ma = (mapper_t *)iter->data;

	    if ((mapped = mapper_map(ma, filename)) != 0)
		break;
	}

	if (mapped != 0)
	    strarray_appendm(rec->strarray, mapped);
    }
    
    return TRUE;   /* keep going */
}

void
fileset_gather_mapped(
    const fileset_t *fs,
    const props_t *props,
    strarray_t *sa,
    GList *mappers)
{
    fs_gather_rec_t rec;
    
    rec.mappers = mappers;
    rec.strarray = sa;
    
    fileset_apply(fs, props, fileset_gather_one, &rec);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
