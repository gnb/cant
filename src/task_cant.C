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
#include <time.h>

CVSID("$Id: task_cant.C,v 1.3 2002-04-12 13:07:24 gnb Exp $");

class cant_task_t : public task_t
{
private:
    char *buildfile_;
    char *dir_;
    char *target_;
    char *output_;
    gboolean inherit_all_;

public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cant_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
    inherit_all_ = TRUE;
}

~cant_task_t()
{
    strdelete(buildfile_);
    strdelete(dir_);
    strdelete(target_);
    strdelete(output_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_buildfile(const char *name, const char *value)
{
    strassign(buildfile_, value);
    return TRUE;
}

gboolean
set_dir(const char *name, const char *value)
{
    strassign(dir_, value);
    return TRUE;
}

gboolean
set_target(const char *name, const char *value)
{
    strassign(target_, value);
    return TRUE;
}

gboolean
set_output(const char *name, const char *value)
{
    strassign(output_, value);
    return TRUE;
}

gboolean
set_inheritAll(const char *name, const char *value)
{
    boolassign(inherit_all_, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: support nested <property> tags */
/* TODO: cache projects to avoid re-reading them every execute */

gboolean
exec()
{
    project_t *proj;
    char *buildfile_e;
    char *dir_e;
    char *target_e;
    gboolean ret;

    dir_e = expand(dir_);
    strnullnorm(dir_e);
    if (dir_e == 0)
    	dir_e = g_strdup(".");

    buildfile_e = expand(buildfile_);
    strnullnorm(buildfile_e);
    if (buildfile_e == 0)
	buildfile_e = g_strconcat(dir_e, "/build.xml", 0);
    buildfile_e = file_normalise_m(buildfile_e, 0);
	
    proj = read_buildfile(buildfile_e,
	    	    (inherit_all_ ? project_ : project_globals));
    if (proj == 0)
    {
    	g_free(dir_e);
	g_free(buildfile_e);
	return FALSE;
    }
    
    strassign(proj->basedir, dir_e);
    g_free(dir_e);
    
#if DEBUG
    dump_project(proj);
#endif

    /*
     * TODO: The default value of "target" should be the name
     * of the target which invoked us, which makes recursive
     * targets marginally easier.  Either that or define
     * a $@ like property on the task.
     * TODO: is the default target taken from the right project?
     */
    target_e = expand(target_);
    strnullnorm(target_e);
    if (target_e == 0)
    	target_e = g_strdup(project_->default_target);

    /*
     * Now actually execute the target in the sub-project.
     */
    if (verbose)
	log::infof("buildfile %s\n", buildfile_e);
    
    file_push_dir(proj->basedir);
    ret = project_execute_target_by_name(proj, target_e);
    file_pop_dir();
    
    g_free(target_e);
    g_free(buildfile_e);
    project_delete(proj);
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

static task_attr_t cant_attrs[] = 
{
    TASK_ATTR(cant, buildfile, 0),
    TASK_ATTR(cant, dir, 0),
    TASK_ATTR(cant, target, 0),
    TASK_ATTR(cant, output, 0),
    TASK_ATTR(cant, inheritAll, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(cant,
			cant_attrs,
			/*children*/0,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/FALSE)
TASK_DEFINE_CLASS_END(cant)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
