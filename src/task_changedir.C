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

CVSID("$Id: task_changedir.C,v 1.1 2002-03-29 12:36:27 gnb Exp $");

typedef struct
{
    char *dir;
} changedir_private_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
changedir_new(task_t *task)
{
    task->private_data = new(changedir_private_t);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
changedir_set_dir(task_t *task, const char *name, const char *value)
{
    changedir_private_t *cp = (changedir_private_t *)task->private_data;

    strassign(cp->dir, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
changedir_execute(task_t *task)
{
    changedir_private_t *cp = (changedir_private_t *)task->private_data;
    char *dir_e;
    GList *iter;
    gboolean ret = TRUE;

    dir_e = task_expand(task, cp->dir);
    strnullnorm(dir_e);
    if (dir_e == 0)
    {
    	logf("No directory for <changedir>\n");
    	return FALSE;
    }
    
    if (file_is_directory(dir_e) < 0)
    {
    	log_perror(dir_e);
	g_free(dir_e);
	return FALSE;
    }
        
    if (verbose)
	logf("%s\n", dir_e);
    
    file_push_dir(dir_e);

    if (!task_execute_subtasks(task))
    	ret = FALSE;
    
    file_pop_dir();

    g_free(dir_e);
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
changedir_delete(task_t *task)
{
    changedir_private_t *cp = (changedir_private_t *)task->private_data;
    
    strdelete(cp->dir);
    g_free(cp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_attr_t changedir_attrs[] = 
{
    TASK_ATTR(changedir, dir, TT_REQUIRED),
    {0}
};

task_ops_t changedir_ops = 
{
    "changedir",
    /*init*/0,
    changedir_new,
    /*set_content*/0,
    /*post_parse*/0,
    changedir_execute,
    changedir_delete,
    changedir_attrs,
    /*children*/0,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
