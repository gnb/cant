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

CVSID("$Id: task_copy.C,v 1.8 2002-04-13 09:26:06 gnb Exp $");

class copy_task_t : public task_t
{
private:
    char *file_;
    char *tofile_;
    char *todir_;
    list_t<fileset_t> filesets_;
//    list_t<filterset_t> filtersets_;
    mapper_t *mapper_;
    gboolean preserve_last_modified_:1;
    gboolean overwrite_:1;
    gboolean filtering_:1;   	/* whether to apply *global* filters */
    gboolean flatten_:1;
    gboolean include_empty_dirs_:1;

    gboolean result_:1;
    char *exp_todir_;
    unsigned ncopied_;

public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

copy_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
    preserve_last_modified_ = FALSE;
    overwrite_ = FALSE;
    filtering_ = FALSE;
    flatten_ = FALSE;
    include_empty_dirs_ = TRUE;
}

~copy_task_t()
{
    strdelete(file_);
    strdelete(tofile_);
    strdelete(todir_);

    if (mapper_ != 0)
	delete mapper_;
    
    /* delete filesets */
    // TODO: void unref(refcounted_t*)
    filesets_.apply_remove(unref);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_file(const char *name, const char *value)
{
    strassign(file_, value);
    return TRUE;
}

gboolean
set_preservelastmodified(const char *name, const char *value)
{
    boolassign(preserve_last_modified_, value);
    return TRUE;
}

gboolean
set_tofile(const char *name, const char *value)
{
    strassign(tofile_, value);
    return TRUE;
}

gboolean
set_todir(const char *name, const char *value)
{
    strassign(todir_, value);
    return TRUE;
}

gboolean
set_overwrite(const char *name, const char *value)
{
    boolassign(overwrite_, value);
    return TRUE;
}

gboolean
set_filtering(const char *name, const char *value)
{
    boolassign(filtering_, value);
    return TRUE;
}

gboolean
set_flatten(const char *name, const char *value)
{
    boolassign(flatten_, value);
    return TRUE;
}

gboolean
set_includeEmptyDirs(const char *name, const char *value)
{
    boolassign(include_empty_dirs_, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
add_fileset(xml_node_t *node)
{
    fileset_t *fs;

    if ((fs = parse_fileset(project_, node, "dir")) == 0)
    	return FALSE;
	
    filesets_.append(fs);
    return TRUE;
}

gboolean
add_mapper(xml_node_t *node)
{
    if (mapper_ != 0)
    {
    	log::errorf("Only a single \"mapper\" child may be used\n");
	return FALSE;
    }
    
    if ((mapper_ = parse_mapper(project_, node)) == 0)
    	return FALSE;
	
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
post_parse()
{
    if (file_ != 0)
    {
    	if (tofile_ == 0 && todir_ == 0)
	{
	    log::errorf("One of \"tofile\" or \"todir\" must be specified with \"file\"\n");
	    return FALSE;
	}
    }
    else if (filesets_.first() != 0)
    {
    	if (tofile_ != 0)
	{
	    log::errorf("Only \"todir\" is allowed with \"filesets\"\n");
	    return FALSE;
	}
    	if (todir_ == 0)
	{
	    log::errorf("Must use \"todir\" is \"filesets\"\n");
	    return FALSE;
	}
    }
    else if (file_ == 0 && filesets_.first() == 0)
    {
    	log::errorf("At least one of \"file\" or \"<fileset>\" must be present\n");
	return FALSE;
    }

    if (mapper_ == 0)
    	mapper_ = mapper_t::create("identity", 0, 0);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: filtersets */
/* TODO: preserve mod times */
static gboolean
do_copy_file(const char *fromfile, const char *tofile)
{
    FILE *fromfp, *tofp;
    mode_t mode;
    char *todir;
    int n;
    char buf[1024];
    
    todir = file_dirname(fromfile);
    if ((mode = file_mode(todir)) < 0)
    	mode = 0755;
    g_free(todir);
    
    todir = file_dirname(tofile);
    if (file_build_tree(todir, mode) < 0)
    {
    	g_free(todir);
	return FALSE;
    }
    g_free(todir);
    
    if ((fromfp = fopen(fromfile, "r")) == 0)
    {
    	log::perror(fromfile);
	return FALSE;
    }

    if ((mode = file_mode(fromfile)) == 0)
    	mode = 0644;	    /* should never happen */
    if ((tofp = file_open_mode(tofile, "w", mode)) == 0)
    {
    	log::perror(tofile);
	fclose(fromfp);
	return FALSE;
    }
    
    while ((n = fread(buf, 1, sizeof(buf), fromfp)) > 0)
    	fwrite(buf, 1, n, tofp);
	
    fclose(fromfp);
    fclose(tofp);
    
    return TRUE;
}


static gboolean
copy_one(const char *filename, void *userdata)
{
    copy_task_t *ct = (copy_task_t *)userdata;
    const char *fromfile;
    char *tofile, *mappedfile;
    
    // TODO: WTF
        
    /* construct the target filename */
    if (ct->flatten_)
    	fromfile = file_basename_c(filename);
    else
    	fromfile = filename;
	
    if (ct->tofile_ != 0)
    	tofile = ct->expand(ct->tofile_);
    else if (ct->exp_todir_ != 0)
	tofile = g_strconcat(ct->exp_todir_, "/", filename, 0);
    else
    {
	ct->result_ = FALSE; 	/* failed */
	g_free(tofile);
	return FALSE;	    	/* stop iteration */
    }
    if ((mappedfile = ct->mapper_->map(tofile)) == 0)
    {
	g_free(tofile);
	return TRUE;	    	/* keep going */
    }
        
    log::infof("%s -> %s\n", filename, mappedfile);
	
    /* TODO: apply proj->basedir */

    if (file_is_directory(filename) == 0)
    {
    	int r;
	unsigned ncopied = ct->ncopied_;
	
	r = file_apply_children(filename, copy_one, userdata);
	
	if (r < 0)
	{
	    log::perror(filename);
	}
	else if (ncopied == ct->ncopied_ && ct->include_empty_dirs_)
	{
	    /* no files copied underneath this directory -- it must be empty */
	    /* TODO: sensible mode from original directory */
	    /* TODO: control uid,gid */
	    if (file_build_tree(mappedfile, 0755))
	    {
		log::perror(filename);
		ct->result_ = FALSE;
    	    }
	}
    }
    else
    {
	if (!do_copy_file(filename, mappedfile))
	    ct->result_ = FALSE;
    }
        
    return ct->result_;    /* keep going if we didn't fail */
}

gboolean
exec()
{
    list_iterator_t<fileset_t> iter;
    
    result_ = TRUE;
    ncopied_ = 0;
    
    exp_todir_ = expand(todir_);

    if (file_ != 0)
    {
    	char *expfile = expand(file_);
    	copy_one(expfile, this);
	g_free(expfile);
    }

    /* execute for <fileset> children */
    
    for (iter = filesets_.first() ; iter != 0 ; ++iter)
	(*iter)->apply(project_->properties(), copy_one, this);
    
    strdelete(exp_todir_);

    return result_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

};  // end of class

static task_attr_t copy_attrs[] =
{
    TASK_ATTR(copy, file, 0),
    TASK_ATTR(copy, preservelastmodified, 0),
    TASK_ATTR(copy, tofile, 0),
    TASK_ATTR(copy, todir, 0),
    TASK_ATTR(copy, overwrite, 0),
    TASK_ATTR(copy, filtering, 0),
    TASK_ATTR(copy, flatten, 0),
    TASK_ATTR(copy, includeEmptyDirs, 0),
    {0}
};

static task_child_t copy_children[] = 
{
    TASK_CHILD(copy, fileset, 0),
    TASK_CHILD(copy, mapper, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(copy,
			copy_attrs,
			copy_children,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/FALSE)
TASK_DEFINE_CLASS_END(copy)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
