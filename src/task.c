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

CVSID("$Id: task.c,v 1.3 2001-11-06 09:29:06 gnb Exp $");

static GHashTable *task_ops_all;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_t *
task_new(void)
{
    task_t *task;
    
    task = new(task_t);
    
    task->attributes = props_new(0);
    
    return task;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_delete(task_t *task)
{
    if (task->ops != 0 && task->ops->delete != 0)
    	(*task->ops->delete)(task);
    strdelete(task->id);
	
    props_delete(task->attributes);
	
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

const char *
task_get_attribute(task_t *task, const char *name)
{
    return props_get(task->attributes, name);
}

void
task_add_attribute(task_t *task, const char *name, const char *value)
{
    props_set(task->attributes, name, value);
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
task_ops_register(task_ops_t *ops)
{
    if (task_ops_all == 0)
    	task_ops_all = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(task_ops_all, (gpointer)ops->name, ops);
#if DEBUG
    fprintf(stderr, "task_ops_register: registering \"%s\"\n", ops->name);
#endif
    if (ops->init != 0)
    	(*ops->init)();
}

void
task_ops_unregister(task_ops_t *ops)
{
    if (task_ops_all != 0)
	g_hash_table_remove(task_ops_all, ops->name);
}

task_ops_t *
task_ops_find(const char *name)
{
    return (task_ops_t *)g_hash_table_lookup(task_ops_all, name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define TASKOPS(t)  	extern task_ops_t t;
#include "builtins.h"
#undef TASKOPS

void
task_initialise_builtins(void)
{
#define TASKOPS(t)  task_ops_register(&t);
#include "builtins.h"
#undef TASKOPS
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
