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

#if 0
typedef struct
{
    char *file;
    char *directory;
    GList *filesets;
    /* TODO: filtersets */
    gboolean verbose:1;
    gboolean quiet:1;
    gboolean fail_on_error:1;
    gboolean include_empty_dirs:1;
    gboolean result:1;
} copy_private_t;

CVSID("$Id: task_copy.c,v 1.4 2001-11-08 04:13:35 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void copy_delete(task_t *);

static gboolean
copy_parse(task_t *task, xmlNode *node)
{
    xmlNode *child;
    copy_private_t *dp;
    fileset_t *fs;
    
    dp = new(copy_private_t);
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
	copy_delete(task);
	return FALSE;
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
copy_copy_one(const char *filename, void *userdata)
{
    task_t *task = (task_t *)userdata;
    copy_private_t *dp = (copy_private_t *)task->private;
    int r;
    
    if (dp->verbose)
    	logf("%s\n", filename);
	
    /* TODO: apply proj->basedir */

    if (file_is_directory(filename) == 0)
    {
    	r = file_apply_children(filename, copy_copy_one, task);
	
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
copy_execute(task_t *task)
{
    copy_private_t *dp = (copy_private_t *)task->private;
    char *expfile;
    GList *iter;
    
    dp->result = TRUE;
    
    if (dp->file != 0)
    {
    	expfile = task_expand(task, dp->file);
    	copy_copy_one(expfile, task);
	g_free(expfile);
    }
    if (dp->directory != 0)
    {
    	expfile = task_expand(task, dp->directory);
    	copy_copy_one(expfile, task);
	g_free(expfile);
    }

    /* execute for <fileset> children */
    
    for (iter = dp->filesets ; iter != 0 ; iter = iter->next)
    {
    	fileset_t *fs = (fileset_t *)iter->data;
	
	fileset_apply(fs, copy_copy_one, task);
    }

    return dp->result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
copy_delete(task_t *task)
{
    copy_private_t *dp = (copy_private_t *)task->private;
    
    strdelete(dp->file);
    strdelete(dp->directory);
    
    /* copy filesets */
    while (dp->filesets != 0)
    {
    	fileset_delete((fileset_t *)dp->filesets->data);
    	dp->filesets = g_list_remove_link(dp->filesets, dp->filesets);
    }
    
    task->private = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*NOT*/task_ops_t copy_ops = 
{
    "copy",
    /*init*/0,
    copy_parse,
    copy_execute,
    copy_delete
};
#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
