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

CVSID("$Id: task.c,v 1.9 2001-11-16 05:22:37 gnb Exp $");

task_scope_t *tscope_builtins;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_t *
task_new(void)
{
    task_t *task;
    
    task = new(task_t);
    
    return task;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_delete(task_t *task)
{
    if (task->ops != 0 && task->ops->delete != 0)
    	(*task->ops->delete)(task);
    strdelete(task->id);
	
    g_free(task);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_set_id(task_t *task, const char *s)
{
    strassign(task->id, s);
}

void
task_set_name(task_t *task, const char *s)
{
    strassign(task->name, s);
}

void
task_set_description(task_t *task, const char *s)
{
    strassign(task->description, s);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
task_set_attribute(task_t *task, const char *name, const char *value)
{
    task_attr_t *ta = 0;
    
    if (task->ops->attrs_hashed != 0)
	ta = (task_attr_t *)g_hash_table_lookup(task->ops->attrs_hashed, name);

    if (ta == 0)
    	return FALSE;

    return (*ta->setter)(task, name, value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
task_execute(task_t *task)
{
    gboolean ret = TRUE;

    /* TODO: filename, linenumber?? */
    log_push_context(task->name);
        
    if (task->ops->execute != 0 && !(*task->ops->execute)(task))
    {
    	/* TODO: be more gracious, e.g. for <condition> */
    	logf("FAILED\n");
	ret = FALSE;
    }

    log_pop_context();
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_ops_add_attribute(task_ops_t *ops, const task_attr_t *proto)
{
    task_attr_t *ta;
    
    /* TODO: somehow we have to delete these again!! */
    ta = new(task_attr_t);
    *ta = *proto;
    ta->name = g_strdup(ta->name);
    
    if (ops->attrs_hashed == 0)
	ops->attrs_hashed = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(ops->attrs_hashed, ta->name, ta);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    void (*function)(const task_attr_t *ta, void *userdata);
    void *userdata;
} task_ops_foreach_attr_rec_t;

static void
task_ops_attributes_apply_one(gpointer key, gpointer value, gpointer userdata)
{
    task_ops_foreach_attr_rec_t *recp = (task_ops_foreach_attr_rec_t *)userdata;
    task_attr_t *ta = (task_attr_t *)value;
    
    (*recp->function)(ta, recp->userdata);
}

void
task_ops_attributes_apply(
    task_ops_t *ops,
    void (*function)(const task_attr_t *ta, void *userdata),
    void *userdata)
{
    if (ops->attrs_hashed != 0)
    {
    	task_ops_foreach_attr_rec_t rec;
	
	rec.function = function;
	rec.userdata = userdata;
	
    	g_hash_table_foreach(ops->attrs_hashed, task_ops_attributes_apply_one, &rec);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_scope_t *
tscope_new(task_scope_t *parent)
{
    task_scope_t *ts;
    
    ts = new(task_scope_t);

    ts->parent = parent;
    ts->taskdefs = g_hash_table_new(g_str_hash, g_str_equal);
    
    return ts;
}

/* TODO: have some refcounting scheme to ensure that we can 
 *       delete task_ops_t's whose time has come without
 *       trying to delete the static builtin ones.  Currently
 *       we just leak 'em all.
 */
void
tscope_delete(task_scope_t *ts)
{
    g_hash_table_destroy(ts->taskdefs);
    g_free(ts);
}

gboolean
tscope_register(task_scope_t *ts, task_ops_t *ops)
{
    if (g_hash_table_lookup(ts->taskdefs, ops->name) != 0)
    {
    	logf("Task operations \"%s\" already registered, ignoring new definition\n",
	    	ops->name);
    	return FALSE;
    }
    
    g_hash_table_insert(ts->taskdefs, ops->name, ops);
#if DEBUG
    fprintf(stderr, "tscope_register: registering \"%s\"\n", ops->name);
#endif

    if (ops->attrs != 0)
    {
	task_attr_t *ta;

	for (ta = ops->attrs ; ta->name != 0 ; ta++)
	    task_ops_add_attribute(ops, ta);
    }
    
    if (ops->children != 0)
    {
	task_child_t *tc;

    	ops->children_hashed = g_hash_table_new(g_str_hash, g_str_equal);
	for (tc = ops->children ; tc->name != 0 ; tc++)
    	    g_hash_table_insert(ops->children_hashed, tc->name, tc);
    }

    if (ops->init != 0)
    	(*ops->init)();
	
    return TRUE;
}

void
tscope_unregister(task_scope_t *ts, task_ops_t *ops)
{
    g_hash_table_remove(ts->taskdefs, ops->name);
}

task_ops_t *
tscope_find(task_scope_t *ts, const char *name)
{
    for ( ; ts != 0 ; ts = ts->parent)
    {
    	task_ops_t *ops;
	
	if ((ops = (task_ops_t *)g_hash_table_lookup(ts->taskdefs, name)) != 0)
	    return ops;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define TASKOPS(t)  	extern task_ops_t t;
#include "builtin-tasks.h"
#undef TASKOPS

void
task_initialise_builtins(void)
{
    tscope_builtins = tscope_new(0);
    
#define TASKOPS(t)  tscope_register(tscope_builtins, &t);
#include "builtin-tasks.h"
#undef TASKOPS
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
