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

typedef struct
{
    char *file;
    char *directory;
    GList *filesets;
    gboolean verbose:1;
    gboolean quiet:1;
    gboolean fail_on_error:1;
    gboolean include_empty_dirs:1;
    gboolean result:1;
} delete_private_t;

CVSID("$Id: task_delete.C,v 1.1 2002-03-29 12:36:27 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
delete_new(task_t *task)
{
    delete_private_t *dp;

    task->private_data = dp = new(delete_private_t);
    
    /* default values */
    dp->verbose = FALSE;
    dp->quiet = FALSE;
    dp->fail_on_error = TRUE;
    dp->include_empty_dirs = FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void delete_delete(task_t *);
    
static gboolean
delete_set_file(task_t *task, const char *name, const char *value)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;

    strassign(dp->file, value);
    return TRUE;
}

static gboolean
delete_set_dir(task_t *task, const char *name, const char *value)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;

    strassign(dp->directory, value);
    return TRUE;
}
    
static gboolean
delete_set_verbose(task_t *task, const char *name, const char *value)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;

    boolassign(dp->verbose, value);
    return TRUE;
}

static gboolean
delete_set_quiet(task_t *task, const char *name, const char *value)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;

    boolassign(dp->quiet, value);
    return TRUE;
}

static gboolean
delete_set_failonerror(task_t *task, const char *name, const char *value)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;

    boolassign(dp->fail_on_error, value);
    return TRUE;
}

static gboolean
delete_set_includeEmptyDirs(task_t *task, const char *name, const char *value)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;

    boolassign(dp->include_empty_dirs, value);
    return TRUE;
}

static gboolean
delete_add_fileset(task_t *task, xmlNode *node)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;
    fileset_t *fs;

    if ((fs = parse_fileset(task->project, node, "dir")) == 0)
    	return FALSE;
	
    dp->filesets = g_list_append(dp->filesets, fs);
    return TRUE;
}

static gboolean
delete_post_parse(task_t *task)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;

    if (dp->file == 0 && dp->directory == 0 && dp->filesets == 0)
    {
    	parse_error("At least one of \"file\", \"dir\" or \"<fileset>\" must be present\n");
	return FALSE;
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
delete_delete_one(const char *filename/*unnormalised*/, void *userdata)
{
    task_t *task = (task_t *)userdata;
    delete_private_t *dp = (delete_private_t *)task->private_data;
    int r;
    
    if (dp->verbose)
    	logf("%s\n", filename);
	
    if (file_is_directory(filename) == 0)
    {
    	r = file_apply_children(filename, delete_delete_one, task);

	if (r < 0)
	{
	    log_perror(filename);
	}
	else if (r == 1)
	{
	    if (dp->include_empty_dirs && file_rmdir(filename) < 0)
	    {
	    	if (!dp->quiet)
		    log_perror(filename);
		if (dp->fail_on_error)
		    dp->result = FALSE;
    	    }
	}	    
    }
    else
    {
	if (file_unlink(filename) < 0)
	{
	    if (!dp->quiet)
		log_perror(filename);
	    if (dp->fail_on_error)
		dp->result = FALSE;
	}
    }
        
    return TRUE;    /* keep going */
}

static gboolean
delete_execute(task_t *task)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;
    char *expfile;
    GList *iter;
    
    dp->result = TRUE;
    
    if (!dp->verbose)
    	logf("\n");
    
    if (dp->file != 0)
    {
    	expfile = task_expand(task, dp->file);
    	delete_delete_one(expfile, task);
	g_free(expfile);
    }
    if (dp->directory != 0)
    {
    	expfile = task_expand(task, dp->directory);
    	delete_delete_one(expfile, task);
	g_free(expfile);
    }

    /* execute for <fileset> children */
    
    for (iter = dp->filesets ; iter != 0 ; iter = iter->next)
    {
    	fileset_t *fs = (fileset_t *)iter->data;
	
	fileset_apply(fs, project_get_props(task->project),
	    	      delete_delete_one, task);
    }

    return dp->result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
delete_delete(task_t *task)
{
    delete_private_t *dp = (delete_private_t *)task->private_data;
    
    strdelete(dp->file);
    strdelete(dp->directory);
    
    /* delete filesets */
    listdelete(dp->filesets, fileset_t, fileset_unref);
    
    task->private_data = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_attr_t delete_attrs[] = 
{
    TASK_ATTR(delete, file, 0),
    TASK_ATTR(delete, dir, 0),
    TASK_ATTR(delete, verbose, 0),
    TASK_ATTR(delete, quiet, 0),
    TASK_ATTR(delete, failonerror, 0),
    TASK_ATTR(delete, includeEmptyDirs, 0),
    {0}
};

static task_child_t delete_children[] = 
{
    TASK_CHILD(delete, fileset, 0),
    {0}
};

task_ops_t delete_ops = 
{
    "delete",
    /*init*/0,
    delete_new,
    /*set_content*/0,
    delete_post_parse,
    delete_execute,
    delete_delete,
    delete_attrs,
    delete_children,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
