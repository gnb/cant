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
#include "job.h"

CVSID("$Id: target.c,v 1.6 2001-11-21 16:31:34 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

target_t *
target_new(void)
{
    target_t *targ;
    
    targ = new(target_t);
    
    return targ;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_delete(target_t *targ)
{
    listdelete(targ->tasks, task_t, task_delete);
    strdelete(targ->name);
    strdelete(targ->description);
    condition_free(&targ->condition);
	
    while (targ->depends != 0)
    	targ->depends = g_list_remove_link(targ->depends, targ->depends);
	
    g_free(targ);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_set_name(target_t *targ, const char *s)
{
    strassign(targ->name, s);
}

void
target_set_description(target_t *targ, const char *s)
{
    strassign(targ->description, s);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_add_task(target_t *targ, task_t *task)
{
    targ->tasks = g_list_append(targ->tasks, task);
    task->target = targ;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
target_execute(target_t *targ)
{
    GList *iter;

    if (!condition_evaluate(&targ->condition,
    	    	    	    project_get_props(targ->project)))
	return TRUE;	    /* disabled target: trivially successful */
    
    log_push_context(targ->name);
    logf("\n");
    
    /* TODO: check if finished first */

    /* go to depends first */
    for (iter = targ->depends ; iter != 0 ; iter = iter->next)
    {
    	target_t *dep = (target_t *)iter->data;
	
	if (!target_execute(dep))
	{
	    log_pop_context();
	    return FALSE;
	}
    }
    
    /* now handle this target's tasks */
    for (iter = targ->tasks ; iter != 0 ; iter = iter->next)
    {
    	task_t *task = (task_t *)iter->data;
	
	if (!task_execute(task))
	{
	    job_clear();
	    log_pop_context();
	    return FALSE;
	}
    }
    
    job_run();
    
    log_pop_context();
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
