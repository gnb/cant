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

#include "xtask.H"
#include "job.H"

CVSID("$Id: xtask.C,v 1.4 2002-04-02 11:52:28 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_t::xtask_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
    /* TODO: delay attachment to project? */
    properties_ = props_new(project_get_props(project_));
}

xtask_t::~xtask_t()
{
#if DEBUG
    fprintf(stderr, "~xtask_t: deleting \"%s\"\n", name_);
#endif

    /* TODO: delete unrefed filesets */
    listdelete(taglists_, taglist_t, taglist_unref);
    
    props_delete(properties_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
xtask_t::generic_setter(const char *name, const char *value)
{
    xtask_class_t *xtclass = (xtask_class_t *)tclass_;	/* downcast */
    const char *propname;
    
    if ((propname = props_get(xtclass->property_map_, name)) == 0)
    	return FALSE;
    props_set(properties_, propname, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
xtask_t::generic_adder(xmlNode *node)
{
    taglist_t *tl;
    
    if ((tl = parse_taglist(project_, node)) == 0)
    	return FALSE;
    
    taglists_ = g_list_append(taglists_, tl);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: build env too */
gboolean
xtask_t::build_command(strarray_t *command)
{
    xtask_class_t *xtclass = (xtask_class_t *)tclass_;     /* downcast */
    GList *iter;
    char *exp;

    if (xtclass->executable_ != 0)
    	command->appendm(props_expand(properties_, xtclass->executable_));
    
    for (iter = xtclass->args_ ; iter != 0 ; iter = iter->next)
    {
    	xtask_class_t::arg_t *xa = (xtask_class_t::arg_t *)iter->data;

    	if (!condition_evaluate(&xa->condition, properties_))
	    continue;
	
    	switch (xa->type)
	{
	case xtask_class_t::XT_VALUE:	    /* <arg value=""> child */
	    /* TODO: <arg arglistref=""> child */
	    exp = props_expand(properties_, xa->data.arg);
	    strnullnorm(exp);
	    if (exp != 0)
		command->appendm(exp);
	    break;

	case xtask_class_t::XT_LINE:	    /* <arg line=""> child */
	    exp = props_expand(properties_, xa->data.arg);
	    strnullnorm(exp);
	    if (exp != 0)
	    	command->split_tom(exp, /*sep*/0);
	    break;
	    /* TODO: <env> child */
	
	case xtask_class_t::XT_FILE:	    /* <arg file=""> child */
	    exp = props_expand(properties_, xa->data.arg);
	    strnullnorm(exp);
	    if (exp != 0)
	    	command->appendm(file_normalise_m(exp, project_->basedir));
	    break;
	    
	case xtask_class_t::XT_FILESET:    /* <fileset> child */
    	    fileset_gather_mapped(xa->data.fileset, properties_,
	    	    	    	  command, /*mappers*/0);
	    break;

	case xtask_class_t::XT_FILES:	    /* <files> child */
	    if (fileset_ != 0)
    	    	fileset_gather_mapped(fileset_, properties_,
		    	    	      command, xtclass->mappers_);
	    break;
	    
	case xtask_class_t::XT_TAGEXPAND:  /* <tagexpand> child */
	    taglist_list_gather(taglists_, xa->data.tagexp,
	    	    	    	properties_, command);
	    break;
	}
    }
    
#if DEBUG
    {
    	char *commstr = command->join("\" \"");
	fprintf(stderr, "xtask_build_command: \"%s\"\n", commstr);
	g_free(commstr);
    }
#endif

    return TRUE;
}



gboolean
xtask_t::execute_command()
{
    xtask_class_t *xtclass = (xtask_class_t *)tclass_;	/* downcast */
    logmsg_t *logmsg = 0;
    strarray_t *command;
    strarray_t *depfiles;
    char *targfile = 0;
    
       
    /* try to build dependency information for the command */

    depfiles = new strarray_t;

    if (xtclass->is_fileset_)
    {
	if (xtclass->foreach_)
	{
	    GList *iter;
	    char *depfile;

	    depfile = props_expand(properties_, "${file}");
	    depfiles->appendm(depfile);
	    
	    for (iter = xtclass->dep_mappers_ ; iter != 0 ; iter = iter->next)
	    {
		mapper_t *ma = (mapper_t *)iter->data;

		if ((targfile = mapper_map(ma, depfile)) != 0)
	    	    break;
	    }
	    /* TODO: expand targfile */
	}
	else
	{
    	    fileset_gather_mapped(fileset_, properties_,
	    	    	    	  depfiles, xtclass->mappers_);

    	    targfile = props_expand(properties_, xtclass->dep_target_);
    	}
    }

    strnullnorm(targfile);
    if (targfile != 0)
	props_set(properties_, "targfile", targfile);


    /* build the command from args and properties */
    command = new strarray_t;
    
    if (!build_command(command))
    {
    	strdelete(targfile);
    	delete command;
    	delete depfiles;
	return FALSE;
    }
    
    
    if (verbose)
    	logmsg = logmsg_newnm(command->join(" "));
    else if (xtclass->logmessage_ != 0)
    	logmsg = logmsg_newnm(props_expand(properties_, xtclass->logmessage_));

    if (targfile == 0)
    {
    	/* no dependency information -- job barrier, serialised */
    	if (!job_t::immediate(new command_job_op_t(command, /*env*/0, logmsg)))
	    result_ = FALSE;
    }
    else
    {
    	/* have dependency information -- schedule job for later */
    	job_t *job;
	int i;
	
	job = job_t::add(targfile, new command_job_op_t(command, /*env*/0, logmsg));
	for (i = 0 ; i < depfiles->len ; i++)
	    job->add_depend(depfiles->nth(i));
    }

    strdelete(targfile);
    delete depfiles;

    return result_;    /* keep going unless failure */
}


gboolean
xtask_t::execute_one(const char *filename, void *userdata)
{
    xtask_t *xtask = (xtask_t *)userdata;
    
    props_set(xtask->properties_, "file", filename);
    return xtask->execute_command();
}

gboolean
xtask_t::exec()
{
    xtask_class_t *xtclass = (xtask_class_t *)tclass_; /* downcast */
    
#if DEBUG
    fprintf(stderr, "xtask_execute: executing \"%s\"\n", name_);
#endif

    /* default result */
    result_ = TRUE;
    
    if (xtclass->is_fileset_)
    {
    	if (xtclass->foreach_)
	{
	    /* run the command once for each file in the fileset */
	    fileset_apply(fileset_, properties_, execute_one, this);
	}
	else
	{
    	    /* run the command just once */
	    execute_command();
	}
    }
    else
    {
    	/* run the command just once */
	execute_command();
    }
    
    /* wipe out any temporary properties */
    props_setm(properties_, "file", 0);
    
    return result_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_class_t::arg_t::arg_t(xtask_class_t::arg_type_t ty)
 :  type(ty)
{
    condition_init(&condition);
}

xtask_class_t::arg_t::~arg_t()
{
    switch (type)
    {
    case XT_VALUE:    	/* <arg value=""> child */
    case XT_LINE:    	/* <arg line=""> child */
    case XT_FILE:    	/* <arg file=""> child */
	strdelete(data.arg);
    	break;
    case XT_FILESET:    /* <fileset> child */
	if (data.fileset != 0)
    	    fileset_unref(data.fileset);
    	break;
    case XT_FILES:  	/* <files> child */
    	break;
    case XT_TAGEXPAND:	/* <tagexpand> child */
    	tagexp_delete(data.tagexp);
    	break;
    }
    
    condition_free(&condition);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_class_t::xtask_class_t(const char *name)
{
    strassign(name_, name);
    property_map_ = props_new(0);
}

xtask_class_t::~xtask_class_t()
{
    listdelete(args_, xtask_class_t::arg_t, delete);
    listdelete(mappers_, mapper_t, mapper_delete);
    listdelete(dep_mappers_, mapper_t, mapper_delete);
    
    strdelete(name_);
    strdelete(executable_);
    strdelete(logmessage_);
    strdelete(dep_target_);
    
    props_delete(property_map_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
xtask_class_t::name() const
{
    return name_;
}

task_t *
xtask_class_t::create_task(project_t *proj)
{
    return new xtask_t(this, proj);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_class_t::arg_t *
xtask_class_t::add_arg(xtask_class_t::arg_type_t type)
{
    arg_t *xa;
    
    xa = new arg_t(type);
    
    args_ = g_list_append(args_, xa);
    
    return xa;
}

xtask_class_t::arg_t *
xtask_class_t::add_line(const char *s)
{
    arg_t *xa;
    
    xa = add_arg(XT_LINE);
    strassign(xa->data.arg, s);
    
    return xa;
}

xtask_class_t::arg_t *
xtask_class_t::add_value(const char *s)
{
    arg_t *xa;
    
    xa = add_arg(XT_VALUE);
    strassign(xa->data.arg, s);
    
    return xa;
}

xtask_class_t::arg_t *
xtask_class_t::add_file(const char *s)
{
    arg_t *xa;
    
    xa = add_arg(XT_FILE);
    strassign(xa->data.arg, s);
    
    return xa;
}

xtask_class_t::arg_t *
xtask_class_t::add_fileset(fileset_t *fs)
{
    arg_t *xa;
    
    xa = add_arg(XT_FILESET);
    xa->data.fileset = fs;
    
    return xa;
}

xtask_class_t::arg_t *
xtask_class_t::add_files()
{
    return add_arg(XT_FILES);
}

xtask_class_t::arg_t *
xtask_class_t::add_tagexpand(tagexp_t *te)
{
    xtask_class_t::arg_t *xa;
    
    xa = add_arg(XT_TAGEXPAND);
    xa->data.tagexp = te;
    
    return xa;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
xtask_class_t::add_attribute(
    const char *attr,
    const char *prop,
    gboolean required)
{
    task_attr_t proto;
    
    proto.name = (char *)attr;
    proto.setter = (gboolean (task_t::*)(const char *, const char *))
    	    	    	&xtask_t::generic_setter;
    proto.flags = (required ? TT_REQUIRED : 0);

    task_class_t::add_attribute(&proto);
    
    props_set(property_map_, attr, prop);
}

/* TODO: add `gboolean required' */

void
xtask_class_t::add_child(
    const char *name)
{
    task_child_t proto;
    
    proto.name = (char *)name;
    proto.adder = (gboolean (task_t::*)(xmlNode *))
    	    	    &xtask_t::generic_adder;
    proto.flags = 0;

    task_class_t::add_child(&proto);
    /* TODO: will xtasks ever have children other than taglists?? */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
