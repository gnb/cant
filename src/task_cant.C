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

CVSID("$Id: task_cant.C,v 1.6 2002-04-13 12:30:42 gnb Exp $");

class cant_task_t : public task_t
{
private:
    string_var buildfile_;
    string_var dir_;
    string_var target_;
    string_var output_;
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
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_buildfile(const char *name, const char *value)
{
    buildfile_ = value;
    return TRUE;
}

gboolean
set_dir(const char *name, const char *value)
{
    dir_ = value;
    return TRUE;
}

gboolean
set_target(const char *name, const char *value)
{
    target_ = value;
    return TRUE;
}

gboolean
set_output(const char *name, const char *value)
{
    output_ = value;
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
    string_var buildfile_e;
    string_var dir_e;
    string_var target_e;
    gboolean ret;

    dir_e = expand(dir_);
    if (dir_e.is_null())
    	dir_e = ".";

    buildfile_e = expand(buildfile_);
    if (buildfile_e.is_null())
	buildfile_e = g_strconcat(dir_e, "/build.xml", 0);
    buildfile_e = file_normalise(buildfile_e, 0);
	
    proj = read_buildfile(buildfile_e, (inherit_all_ ? project_ : 0));
    if (proj == 0)
	return FALSE;

    // TODO: double check that _PATHUP etc are right 
    proj->set_basedir(dir_e);
    
#if DEBUG
    proj->dump();
#endif

    /*
     * TODO: The default value of "target" should be the name
     * of the target which invoked us, which makes recursive
     * targets marginally easier.  Either that or define
     * a $@ like property on the task.
     * TODO: is the default target taken from the right project?
     */
    target_e = expand(target_);
    if (target_e.is_null())
    	target_e = project_->default_target();

    /*
     * Now actually execute the target in the sub-project.
     */
    if (verbose)
	log::infof("buildfile %s\n", buildfile_e.data());
    
    file_push_dir(proj->basedir());
    ret = proj->execute_target_by_name(target_e);
    file_pop_dir();
    
    delete proj;
    
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
