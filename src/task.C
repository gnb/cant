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

CVSID("$Id: task.C,v 1.2 2002-03-29 16:12:31 gnb Exp $");

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
    if (task->ops != 0 && task->ops->dtor != 0)
    	(*task->ops->dtor)(task);
    strdelete(task->id);

    listdelete(task->subtasks, task_t, task_delete);
    	
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

void
task_add_subtask(task_t *task, task_t *subtask)
{
    assert(task->ops->is_composite);
    task->subtasks = g_list_append(task->subtasks, subtask);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_attach(task_t *task, target_t *targ)
{
    GList *iter;
    
    task->target = targ;
    for (iter = task->subtasks ; iter != 0 ; iter = iter->next)
    {
    	task_t *subtask = (task_t *)iter->data;
	
	task_attach(subtask, targ);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
task_set_attribute(task_t *task, const char *name, const char *value)
{
    task_attr_t *ta = 0;
    
    if (task->ops->attrs_hashed != 0)
	ta = task->ops->attrs_hashed->lookup(name);

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

gboolean
task_execute_subtasks(task_t *task)
{
    GList *iter;

    for (iter = task->subtasks ; iter != 0 ; iter = iter->next)
    {
	task_t *subtask = (task_t *)iter->data;

	if (!task_execute(subtask))
	    return FALSE;
    }
    return TRUE;
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
	ops->attrs_hashed = new hashtable_t<const char *, task_attr_t>;
    ops->attrs_hashed->insert(ta->name, ta);
}


typedef struct
{
    void (*function)(const task_attr_t *ta, void *userdata);
    void *userdata;
} task_ops_foreach_attr_rec_t;

static void
task_ops_attributes_apply_one(const char *key, task_attr_t *ta, void *userdata)
{
    task_ops_foreach_attr_rec_t *recp = (task_ops_foreach_attr_rec_t *)userdata;
    
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
	
    	ops->attrs_hashed->foreach(task_ops_attributes_apply_one, &rec);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const task_child_t *
task_ops_find_child(const task_ops_t *ops, const char *name)
{
    task_child_t *tc;
    
    if (ops->children_hashed == 0)
    	return 0;
    if ((tc = ops->children_hashed->lookup(name)) != 0)
    	return tc;
    return ops->children_hashed->lookup("*");
}

void
task_ops_add_child(task_ops_t *ops, const task_child_t *proto)
{
    task_child_t *tc;
    
    /* TODO: somehow we have to delete these again!! */
    tc = new(task_child_t);
    *tc = *proto;
    tc->name = g_strdup(tc->name);
    
    if (ops->children_hashed == 0)
	ops->children_hashed = new hashtable_t<const char *, task_child_t>;
    ops->children_hashed->insert(tc->name, tc);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
remove_one_attr(const char *key, task_attr_t *ta, void *userdata)
{
    g_free(ta->name);
    g_free(ta);

    return TRUE;    /* remove me */
}

static gboolean
remove_one_child(const char *key, task_child_t *tc, void *userdata)
{
    g_free(tc->name);
    g_free(tc);
    
    return TRUE;    /* remove me */
}

static void
task_ops_cleanup(task_ops_t *ops)
{
    if (ops->cleanup != 0)
    	(*ops->cleanup)(ops);
   
    if (ops->attrs_hashed != 0)
    {
	ops->attrs_hashed->foreach_remove(remove_one_attr, 0);
	delete ops->attrs_hashed;
    }

    if (ops->children_hashed != 0)
    {
	ops->children_hashed->foreach_remove(remove_one_child, 0);
	delete ops->children_hashed;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_scope_t *
tscope_new(task_scope_t *parent)
{
    task_scope_t *ts;
    
    ts = new(task_scope_t);

    ts->parent = parent;
    ts->taskdefs = new hashtable_t<const char *, task_ops_t>;
    
    return ts;
}

static gboolean
cleanup_one_taskdef(const char *key, task_ops_t *value, void *userdata)
{
    task_ops_cleanup(value);
    return TRUE;    /* remove me */
}

void
tscope_delete(task_scope_t *ts)
{
    ts->taskdefs->foreach_remove(cleanup_one_taskdef, 0);
    delete ts->taskdefs;
    g_free(ts);
}

gboolean
tscope_register(task_scope_t *ts, task_ops_t *ops)
{
    if (ts->taskdefs->lookup(ops->name) != 0)
    {
    	logf("Task operations \"%s\" already registered, ignoring new definition\n",
	    	ops->name);
    	return FALSE;
    }
    
    ts->taskdefs->insert(ops->name, ops);
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

	for (tc = ops->children ; tc->name != 0 ; tc++)
	    task_ops_add_child(ops, tc);
    }

    if (ops->init != 0)
    	(*ops->init)();
	
    return TRUE;
}

void
tscope_unregister(task_scope_t *ts, task_ops_t *ops)
{
    ts->taskdefs->remove(ops->name);
}

task_ops_t *
tscope_find(task_scope_t *ts, const char *name)
{
    for ( ; ts != 0 ; ts = ts->parent)
    {
    	task_ops_t *ops;
	
	if ((ops = ts->taskdefs->lookup(name)) != 0)
	    return ops;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define TASKOPS(t)  	extern task_ops_t t;
#include "builtin-tasks.H"
#undef TASKOPS

void
task_initialise_builtins(void)
{
    tscope_builtins = tscope_new(0);
    
#define TASKOPS(t)  tscope_register(tscope_builtins, &t);
#include "builtin-tasks.H"
#undef TASKOPS
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
