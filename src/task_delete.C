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

CVSID("$Id: task_delete.C,v 1.2 2002-04-02 11:52:28 gnb Exp $");

class delete_task_t : public task_t
{
private:
    char *file_;
    char *directory_;
    GList *filesets_;
    gboolean verbose_:1;
    gboolean quiet_:1;
    gboolean fail_on_error_:1;
    gboolean include_empty_dirs_:1;
    gboolean result_:1;

public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

delete_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
    /* default values */
    verbose_ = FALSE;
    quiet_ = FALSE;
    fail_on_error_ = TRUE;
    include_empty_dirs_ = FALSE;
}

~delete_task_t()
{
    strdelete(file_);
    strdelete(directory_);
    
    /* delete filesets */
    listdelete(filesets_, fileset_t, fileset_unref);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void delete_delete(task_t *);
    
gboolean
set_file(const char *name, const char *value)
{
    strassign(file_, value);
    return TRUE;
}

gboolean
set_dir(const char *name, const char *value)
{
    strassign(directory_, value);
    return TRUE;
}
    
gboolean
set_verbose(const char *name, const char *value)
{
    boolassign(verbose_, value);
    return TRUE;
}

gboolean
set_quiet(const char *name, const char *value)
{
    boolassign(quiet_, value);
    return TRUE;
}

gboolean
set_failonerror(const char *name, const char *value)
{
    boolassign(fail_on_error_, value);
    return TRUE;
}

gboolean
set_includeEmptyDirs(const char *name, const char *value)
{
    boolassign(include_empty_dirs_, value);
    return TRUE;
}

gboolean
add_fileset(xmlNode *node)
{
    fileset_t *fs;

    if ((fs = parse_fileset(project_, node, "dir")) == 0)
    	return FALSE;
	
    filesets_ = g_list_append(filesets_, fs);
    return TRUE;
}

gboolean
post_parse()
{
    if (file_ == 0 && directory_ == 0 && filesets_ == 0)
    {
    	parse_error("At least one of \"file\", \"dir\" or \"<fileset>\" must be present\n");
	return FALSE;
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
delete_one(const char *filename/*unnormalised*/, void *userdata)
{
    delete_task_t *dt = (delete_task_t *)userdata;
    int r;
    
    if (dt->verbose_)
    	logf("%s\n", filename);
	
    if (file_is_directory(filename) == 0)
    {
    	r = file_apply_children(filename, delete_one, dt);

	if (r < 0)
	{
	    log_perror(filename);
	}
	else if (r == 1)
	{
	    if (dt->include_empty_dirs_ && file_rmdir(filename) < 0)
	    {
	    	if (!dt->quiet_)
		    log_perror(filename);
		if (dt->fail_on_error_)
		    dt->result_ = FALSE;
    	    }
	}	    
    }
    else
    {
	if (file_unlink(filename) < 0)
	{
	    if (!dt->quiet_)
		log_perror(filename);
	    if (dt->fail_on_error_)
		dt->result_ = FALSE;
	}
    }
        
    return TRUE;    /* keep going */
}

gboolean
exec()
{
    char *expfile;
    GList *iter;
    
    result_ = TRUE;
    
    if (!verbose_)
    	logf("\n");
    
    if (file_ != 0)
    {
    	expfile = expand(file_);
    	delete_one(expfile, this);
	g_free(expfile);
    }
    if (directory_ != 0)
    {
    	expfile = expand(directory_);
    	delete_one(expfile, this);
	g_free(expfile);
    }

    /* execute for <fileset> children */
    
    for (iter = filesets_ ; iter != 0 ; iter = iter->next)
    {
    	fileset_t *fs = (fileset_t *)iter->data;
	
	fileset_apply(fs, project_get_props(project_), delete_one, this);
    }

    return result_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

static task_attr_t delete_attrs[] = 
{
    TASK_ATTR(delete, file, 0),
    TASK_ATTR(delete, dir, 0),
    TASK_ATTR(delete, verbose, 0),
    TASK_ATTR(delete, quiet, 0),
    TASK_ATTR(delete, failonerror, 0),
    TASK_ATTR(delete, includeEmptyDirs, 0),
    {0}
};

static task_child_t delete_children[] = 
{
    TASK_CHILD(delete, fileset, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(delete,
			delete_attrs,
			delete_children,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/FALSE)
TASK_DEFINE_CLASS_END(delete)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
