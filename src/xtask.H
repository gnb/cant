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

#ifndef _cant_xtask_h_
#define _cant_xtask_h_ 1

#include "cant.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class xtask_t;

class xtask_class_t : public task_class_t
{
public:
    xtask_class_t(const char *name);
    ~xtask_class_t();

    const char *name() const;
    task_t *create_task(project_t *);

    struct arg_t
    {
	condition_t condition;
	
	arg_t();
	virtual ~arg_t();
	
    	virtual gboolean command_add(const xtask_t *, strarray_t *) const = 0;
#if DEBUG
	virtual void dump() const;
#endif
    };


    /* <arg line=""> argument is split on whitespace */
    arg_t *add_line(const char *s);
    /* <arg value=""> argument used directly, whitespace-safe */
    arg_t *add_value(const char *s);
    /* <arg file=""> agument is filename, to be canonicalised & absoluted. */
    arg_t *add_file(const char *s);
    /* <fileset> insert list of files from fileset child. */
    arg_t *add_fileset(fileset_t *fs);
    /* <files> insert list of files from task's own fileset */
    arg_t *add_files();
    /* <tagexpand> expand tags from xtask's child taglists */
    arg_t *add_tagexpand(tagexp_t *te);
    /* TODO: <filelist> */
    /* TODO: <env> */
    
private:
    string_var name_;
    string_var executable_;
    string_var logmessage_;
    list_t<arg_t> args_;
    props_t *property_map_;	/* maps attributes to local property *name*s */
    list_t<mapper_t> mappers_;	/* list of mapper_t: args to files */
    list_t<mapper_t> dep_mappers_; /* list of mapper_t: depfiles to targfiles */
    string_var dep_target_;
    
    gboolean foreach_:1;

    friend class xtask_t;

public:

    void set_executable(const char *s) { executable_ = s; }
    void set_logmessage(const char *s) { logmessage_ = s; }
    void set_foreach(gboolean b) { foreach_ = b; }
    void set_dep_target(const char *s) { dep_target_ = s; }

    void add_attribute(const char *attr, const char *prop, gboolean required);
    void add_child(const char *name);

    const list_t<mapper_t> *mappers() const { return &mappers_; }    
    void add_mapper(mapper_t *ma)
    {
    	mappers_.append(ma);
    }
    const list_t<mapper_t> *dep_mappers() const { return &dep_mappers_; }
    void add_dep_mapper(mapper_t *ma)
    {
    	dep_mappers_.append(ma);
    }



    
private:
    arg_t *add_arg(arg_t *xa);
};


class xtask_t : public task_t
{
private:
    gboolean result_;
    props_t *properties_;   /* local properties, overriding the project */
    list_t<taglist_t> taglists_;

    xtask_t(task_class_t *, project_t *);
    ~xtask_t();
    
    gboolean generic_setter(const char *name, const char *value);
    gboolean generic_adder(xml_node_t *node);
    
    gboolean build_command(strarray_t *command);
    gboolean execute_command();
    static gboolean execute_one(const char *filename, void *userdata);

    gboolean exec();

    friend class xtask_class_t;
public:

    xtask_class_t *xtask_class() const
    	{ return (xtask_class_t *)tclass_; /* downcast */ }

    const props_t *properties() const { return properties_; }
    const list_t<taglist_t> *taglists() const { return &taglists_; }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_xtask_h_ */
