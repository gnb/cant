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

typedef struct
{
    char *file;
    char *tofile;
    char *todir;
    GList *filesets;	    	/* list of fileset_t */
    GList *filtersets;	    	/* list of filterset_t */
    mapper_t *mapper;
    gboolean preserve_last_modified:1;
    gboolean overwrite:1;
    gboolean filtering:1;   	/* whether to apply *global* filters */
    gboolean flatten:1;
    gboolean include_empty_dirs:1;

    gboolean result:1;
    char *exp_todir;
    unsigned ncopied;
} copy_private_t;

CVSID("$Id: task_copy.c,v 1.5 2001-11-10 03:17:24 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
copy_new(task_t *task)
{
    copy_private_t *cp;
    
    cp = new(copy_private_t);
    
    cp->preserve_last_modified = FALSE;
    cp->overwrite = FALSE;
    cp->filtering = FALSE;
    cp->flatten = FALSE;
    cp->include_empty_dirs = TRUE;

    task->private = cp;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
copy_set_file(task_t *task, const char *name, const char *value)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    strassign(cp->file, value);
    return TRUE;
}

static gboolean
copy_set_preservelastmodified(task_t *task, const char *name, const char *value)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    boolassign(cp->preserve_last_modified, value);
    return TRUE;
}

static gboolean
copy_set_tofile(task_t *task, const char *name, const char *value)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    strassign(cp->tofile, value);
    return TRUE;
}

static gboolean
copy_set_todir(task_t *task, const char *name, const char *value)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    strassign(cp->todir, value);
    return TRUE;
}

static gboolean
copy_set_overwrite(task_t *task, const char *name, const char *value)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    boolassign(cp->overwrite, value);
    return TRUE;
}

static gboolean
copy_set_filtering(task_t *task, const char *name, const char *value)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    boolassign(cp->filtering, value);
    return TRUE;
}

static gboolean
copy_set_flatten(task_t *task, const char *name, const char *value)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    boolassign(cp->flatten, value);
    return TRUE;
}

static gboolean
copy_set_includeEmptyDirs(task_t *task, const char *name, const char *value)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    boolassign(cp->include_empty_dirs, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
copy_add_fileset(task_t *task, xmlNode *node)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    fileset_t *fs;

    if ((fs = parse_fileset(task->project, node, "dir")) == 0)
    	return FALSE;
	
    cp->filesets = g_list_append(cp->filesets, fs);
    return TRUE;
}

#if 0
static gboolean
copy_add_filterset(task_t *task, xmlNode *node)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    fileset_t *fs;

    if ((fs = parse_fileset(task->project, node, "dir")) == 0)
    	return FALSE;

    cp->filesets = g_list_append(cp->filesets, fs);
    return TRUE;
}
#endif

static gboolean
copy_add_mapper(task_t *task, xmlNode *node)
{
    copy_private_t *cp = (copy_private_t *)task->private;

    if (cp->mapper != 0)
    {
    	parse_error("Only a single \"mapper\" child may be used\n");
	return FALSE;
    }
    
    if ((cp->mapper = parse_mapper(task->project, node)) == 0)
    	return FALSE;
	
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
copy_post_parse(task_t *task)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    if (cp->file != 0)
    {
    	if (cp->tofile == 0 && cp->todir == 0)
	{
	    parse_error("One of \"tofile\" or \"todir\" must be specified with \"file\"\n");
	    return FALSE;
	}
    }
    else if (cp->filesets != 0)
    {
    	if (cp->tofile != 0)
	{
	    parse_error("Only \"todir\" is allowed with \"filesets\"\n");
	    return FALSE;
	}
    	if (cp->todir == 0)
	{
	    parse_error("Must use \"todir\" is \"filesets\"\n");
	    return FALSE;
	}
    }
    else if (cp->file == 0 && cp->filesets == 0)
    {
    	parse_error("At least one of \"file\" or \"<fileset>\" must be present\n");
	return FALSE;
    }

    if (cp->mapper == 0)
    	cp->mapper = mapper_new("identity", 0, 0);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: filtersets */
/* TODO: preserve mod times */
static gboolean
do_copy_file(copy_private_t *cp, const char *fromfile, const char *tofile)
{
    FILE *fromfp, *tofp;
    mode_t mode;
    char *todir;
    int n;
    char buf[1024];
    
    todir = file_dirname(fromfile);
    if ((mode = file_mode(todir)) < 0)
    	mode = 0755;
    g_free(todir);
    
    todir = file_dirname(tofile);
    if (file_build_tree(todir, mode) < 0)
    {
    	g_free(todir);
	return FALSE;
    }
    g_free(todir);
    
    if ((fromfp = fopen(fromfile, "r")) == 0)
    {
    	logperror(fromfile);
	return FALSE;
    }

    if ((mode = file_mode(fromfile)) == 0)
    	mode = 0644;	    /* should never happen */
    if ((tofp = file_open_mode(tofile, "w", mode)) == 0)
    {
    	logperror(tofile);
	fclose(fromfp);
	return FALSE;
    }
    
    while ((n = fread(buf, 1, sizeof(buf), fromfp)) > 0)
    	fwrite(buf, 1, n, tofp);
	
    fclose(fromfp);
    fclose(tofp);
    
    return TRUE;
}


static gboolean
copy_copy_one(const char *filename, void *userdata)
{
    task_t *task = (task_t *)userdata;
    copy_private_t *cp = (copy_private_t *)task->private;
    const char *fromfile;
    char *tofile, *mappedfile;
    
    
    /* construct the target filename */
    if (cp->flatten)
    	fromfile = file_basename_c(filename);
    else
    	fromfile = filename;
	
    if (cp->tofile != 0)
    	tofile = task_expand(task, cp->tofile);
    else if (cp->exp_todir != 0)
	tofile = g_strconcat(cp->exp_todir, "/", filename, 0);
    else
    {
	cp->result = FALSE; 	/* failed */
	g_free(tofile);
	return FALSE;	    	/* stop iteration */
    }
    if ((mappedfile = mapper_map(cp->mapper, tofile)) == 0)
    {
	g_free(tofile);
	return TRUE;	    	/* keep going */
    }
        
    logf("%s -> %s\n", filename, mappedfile);
	
    /* TODO: apply proj->basedir */

    if (file_is_directory(filename) == 0)
    {
    	int r;
	unsigned ncopied = cp->ncopied;
	
	r = file_apply_children(filename, copy_copy_one, userdata);
	
	if (r < 0)
	{
	    logperror(filename);
	}
	else if (ncopied == cp->ncopied && cp->include_empty_dirs)
	{
	    /* no files copied underneath this directory -- it must be empty */
	    /* TODO: sensible mode from original directory */
	    /* TODO: control uid,gid */
	    if (file_build_tree(mappedfile, 0755))
	    {
		logperror(filename);
		cp->result = FALSE;
    	    }
	}
    }
    else
    {
	if (!do_copy_file(cp, filename, mappedfile))
	    cp->result = FALSE;
    }
        
    return cp->result;    /* keep going if we didn't fail */
}

static gboolean
copy_execute(task_t *task)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    GList *iter;
    
    cp->result = TRUE;
    cp->ncopied = 0;
    
    cp->exp_todir = task_expand(task, cp->todir);

    if (cp->file != 0)
    {
    	char *expfile = task_expand(task, cp->file);
    	copy_copy_one(expfile, task);
	g_free(expfile);
    }

    /* execute for <fileset> children */
    
    for (iter = cp->filesets ; iter != 0 ; iter = iter->next)
    {
    	fileset_t *fs = (fileset_t *)iter->data;
	
	fileset_apply(fs, copy_copy_one, task);
    }
    
    strdelete(cp->exp_todir);

    return cp->result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
copy_delete(task_t *task)
{
    copy_private_t *cp = (copy_private_t *)task->private;
    
    strdelete(cp->file);
    strdelete(cp->tofile);
    strdelete(cp->todir);

    if (cp->mapper != 0)    
	mapper_delete(cp->mapper);
    
    /* delete filesets */
    while (cp->filesets != 0)
    {
    	fileset_delete((fileset_t *)cp->filesets->data);
    	cp->filesets = g_list_remove_link(cp->filesets, cp->filesets);
    }

#if 0
    /* delete filtersets */
    while (dp->filtersets != 0)
    {
    	filterset_delete((filterset_t *)dp->filtersets->data);
    	dp->filtersets = g_list_remove_link(dp->filtersets, dp->filtersets);
    }
#endif
    
    task->private = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_attr_t copy_attrs[] =
{
    TASK_ATTR(copy, file, 0),
    TASK_ATTR(copy, preservelastmodified, 0),
    TASK_ATTR(copy, tofile, 0),
    TASK_ATTR(copy, todir, 0),
    TASK_ATTR(copy, overwrite, 0),
    TASK_ATTR(copy, filtering, 0),
    TASK_ATTR(copy, flatten, 0),
    TASK_ATTR(copy, includeEmptyDirs, 0),
    {0}
};

static task_child_t copy_children[] = 
{
    TASK_CHILD(copy, fileset, 0),
    TASK_CHILD(copy, mapper, 0),
/*    TASK_CHILD(copy, filterset, 0), */
    {0}
};

task_ops_t copy_ops = 
{
    "copy",
    /*init*/0,
    copy_new,
    copy_post_parse,
    copy_execute,
    copy_delete,
    copy_attrs,
    copy_children,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
