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

#ifndef _project_h_
#define _project_h_ 1

#include "common.H"
#include "props.H"
#include "fileset.H"
#include "hashtable.H"
#include "task.H"
#include "taglist.H"
#include "target.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class project_t
{
private:
    char *name_;
    char *description_;
    char *filename_;
    char *default_target_;
    char *basedir_;
    project_t *parent_;	    	/* inherits tscope, props etc */
    
    hashtable_t<const char*, target_t> *targets_;
    hashtable_t<const char*, fileset_t> *filesets_;   	/* <fileset>s in project scope */
    hashtable_t<char*, taglist_t> *taglists_;   	/* taglists in project scope */
    
    hashtable_t<const char*, tl_def_t> *tl_defs_;   	/* all taglistdefs */
    task_scope_t *tscope_;   	/* scope for taskdefs */
    
    props_t *fixed_properties_;	/* e.g. "basedir" which can't be overridden */
    props_t *properties_;    	/* mutable properties from <property> element */

    static project_t *globals_;

    void update_magic_paths();

public:
    project_t(project_t *parent);
    ~project_t();

    const char *name() const { return name_; }    
    void set_name(const char *name);
    
    const char *description() const { return description_; }
    void set_description(const char *description);

    const char *default_target() const { return default_target_; }    
    void set_default_target(const char *s);
    
    const char *basedir() const { return basedir_; }
    void set_basedir(const char *s);
    
    const char *filename() const { return filename_; }
    void set_filename(const char *s);

    target_t *find_target(const char *name) const;
    void add_target(target_t*);
    void remove_target(target_t *targ);
    
    task_class_t *find_task_class(const char *name) const;
    void add_task_class(task_class_t *);
    void remove_task_class(task_class_t *);

    tl_def_t *find_tl_def(const char *name) const;
    void add_tl_def(tl_def_t *tld);
    void remove_tl_def(tl_def_t *tld);

    taglist_t *find_taglist(const char *name_space, const char *id) const;
    void add_taglist(taglist_t*);
    void remove_taglist(taglist_t *);

    const char *get_property(const char *name) const
        { return fixed_properties_->get(name); }
    void set_property(const char *name, const char *value);
    void set_propertym(const char *name, char *value);
    void append_property(const char *name, const char *value);
    void override_properties(props_t *props);
    gboolean read_property_shellfile(const char *filename)
    	{ return properties_->read_shellfile(filename); }
    const props_t *properties() const { return fixed_properties_; }
    char *expand(const char *str) const { return fixed_properties_->expand(str); }

    fileset_t *find_fileset(const char *id) const;
    void add_fileset(fileset_t *);
    void remove_fileset(fileset_t *);
    
    gboolean check_dangling_targets() const;
    
    gboolean execute_target_by_name(const char *);
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _project_h_ */
