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

CVSID("$Id: task_changedir.C,v 1.5 2002-04-13 12:30:42 gnb Exp $");

class changedir_task_t : public task_t
{
private:
    string_var dir_;
    
public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

changedir_task_t(task_class_t *tclass, project_t *proj)
 : task_t(tclass, proj)
{
}

~changedir_task_t()
{
}
    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_dir(const char *name, const char *value)
{
    dir_ = value;
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
exec()
{
    string_var dir_e;
    gboolean ret = TRUE;

    dir_e = expand(dir_);
    if (dir_e.is_null())
    {
    	log::errorf("No directory for <changedir>\n");
    	return FALSE;
    }
    
    if (file_is_directory(dir_e) < 0)
    {
    	log::perror(dir_e);
	return FALSE;
    }
        
    if (verbose)
	log::infof("%s\n", dir_e.data());
    
    file_push_dir(dir_e);

    if (!execute_subtasks())
    	ret = FALSE;
    
    file_pop_dir();

    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

static task_attr_t changedir_attrs[] = 
{
    TASK_ATTR(changedir, dir, TT_REQUIRED),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(changedir,
			changedir_attrs,
			/*children*/0,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/FALSE);
TASK_DEFINE_CLASS_END(changedir)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
