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

CVSID("$Id: target.C,v 1.10 2002-04-13 12:30:42 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

target_t::target_t()
{
}

target_t::~target_t()
{
    tasks_.delete_all();
    depends_.remove_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_t::set_name(const char *s)
{
    name_ = s;
}

void
target_t::set_description(const char *s)
{
    description_ = s;
}

void
target_t::set_is_defined(gboolean b)
{
    if (b)
    	flags_ |= T_DEFINED;
    else
    	flags_ &= ~T_DEFINED;
}

void
target_t::set_project(project_t *proj)
{
    project_  = proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_t::add_depend(target_t *dep)
{
    dep->flags_ |= T_DEPENDED_ON;
    depends_.append(dep);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_t::add_task(task_t *task)
{
    tasks_.append(task);
    task->attach(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
target_t::execute()
{
    list_iterator_t<target_t> diter;
    list_iterator_t<task_t> titer;

    if (!condition_.evaluate(project_->properties()))
	return TRUE;	    /* disabled target: trivially successful */
    
    log_tree_context_t context(name_);
    log::infof("\n");
    
    /* TODO: check if finished first */

    /* go to depends first */
    for (diter = depends_.first() ; diter != 0 ; ++diter)
    {
	if (!(*diter)->execute())
	    return FALSE;
    }
    
    /* now handle this target's tasks */
    for (titer = tasks_.first() ; titer != 0 ; ++titer)
    {
    	task_t *task = *titer;
	
	if (!task->execute())
	{
	    job_t::clear();
	    return FALSE;
	}
    }
    
    job_t::run();
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if DEBUG

void
target_t::dump() const
{
    list_iterator_t<target_t> diter;
    list_iterator_t<task_t> titer;
    char *cond_desc;
    
    fprintf(stderr, "    TARGET {\n");
    fprintf(stderr, "        NAME=\"%s\"\n", name());
    fprintf(stderr, "        DESCRIPTION=\"%s\"\n", description());
    fprintf(stderr, "        FLAGS=\"%08x\"\n", flags_);
    cond_desc = condition_.describe();
    fprintf(stderr, "        CONDITION=%s\n", cond_desc);
    g_free(cond_desc);
    fprintf(stderr, "        DEPENDS {\n");
    for (diter = depends_.first() ; diter != 0 ; ++diter)
    {
    	target_t *dep = *diter;
	fprintf(stderr, "            \"%s\"\n", dep->name());
    }
    fprintf(stderr, "        }\n");
    for (titer = tasks_.first() ; titer != 0 ; ++titer)
    {
    	task_t *task = *titer;
	fprintf(stderr, "        TASK {\n");
	fprintf(stderr, "            NAME = \"%s\"\n", task->name());
	fprintf(stderr, "            ID = \"%s\"\n", task->id());
	fprintf(stderr, "        }\n");
    }
    fprintf(stderr, "    }\n");
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
