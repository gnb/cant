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
#include "tok.H"
#include <time.h>

CVSID("$Id: task_foreach.C,v 1.1 2002-03-29 12:36:27 gnb Exp $");

typedef struct
{
    char *variable;
    /* TODO: whitespace-safe technique */
    char *values;
    GList *filesets;
    /* TODO: support nested <property> tags */

    char *variable_e;
    gboolean failed:1;
} foreach_private_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
foreach_new(task_t *task)
{
    task->private_data = new(foreach_private_t);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
foreach_set_variable(task_t *task, const char *name, const char *value)
{
    foreach_private_t *fp = (foreach_private_t *)task->private_data;
    
    strassign(fp->variable, value);
    return TRUE;
}

static gboolean
foreach_set_values(task_t *task, const char *name, const char *value)
{
    foreach_private_t *fp = (foreach_private_t *)task->private_data;

    strassign(fp->values, value);
    return TRUE;
}

static gboolean
foreach_add_fileset(task_t *task, xmlNode *node)
{
    foreach_private_t *fp = (foreach_private_t *)task->private_data;
    fileset_t *fs;
    
    if ((fs = parse_fileset(task->project, node, "dir")) == 0)
    	return FALSE;

    fp->filesets = g_list_append(fp->filesets, fs);
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
foreach_do_iteration(const char *val, void *userdata)
{
    task_t *task = (task_t *)userdata;
    foreach_private_t *fp = (foreach_private_t *)task->private_data;

    /*
     * TODO: need a props stack... (per-task?) to get scope right.
     * For now, this variable goes into the project scope.
     */
    props_set(task->project->properties, fp->variable_e, val);
    if (verbose)
	logf("%s = %s\n", fp->variable_e, val);

    if (!task_execute_subtasks(task))
    {
	fp->failed = TRUE;
	return FALSE;
    }
    return TRUE;
}

static gboolean
foreach_execute(task_t *task)
{
    foreach_private_t *fp = (foreach_private_t *)task->private_data;
    const char *val;
    GList *iter;
    tok_t tok;

    fp->failed = FALSE;    
    fp->variable_e = task_expand(task, fp->variable);

    tok_init_m(&tok, task_expand(task, fp->values), ",");
    while (!fp->failed && (val = tok_next(&tok)) != 0)
    	foreach_do_iteration(val, task);
    tok_free(&tok);
    
    for (iter = fp->filesets ; !fp->failed && iter != 0 ; iter = iter->next)
    {
    	fileset_t *fs = (fileset_t *)iter->data;
	
	fileset_apply(fs, project_get_props(task->project),
			foreach_do_iteration, task);
    }

    g_free(fp->variable_e);

    return !fp->failed;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
foreach_delete(task_t *task)
{
    foreach_private_t *fp = (foreach_private_t *)task->private_data;
    
    strdelete(fp->variable);
    strdelete(fp->values);
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
    TASK_CHILD(foreach, fileset, 0),
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
    /*fileset_dir_name*/0,
    /*cleanup*/0,
    /*is_composite*/TRUE
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
