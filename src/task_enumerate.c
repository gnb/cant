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

CVSID("$Id: task_enumerate.c,v 1.2 2001-11-06 09:10:30 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
enumerate_parse(task_t *task, xmlNode *node)
{
    xmlNode *child;
    fileset_t *fs;
    GList *list = 0;
    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	    
	if (!strcmp(child->name, "fileset"))
	{
	    if ((fs = parse_fileset(task->project, child, "dir")) != 0)
	    	list = g_list_append(list, fs);
	}
    }
    
    task->private = list;

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
enumerate_one_file(const char *filename, void *userdata)
{
    logf("%s\n", filename);
    return TRUE;    /* keep going */
}

static gboolean
enumerate_execute(task_t *task)
{
    GList *list = task->private;
    
    for ( ; list != 0 ; list = list->next)
    {
    	fileset_t *fs = (fileset_t *)list->data;
	
	logf("-->\n");
	fileset_apply(fs, enumerate_one_file, 0);
	logf("<--\n");
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
enumerate_delete(task_t *task)
{
    GList *list = task->private;
    
    while (list != 0)
    {
    	fileset_delete((fileset_t *)list->data);
    	list = g_list_remove_link(list, list);
    }

    task->private = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_ops_t enumerate_ops = 
{
    "enumerate",
    /*init*/0,
    enumerate_parse,
    enumerate_execute,
    enumerate_delete
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
