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
#include "tok.h"
#include <time.h>

CVSID("$Id: task_cant.c,v 1.3 2002-02-11 05:34:05 gnb Exp $");

typedef struct
{
    char *buildfile;
    char *dir;
    char *target;
    char *output;
    gboolean inherit_all;
} cant_private_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
cant_new(task_t *task)
{
    cant_private_t *cp;

    cp = new(cant_private_t);
    cp->inherit_all = TRUE;
    task->private = cp;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
cant_set_buildfile(task_t *task, const char *name, const char *value)
{
    cant_private_t *cp = (cant_private_t *)task->private;
    
    strassign(cp->buildfile, value);
    return TRUE;
}

static gboolean
cant_set_dir(task_t *task, const char *name, const char *value)
{
    cant_private_t *cp = (cant_private_t *)task->private;

    strassign(cp->dir, value);
    return TRUE;
}

static gboolean
cant_set_target(task_t *task, const char *name, const char *value)
{
    cant_private_t *cp = (cant_private_t *)task->private;
    
    strassign(cp->target, value);
    return TRUE;
}

static gboolean
cant_set_output(task_t *task, const char *name, const char *value)
{
    cant_private_t *cp = (cant_private_t *)task->private;

    strassign(cp->output, value);
    return TRUE;
}

static gboolean
cant_set_inheritAll(task_t *task, const char *name, const char *value)
{
    cant_private_t *cp = (cant_private_t *)task->private;

    boolassign(cp->inherit_all, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Setup the magical variables _PATHUP and _PATHDOWN
 */
static void
cant_setup_magical_paths(project_t *proj)
{
    const char *d;
    estring path;
    tok_t tok;

    tok_init_m(&tok, file_normalise(proj->basedir, 0), "/");
    estring_init(&path);
    
    /* TODO: We need something like a file_denormalise() */
    
    while ((d = tok_next(&tok)) != 0)
	estring_append_string(&path, "../");
    
    project_set_property(proj, "_pathup", path.data);
    project_set_property(proj, "topdir", path.data);
    
    estring_truncate(&path);
    estring_append_string(&path, proj->basedir);
    if (path.length)
    	estring_append_char(&path, '/');
    project_set_property(proj, "_pathdown", path.data);

    
    estring_free(&path);
    tok_free(&tok);
}

/* TODO: support nested <property> tags */
/* TODO: cache projects to avoid re-reading them every execute */

static gboolean
cant_execute(task_t *task)
{
    cant_private_t *cp = (cant_private_t *)task->private;
    project_t *proj;
    char *buildfile_e;
    char *dir_e;
    char *target_e;
    gboolean ret;

    dir_e = task_expand(task, cp->dir);
    strnullnorm(dir_e);
    if (dir_e == 0)
    	dir_e = g_strdup(".");

    buildfile_e = task_expand(task, cp->buildfile);
    strnullnorm(buildfile_e);
    if (buildfile_e == 0)
	buildfile_e = g_strconcat(dir_e, "/build.xml", 0);
    buildfile_e = file_normalise_m(buildfile_e, 0);
	
    proj = read_buildfile(buildfile_e,
	    	    (cp->inherit_all ? task->target->project : project_globals));
    if (proj == 0)
    {
    	g_free(dir_e);
	g_free(buildfile_e);
	return FALSE;
    }
    
    strassign(proj->basedir, dir_e);
    g_free(dir_e);
    
    cant_setup_magical_paths(proj);

#if DEBUG
    dump_project(proj);
#endif

    /*
     * TODO: The default value of "target" should be the name
     * of the target which invoked us, which makes recursive
     * targets marginally easier.  Either that or define
     * a $@ like property on the task.
     */
    target_e = task_expand(task, cp->target);
    strnullnorm(target_e);
    if (target_e == 0)
    	target_e = g_strdup(task->project->default_target);

    /*
     * Now actually execute the target in the sub-project.
     */
    if (verbose)
	logf("buildfile %s\n", buildfile_e);
    
    file_push_dir(proj->basedir);
    ret = project_execute_target_by_name(proj, target_e);
    file_pop_dir();
    
    g_free(target_e);
    g_free(buildfile_e);
    project_delete(proj);
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
cant_delete(task_t *task)
{
    cant_private_t *cp = (cant_private_t *)task->private;
    
    strdelete(cp->buildfile);
    strdelete(cp->dir);
    strdelete(cp->target);
    strdelete(cp->output);
    g_free(cp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_attr_t cant_attrs[] = 
{
    TASK_ATTR(cant, buildfile, 0),
    TASK_ATTR(cant, dir, 0),
    TASK_ATTR(cant, target, 0),
    TASK_ATTR(cant, output, 0),
    TASK_ATTR(cant, inheritAll, 0),
    {0}
};

task_ops_t cant_ops = 
{
    "cant",
    /*init*/0,
    cant_new,
    /*set_content*/0,
    /*post_parse*/0,
    cant_execute,
    cant_delete,
    cant_attrs,
    /*children*/0,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
