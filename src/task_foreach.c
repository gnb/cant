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
#include "tok.h"
#include <time.h>

CVSID("$Id: task_foreach.c,v 1.1 2002-02-08 07:46:08 gnb Exp $");

typedef struct
{
    char *variable;
    /* TODO: whitespace-safe technique */
    char *values;
    GList *subtasks;
    /* TODO: support nested <property> tags */
} foreach_private_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
foreach_new(task_t *task)
{
    task->private = new(foreach_private_t);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
foreach_set_variable(task_t *task, const char *name, const char *value)
{
    foreach_private_t *fp = (foreach_private_t *)task->private;
    
    strassign(fp->variable, value);
    return TRUE;
}

static gboolean
foreach_set_values(task_t *task, const char *name, const char *value)
{
    foreach_private_t *fp = (foreach_private_t *)task->private;

    strassign(fp->values, value);
    return TRUE;
}

static gboolean
foreach_add_subtask(task_t *task, xmlNode *node)
{
    foreach_private_t *fp = (foreach_private_t *)task->private;
    task_t *subtask;
    
    if ((subtask = parse_task(task->project, node)) == 0)
    	return FALSE;

    fp->subtasks = g_list_append(fp->subtasks, subtask);
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
foreach_execute(task_t *task)
{
    foreach_private_t *fp = (foreach_private_t *)task->private;
    const char *val;
    char *var_e;
    GList *iter;
    gboolean ret = TRUE;
    tok_t tok;
    
    var_e = task_expand(task, fp->variable);
    tok_init_m(&tok, task_expand(task, fp->values), ",");
    
    while (ret && (val = tok_next(&tok)) != 0)
    {
	/*
	 * TODO: need a props stack... (per-task?) to get scope right.
	 * For now, this variable goes into the project scope.
	 */
	props_set(task->project->properties, var_e, val);
	
	for (iter = fp->subtasks ; iter != 0 ; iter = iter->next)
	{
	    task_t *subtask = (task_t *)iter->data;
	    
	    /* This really should have happened at parse time */
	    subtask->target = task->target;
	    
	    if (!task_execute(subtask))
	    {
	    	ret = FALSE;
		break;
	    }
	}
    }

    g_free(var_e);
    tok_free(&tok);

    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
foreach_delete(task_t *task)
{
    foreach_private_t *fp = (foreach_private_t *)task->private;
    
    strdelete(fp->variable);
    strdelete(fp->values);
    listdelete(fp->subtasks, task_t, task_delete);
    g_free(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_attr_t foreach_attrs[] = 
{
    TASK_ATTR(foreach, variable, 0),
    TASK_ATTR(foreach, values, 0),
    {0}
};

static task_child_t foreach_children[] = 
{
    TASK_GENERIC_CHILD(foreach, subtask, 0),
    {0}
};

task_ops_t foreach_ops = 
{
    "foreach",
    /*init*/0,
    foreach_new,
    /*set_content*/0,
    /*post_parse*/0,
    foreach_execute,
    foreach_delete,
    foreach_attrs,
    foreach_children,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
