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
#include "job.H"

CVSID("$Id: target.C,v 1.5 2002-04-07 04:22:52 gnb Exp $");

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
    targ->tasks.delete_all();
    strdelete(targ->name);
    strdelete(targ->description);
	
    targ->depends.remove_all();
	
    delete targ;
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

void
target_set_is_defined(target_t *targ, gboolean b)
{
    if (b)
    	targ->flags |= T_DEFINED;
    else
    	targ->flags &= ~T_DEFINED;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_add_depend(target_t *targ, target_t *dep)
{
    dep->flags |= T_DEPENDED_ON;
    targ->depends.append(dep);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_add_task(target_t *targ, task_t *task)
{
    targ->tasks.append(task);
    task->attach(targ);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
target_execute(target_t *targ)
{
    list_iterator_t<target_t> diter;
    list_iterator_t<task_t> titer;

    if (!targ->condition.evaluate(
    	    	    	    project_get_props(targ->project)))
	return TRUE;	    /* disabled target: trivially successful */
    
    log_push_context(targ->name);
    logf("\n");
    
    /* TODO: check if finished first */

    /* go to depends first */
    for (diter = targ->depends.first() ; diter != 0 ; ++diter)
    {
    	target_t *dep = *diter;
	
	if (!target_execute(dep))
	{
	    log_pop_context();
	    return FALSE;
	}
    }
    
    /* now handle this target's tasks */
    for (titer = targ->tasks.first() ; titer != 0 ; ++titer)
    {
    	task_t *task = *titer;
	
	if (!task->execute())
	{
	    job_t::clear();
	    log_pop_context();
	    return FALSE;
	}
    }
    
    job_t::run();
    
    log_pop_context();
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
