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
#include "log.h"
#include <dirent.h>

CVSID("$Id: fileset.c,v 1.9 2001-11-14 10:59:03 gnb Exp $");

typedef enum { FS_IN, FS_EX, FS_UNKNOWN } fs_result_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static fs_spec_t *
fs_spec_new(
    unsigned flags,
    const char *pattern,
    gboolean case_sens,
    const char *filename)
{
    fs_spec_t *fss;
    
    fss = new(fs_spec_t);
    
    fss->flags = flags;
    
    if (pattern != 0)
    	pattern_init(&fss->pattern, pattern, (case_sens ? PAT_CASE : 0));
    
    strassign(fss->filename, filename);
    condition_init(&fss->condition);
    
    return fss;
}

static void
fs_spec_delete(fs_spec_t *fss)
{
    /* delete child specs */
    while (fss->specs != 0)
    {
    	fs_spec_delete((fs_spec_t *)fss->specs->data);
	fss->specs = g_list_remove_link(fss->specs, fss->specs);
    }
    
    strdelete(fss->filename);
    pattern_free(&fss->pattern);
    condition_free(&fss->condition);

    g_free(fss);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static fs_result_t fs_spec_match(fileset_t *, fs_spec_t *, const char *);

static fs_result_t
fs_spec_list_match(fileset_t *fs, GList *list, const char *filename)
{
    fs_result_t res = FS_UNKNOWN;
        
    for ( ; list != 0 ; list = list->next)
    {
    	fs_spec_t *fss = (fs_spec_t *)list->data;
	
	switch (fs_spec_match(fs, fss, filename))
	{
	case FS_IN:
	    res = FS_IN;
	    break;
	case FS_EX:
	    res = FS_EX;
	    break;
	case FS_UNKNOWN:
	    break;
	}
    }

    return res;
}

static void
fs_spec_file_read(fileset_t *fs, fs_spec_t *fss)
{
    fs_spec_t *child;
    FILE *fp;
    char *x, buf[1024];
    
    /* delete any previous child specs */
    while (fss->specs != 0)
    {
    	fs_spec_delete((fs_spec_t *)fss->specs->data);
	fss->specs = g_list_remove_link(fss->specs, fss->specs);
    }
    
    fss->flags |= FS_FILEREAD;
    
    /* read the file */
    if ((fp = fopen(fss->filename, "r")) == 0)
    {
    	logperror(fss->filename);
	return;
    }
    
    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	if ((x = strchr(buf, '\n')) != 0)
	    *x = '\0';
    	if ((x = strchr(buf, '\r')) != 0)
	    *x = '\0';

    	child = fs_spec_new(fss->flags & FS_INCLUDE,
	    	    	    /*pattern*/buf, fs->case_sensitive,
			    /*filename*/0);
	fss->specs = g_list_append(fss->specs, child);
    }
    
    fclose(fp);
}


static fs_result_t
fs_spec_match(fileset_t *fs, fs_spec_t *fss, const char *filename)
{
    if (!condition_evaluate(&fss->condition, fs->props))
	    return FS_UNKNOWN;
    
    if (fss->flags & FS_FILE)
    {
    	if (!(fss->flags & FS_FILEREAD))
	    fs_spec_file_read(fs, fss);
    	return fs_spec_list_match(fs, fss->specs, filename);
    }
    
    if (!pattern_match(&fss->pattern, filename))
    	return FS_UNKNOWN;
	
    return ((fss->flags & FS_INCLUDE) ? FS_IN : FS_EX);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t *
fileset_new(props_t *props)
{
    fileset_t *fs;
    
    fs = new(fileset_t);

    fs->props = props;
    
    return fs;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_delete(fileset_t *fs)
{
    while (fs->specs != 0)
    {
    	fs_spec_t *fss = (fs_spec_t *)fs->specs->data;
	
	fs_spec_delete(fss);
	fs->specs = g_list_remove_link(fs->specs, fs->specs);
    }

    strdelete(fs->id);
    strdelete(fs->directory);
	
    g_free(fs);
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

fs_spec_t *
fileset_add_include(fileset_t *fs, const char *pattern)
{
    fs_spec_t *fss;
    
    fss = fs_spec_new(FS_INCLUDE, pattern, fs->case_sensitive, /*filename*/0);
    fs->specs = g_list_append(fs->specs, fss);
    
    return fss;
}

fs_spec_t *
fileset_add_include_file(fileset_t *fs, const char *filename)
{
    fs_spec_t *fss;
    
    fss = fs_spec_new(FS_INCLUDE|FS_FILE, /*pattern*/0, fs->case_sensitive, filename);
    fs->specs = g_list_append(fs->specs, fss);
    
    return fss;
}

fs_spec_t *
fileset_add_exclude(fileset_t *fs, const char *pattern)
{
    fs_spec_t *fss;
    
    fss = fs_spec_new(0, pattern, fs->case_sensitive, /*filename*/0);
    fs->specs = g_list_append(fs->specs, fss);
    
    return fss;
}

fs_spec_t *
fileset_add_exclude_file(fileset_t *fs, const char *filename)
{
    fs_spec_t *fss;
    
    fss = fs_spec_new(FS_FILE, /*pattern*/0, fs->case_sensitive, filename);
    fs->specs = g_list_append(fs->specs, fss);
    
    return fss;
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

static int
fileset_apply_1(
    fileset_t *fs,
    const char *filename,
    file_apply_proc_t function,
    void *userdata)
{
    DIR *dir;
    struct dirent *de;
    estring child;
    int ret = 1;
    
    if ((dir = opendir(filename)) == 0)
    	return -1;
	
    estring_init(&child);
    while ((de = readdir(dir)) != 0)
    {
    	if (!strcmp(de->d_name, ".") ||
	    !strcmp(de->d_name, ".."))
	    continue;
	    
	/* TODO: estring_truncate_to() */
	estring_truncate(&child);
	if (strcmp(filename, "."))
	{
	    estring_append_string(&child, filename);
	    estring_append_char(&child, '/');
	}
	estring_append_string(&child, de->d_name);
	
	if (file_is_directory(child.data) == 0)
	{
	    if ((ret = fileset_apply_1(fs, child.data, function, userdata)) != 1)
	    	break;
	}
	else
	{
	    /* files */
    	    if (fs_spec_list_match(fs, fs->specs, child.data) != FS_IN)
		continue;

	    if (!(*function)(child.data, userdata))
	    {
		ret = 0;
		break;
	    }
    	}
    }
    
    estring_free(&child);
    closedir(dir);
    return ret;
}

int
fileset_apply(
    fileset_t *fs,
    file_apply_proc_t func,
    void *userdata)
{
    char *expdir;
    int ret;
    GList *iter;

    /* invalidate previously read files */
    for (iter = fs->specs ; iter != 0 ; iter = iter->next)
    {
    	fs_spec_t *fss = (fs_spec_t *)iter->data;
	
	fss->flags &= ~FS_FILEREAD;
    }
    
    expdir = props_expand(fs->props, fs->directory);

    ret = fileset_apply_1(fs, expdir, func, userdata);
    
    g_free(expdir);
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
fileset_gather_one(const char *filename, void *userdata)
{
    GList **listp = (GList **)userdata;
    
    *listp = g_list_prepend(*listp, g_strdup(filename));
    return TRUE;
}

GList *
fileset_gather(fileset_t *fs)
{
    GList *list = 0;
    
    fileset_apply(fs, fileset_gather_one, &list);
    
    return g_list_reverse(list);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
