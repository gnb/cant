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

CVSID("$Id: task_mkdir.C,v 1.4 2002-04-13 12:30:42 gnb Exp $");

class mkdir_task_t : public task_t
{
private:
    string_var directory_;
    string_var mode_;

public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mkdir_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
}

~mkdir_task_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_dir(const char *name, const char *value)
{
    directory_ = value;
    return TRUE;
}

gboolean
set_mode(const char *name, const char *value)
{
    mode_ = value;
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
exec()
{
    string_var dir;
    string_var modestr;
    mode_t mode;
    gboolean ret = TRUE;
    
    dir = expand(directory_);
    if (dir.is_null())
    	return TRUE;	/* empty directory -> trivially succeed ??? */
    
    modestr = expand(mode_);
    
    mode = file_mode_from_string(modestr, 0, 0755);
    
    /* TODO: canonicalise */
    /* TODO: use project's basedir */
    log::infof("%s\n", dir.data());
    
    if (file_build_tree(dir, mode) < 0)
    {
    	perror(dir);
    	ret = FALSE;
    }
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

static task_attr_t mkdir_attrs[] = 
{
    TASK_ATTR(mkdir, dir, TT_REQUIRED),
    TASK_ATTR(mkdir, mode, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(mkdir,
			mkdir_attrs,
			/*children*/0,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/FALSE)
TASK_DEFINE_CLASS_END(mkdir)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
