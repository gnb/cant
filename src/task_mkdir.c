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

static gboolean
mkdir_parse(task_t *task, xmlNode *node)
{
    if (task_get_attribute(task, "dir") == 0)
    {
    	parse_error("Required attribute \"dir\" missing\n");
    	return FALSE;
    }
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
mkdir_execute(task_t *task)
{
    char *dir;
    char *modestr;
    mode_t mode;
    gboolean ret = TRUE;
    
    dir = task_get_attribute_expanded(task, "dir");
    modestr = task_get_attribute_expanded(task, "mode");
    
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

task_ops_t mkdir_ops = 
{
    "mkdir",
    /*init*/0,
    mkdir_parse,
    mkdir_execute,
    /*delete*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
