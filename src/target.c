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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

target_t *
target_new(void)
{
    target_t *targ;
    
    targ = g_new(target_t, 1);
    if (targ == 0)
    	fatal("No memory\n");

    memset(targ, 0, sizeof(*targ));
    
    return targ;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_delete(target_t *targ)
{
    strdelete(targ->name);
    strdelete(targ->description);
	
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

static void
_target_set_condition(target_t *targ, int which, const char *s)
{
    strassign(targ->condition, s);
    
    targ->flags = (targ->flags & ~(T_IFCOND|T_UNLESSCOND)) | which;
}

void
target_set_if_condition(target_t *targ, const char *s)
{
    _target_set_condition(targ, T_IFCOND, s);
}

void
target_set_unless_condition(target_t *targ, const char *s)
{
    _target_set_condition(targ, T_UNLESSCOND, s);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
target_add_task(target_t *targ, task_t *task)
{
    targ->tasks = g_list_append(targ->tasks, task);
    task->target = targ;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
