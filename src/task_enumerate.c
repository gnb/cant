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

#include "cant.h"

CVSID("$Id: task_enumerate.c,v 1.9 2002-02-08 07:44:04 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
enumerate_new(task_t *task)
{
    task->private = 0;	/* GList* of fileset_t */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
enumerate_add_fileset(task_t *task, xmlNode *node)
{
    GList **listp = (GList **)&task->private;
    fileset_t *fs;
    
    if ((fs = parse_fileset(task->project, node, "dir")) == 0)
    	return FALSE;

    *listp = g_list_append(*listp, fs);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
enumerate_execute(task_t *task)
{
    GList *list = (GList *)task->private;
    
    for ( ; list != 0 ; list = list->next)
    {
    	fileset_t *fs = (fileset_t *)list->data;
	strarray_t *sa = strarray_new();
	int i;
	
	fileset_gather_mapped(fs, project_get_props(task->project), sa, 0);
    	strarray_sort(sa, 0);
	
	logf("{\n");
	for (i = 0 ; i < sa->len ; i++)
	    logf("%s\n", strarray_nth(sa, i));
	logf("}\n");
	
	strarray_delete(sa);
    }
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
enumerate_delete(task_t *task)
{
    GList *list = (GList *)task->private;
    
    listdelete(list, fileset_t, fileset_unref);

    task->private = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_child_t enumerate_children[] = 
{
    TASK_CHILD(enumerate, fileset, 0),
    {0}
};

task_ops_t enumerate_ops = 
{
    "enumerate",
    /*init*/0,
    enumerate_new,
    /*set_content*/0,
    /*post_parse*/0,
    enumerate_execute,
    enumerate_delete,
    /*attrs*/0,
    enumerate_children,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
