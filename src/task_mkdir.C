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

CVSID("$Id: task_mkdir.C,v 1.1 2002-03-29 12:36:27 gnb Exp $");

typedef struct
{
    char *directory;
    char *mode;
} mkdir_private_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
mkdir_new(task_t *task)
{
    task->private_data = new(mkdir_private_t);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
mkdir_set_dir(task_t *task, const char *name, const char *value)
{
    mkdir_private_t *mp = (mkdir_private_t *)task->private_data;
    
    strassign(mp->directory, value);
    return TRUE;
}

static gboolean
mkdir_set_mode(task_t *task, const char *name, const char *value)
{
    mkdir_private_t *mp = (mkdir_private_t *)task->private_data;
    
    strassign(mp->mode, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
mkdir_execute(task_t *task)
{
    mkdir_private_t *mp = (mkdir_private_t *)task->private_data;
    char *dir;
    char *modestr;
    mode_t mode;
    gboolean ret = TRUE;
    
    dir = task_expand(task, mp->directory);
    if (dir != 0 && *dir == '\0')
    {
    	g_free(dir);
	dir = 0;
    }
    if (dir == 0)
    	return TRUE;	/* empty directory -> trivially succeed ??? */
    
    modestr = task_expand(task, mp->mode);
    
    mode = file_mode_from_string(modestr, 0, 0755);
    
    /* TODO: canonicalise */
    /* TODO: use project's basedir */
    logf("%s\n", dir);
    
    if (file_build_tree(dir, mode) < 0)
    {
    	perror(dir);
    	ret = FALSE;
    }
    
    g_free(dir);
    if (modestr != 0)
	g_free(modestr);
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_attr_t mkdir_attrs[] = 
{
    TASK_ATTR(mkdir, dir, TT_REQUIRED),
    TASK_ATTR(mkdir, mode, 0),
    {0}
};

task_ops_t mkdir_ops = 
{
    "mkdir",
    /*init*/0,
    mkdir_new,
    /*set_content*/0,
    /*post_parse*/0,
    mkdir_execute,
    /*delete*/0,
    mkdir_attrs,
    /*children*/0,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
