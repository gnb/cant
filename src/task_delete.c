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
    char *directory;
    GList *filesets;
    gboolean verbose:1;
    gboolean quiet:1;
    gboolean fail_on_error:1;
    gboolean include_empty_dirs:1;
    gboolean result:1;
} delete_private_t;

CVSID("$Id: task_delete.c,v 1.2 2001-11-06 09:10:30 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void delete_delete(task_t *);

static gboolean
delete_parse(task_t *task, xmlNode *node)
{
    xmlNode *child;
    delete_private_t *dp;
    fileset_t *fs;
    
    dp = g_new(delete_private_t, 1);
    if (dp == 0)
    	fatal("No memory\n");
    memset(dp, 0, sizeof(*dp));
    task->private = dp;
    
    dp->file = xml2g(xmlGetProp(node, "file"));
    dp->directory = xml2g(xmlGetProp(node, "dir"));
    
    dp->verbose = cantXmlGetBooleanProp(node, "verbose", FALSE);
    dp->quiet = cantXmlGetBooleanProp(node, "quiet", FALSE);
    dp->fail_on_error = cantXmlGetBooleanProp(node, "failonerror", TRUE);
    dp->include_empty_dirs = cantXmlGetBooleanProp(node, "includeEmptyDirs", FALSE);
    
    /* parse <fileset> children */    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	
	if (!strcmp(child->name, "fileset"))
	{
    	    if ((fs = parse_fileset(task->project, child, "dir")) != 0)
		dp->filesets = g_list_append(dp->filesets, fs);
	}
    }

    if (dp->file == 0 && dp->directory == 0 && dp->filesets == 0)
    {
    	parse_error("At least one of \"file\", \"dir\" or \"<fileset>\" must be present\n");
	delete_delete(task);
	return FALSE;
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
delete_delete_one(const char *filename, void *userdata)
{
    task_t *task = (task_t *)userdata;
    delete_private_t *dp = (delete_private_t *)task->private;
    int r;
    
    if (dp->verbose)
    	logf("%s\n", filename);
	
    /* TODO: apply proj->basedir */

    if (file_is_directory(filename) == 0)
    {
    	r = file_apply_children(filename, delete_delete_one, task);
	
	if (r < 0)
	{
	    logperror(filename);
	}
	else if (r == 1)
	{
	    if (dp->include_empty_dirs && rmdir(filename) < 0 && !dp->quiet)
	    {
		logperror(filename);
		dp->result = FALSE;
    	    }
	}	    
    }
    else
    {
	if (unlink(filename) < 0 && !dp->quiet)
	{
	    logperror(filename);
	    dp->result = FALSE;
	}
    }
        
    return TRUE;    /* keep going */
}

static gboolean
delete_execute(task_t *task)
{
    delete_private_t *dp = (delete_private_t *)task->private;
    char *expfile;
    GList *iter;
    
    dp->result = TRUE;
    
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
	
	fileset_apply(fs, delete_delete_one, task);
    }

    return dp->result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
delete_delete(task_t *task)
{
    delete_private_t *dp = (delete_private_t *)task->private;
    
    strdelete(dp->file);
    strdelete(dp->directory);
    
    /* delete filesets */
    while (dp->filesets != 0)
    {
    	fileset_delete((fileset_t *)dp->filesets->data);
    	dp->filesets = g_list_remove_link(dp->filesets, dp->filesets);
    }
    
    task->private = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_ops_t delete_ops = 
{
    "delete",
    /*init*/0,
    delete_parse,
    delete_execute,
    delete_delete
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
