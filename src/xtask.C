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

CVSID("$Id: xtask.C,v 1.16 2002-04-21 04:01:40 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_t::xtask_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
    /* TODO: delay attachment to project? */
    properties_ = new props_t(project_->properties());
}

xtask_t::~xtask_t()
{
#if DEBUG
    fprintf(stderr, "~xtask_t: deleting \"%s\"\n", name());
#endif

    taglists_.apply_remove(unref);
    
    delete properties_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
xtask_t::generic_setter(const char *name, const char *value)
{
    const char *propname;
    
    if ((propname = xtask_class()->property_map_->get(name)) == 0)
    	return FALSE;
    properties_->set(propname, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
xtask_t::generic_adder(xml_node_t *node)
{
    taglist_t *tl;
    
    if ((tl = parse_taglist(project_, node)) == 0)
    	return FALSE;
    
    taglists_.append(tl);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: build env too */
gboolean
xtask_t::build_command(strarray_t *command)
{
    xtask_class_t *xtclass = xtask_class();
    list_iterator_t<xtask_class_t::arg_t> iter;
    string_var exp;

    if (xtclass->executable_ != 0)
    	command->appendm(properties_->expand(xtclass->executable_));
    
    for (iter = xtclass->args_.first() ; iter != 0 ; ++iter)
    {
    	xtask_class_t::arg_t *xa = *iter;

    	if (!xa->condition.evaluate(properties_))
	    continue;
	
	if (!xa->command_add(this, command))
	    return FALSE;
    }
    
#if DEBUG
    string_var commstr = command->join("\" \"");
    fprintf(stderr, "xtask_t::build_command: \"%s\"\n", commstr.data());
#endif

    return TRUE;
}



gboolean
xtask_t::execute_command()
{
    xtask_class_t *xtclass = xtask_class();
    log_message_t *logmsg = 0;
    strarray_t *command;
    strarray_t *depfiles;
    string_var targfile;
    
       
    /* try to build dependency information for the command */

    depfiles = new strarray_t;

    if (xtclass->is_fileset_)
    {
	if (xtclass->foreach_)
	{
	    list_iterator_t<mapper_t> iter;
	    char *depfile;

	    depfile = properties_->expand("${file}");
	    depfiles->appendm(depfile);
	    
	    for (iter = xtclass->dep_mappers_.first() ; iter != 0 ; ++iter)
	    {
		if ((targfile = (*iter)->map(depfile)) != 0)
	    	    break;
	    }
	    /* TODO: expand targfile */
	}
	else
	{
    	    fileset_->gather_mapped(properties_,
	    	    	    	  depfiles, &xtclass->mappers_);

    	    targfile = properties_->expand(xtclass->dep_target_);
    	}
    }

    if (!targfile.is_null())
	properties_->set("targfile", targfile);

    runner_t *runner = runner_t::create(xtclass->runmode_);
    runner->setup_properties(properties_);

    /* build the command from args and properties */
    command = new strarray_t;
    
    if (!build_command(command))
    {
    	delete command;
    	delete depfiles;
	return FALSE;
    }
    
    
    if (verbose)
    	logmsg = new log_message_t(command->join(" "), /*addnl*/TRUE);
    else if (xtclass->logmessage_ != 0)
    	logmsg = new log_message_t(properties_->expand(xtclass->logmessage_),
	    	    	    	   /*addnl*/TRUE);

    runner->set_command(command);
    job_op_t *jobop = new command_job_op_t(logmsg, runner);

    if (targfile == 0)
    {
    	/* no dependency information -- job barrier, serialised */
    	if (!job_t::immediate(jobop))
	    result_ = FALSE;
    }
    else
    {
    	/* have dependency information -- schedule job for later */
    	job_t *job;
	unsigned int i;
	
	job = job_t::add(targfile, jobop);
	for (i = 0 ; i < depfiles->len ; i++)
	    job->add_depend(depfiles->nth(i));
	job->add_saved_depends();
    }

    delete depfiles;

    return result_;    /* keep going unless failure */
}


gboolean
xtask_t::execute_one(const char *filename, void *userdata)
{
    xtask_t *xtask = (xtask_t *)userdata;
    
    xtask->properties_->set("file", filename);
    return xtask->execute_command();
}

gboolean
xtask_t::exec()
{
    xtask_class_t *xtclass = xtask_class();
    
#if DEBUG
    fprintf(stderr, "xtask_execute: executing \"%s\"\n", name());
#endif

    /* default result */
    result_ = TRUE;
    
    if (xtclass->is_fileset_)
    {
    	if (xtclass->foreach_)
	{
	    /* run the command once for each file in the fileset */
	    fileset_->apply(properties_, execute_one, this);
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
    properties_->setm("file", 0);
    
    return result_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_class_t::arg_t::arg_t()
{
}

xtask_class_t::arg_t::~arg_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

	/* TODO: <arg arglistref=""> child */

class xtask_value_arg_t : public xtask_class_t::arg_t
{
private:
    string_var value_;
    
public:
    xtask_value_arg_t(const char *s)
     :  value_(s)
    {
    }
    ~xtask_value_arg_t()
    {
    }
    
    gboolean command_add(const xtask_t *xtask, strarray_t *command) const
    {
	string_var exp = xtask->properties()->expand(value_);
	if (!exp.is_null())
	    command->appendm(exp.take());
    	return TRUE;
    }

#if DEBUG
    void dump() const
    {
    	fprintf(stderr, "VALUE=\"%s\"\n", value_.data());
    }
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
	    /* TODO: <env> child */

class xtask_line_arg_t : public xtask_class_t::arg_t
{
private:
    string_var line_;
    
public:
    xtask_line_arg_t(const char *s)
     :  line_(s)
    {
    }
    ~xtask_line_arg_t()
    {
    }
    
    gboolean command_add(const xtask_t *xtask, strarray_t *command) const
    {
	string_var exp = xtask->properties()->expand(line_);
	if (!exp.is_null())
	    command->split_tom(exp.take(), /*sep*/0);
    	return TRUE;
    }

#if DEBUG
    void dump() const
    {
    	fprintf(stderr, "LINE=\"%s\"\n", line_.data());
    }
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class xtask_file_arg_t : public xtask_class_t::arg_t
{
private:
    string_var file_;
    
public:
    xtask_file_arg_t(const char *s)
     :  file_(s)
    {
    }
    ~xtask_file_arg_t()
    {
    }
    
    gboolean command_add(const xtask_t *xtask, strarray_t *command) const
    {
	string_var exp = xtask->properties()->expand(file_);
	if (!exp.is_null())
	    command->appendm(file_normalise_m(exp.take(),
	    	    	     xtask->project()->basedir()));
    	return TRUE;
    }

#if DEBUG
    void dump() const
    {
    	fprintf(stderr, "FILE=\"%s\"\n", file_.data());
    }
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class xtask_fileset_arg_t : public xtask_class_t::arg_t
{
private:
    fileset_t *fileset_;
    
public:
    xtask_fileset_arg_t(fileset_t *fs)
     :  fileset_(fs)
    {
	// TODO: ref()
    }
    ~xtask_fileset_arg_t()
    {
    	fileset_->unref();
    }
    
    gboolean command_add(const xtask_t *xtask, strarray_t *command) const
    {
    	fileset_->gather_mapped(xtask->properties(), command, /*mappers*/0);
    	return TRUE;
    }

#if DEBUG
    void dump() const
    {
    	fprintf(stderr, "FILESET={\n");
	fileset_->dump();
	fprintf(stderr, "}\n");
    }
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class xtask_files_arg_t : public xtask_class_t::arg_t
{
public:
    xtask_files_arg_t()
    {
    }
    ~xtask_files_arg_t()
    {
    }
    
    gboolean command_add(const xtask_t *xtask, strarray_t *command) const
    {
	if (xtask->fileset() != 0)
    	    xtask->fileset()->gather_mapped(xtask->properties(),
		    	    		    command,
					    xtask->xtask_class()->mappers());
    	return TRUE;
    }

#if DEBUG
    void dump() const
    {
    	fprintf(stderr, "FILES\n");
    }
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class xtask_tagexp_arg_t : public xtask_class_t::arg_t
{
private:
    tagexp_t *tagexp_;
    
public:
    xtask_tagexp_arg_t(tagexp_t *te)
     :  tagexp_(te)
    {
    }
    ~xtask_tagexp_arg_t()
    {
    	delete tagexp_;
    }
    
    gboolean command_add(const xtask_t *xtask, strarray_t *command) const
    {
	taglist_t::list_gather(xtask->taglists(), tagexp_,
	    	    	    xtask->properties(), command);
    	return TRUE;
    }

#if DEBUG
    void dump() const
    {
    	fprintf(stderr, "tagexp=\n");
    }
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_class_t::xtask_class_t(const char *name)
{
    name_ = name;
    property_map_ = new props_t(0);
    runmode_ = "simple";
}

xtask_class_t::~xtask_class_t()
{
    args_.delete_all();
    mappers_.delete_all();
    dep_mappers_.delete_all();
    delete property_map_;
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
xtask_class_t::add_arg(xtask_class_t::arg_t *xa)
{
    args_.append(xa);
    return xa;
}

xtask_class_t::arg_t *
xtask_class_t::add_line(const char *s)
{
    return add_arg(new xtask_line_arg_t(s));
}

xtask_class_t::arg_t *
xtask_class_t::add_value(const char *s)
{
    return add_arg(new xtask_value_arg_t(s));
}

xtask_class_t::arg_t *
xtask_class_t::add_file(const char *s)
{
    return add_arg(new xtask_file_arg_t(s));
}

xtask_class_t::arg_t *
xtask_class_t::add_fileset(fileset_t *fs)
{
    return add_arg(new xtask_fileset_arg_t(fs));
}

xtask_class_t::arg_t *
xtask_class_t::add_files()
{
    return add_arg(new xtask_files_arg_t());
}

xtask_class_t::arg_t *
xtask_class_t::add_tagexpand(tagexp_t *te)
{
    return add_arg(new xtask_tagexp_arg_t(te));
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
    
    property_map_->set(attr, prop);
}

/* TODO: add `gboolean required' */

void
xtask_class_t::add_child(
    const char *name)
{
    task_child_t proto;
    
    proto.name = (char *)name;
    proto.adder = (gboolean (task_t::*)(xml_node_t *))
    	    	    &xtask_t::generic_adder;
    proto.flags = 0;

    task_class_t::add_child(&proto);
    /* TODO: will xtasks ever have children other than taglists?? */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
