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

#ifndef _task_h_
#define _task_h_ 1

#include "common.H"
#include "estring.H"
#include "props.H"
#include "strarray.H"
#include "pattern.H"
#include "filename.H"
#include "fileset.H"
#include "condition.H"
#include "mapper.H"
#include "xml.H"
#include "log.H"
#include "hashtable.H"
#include "list.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
class project_t;
class target_t;
class task_t;
class task_class_t;


#define TT_REQUIRED 	(1<<0)

struct task_attr_t
{
    char *name;
    gboolean (task_t::*setter)(const char *name, const char *value);
    unsigned flags;
};
#define TASK_ATTR(scope, name, flags) \
    { \
    	g_string(name), \
	(gboolean (task_t::*)(const char*, const char *)) \
	    &scope ## _task_t::set_ ## name, \
	flags \
    }

struct task_child_t
{
    char *name;
    gboolean (task_t::*adder)(xml_node_t *);
    unsigned flags;
};
#define TASK_CHILD(scope, name, flags) \
    { \
    	g_string(name), \
	(gboolean (task_t::*)(xml_node_t *)) &scope ## _task_t::add_ ## name, \
	flags \
    }
#define TASK_GENERIC_CHILD(scope, name, flags) \
    { \
    	"*", \
	(gboolean (task_t::*)(xml_node_t *)) &scope ## _task_t::add_ ## name, \
	flags \
    }

class task_t
{
protected:  
    string_var id_;
    string_var name_;
    string_var description_;

    // TODO: move to derived composite_task_t
    list_t<task_t> subtasks_;

    task_class_t *tclass_;
    fileset_t *fileset_;     /* for directory-based tasks */
    project_t *project_;
    target_t *target_;

    virtual gboolean exec() = 0;
      
public:
    /* ctor */
    task_t(task_class_t *, project_t *);
    /* dtor */
    virtual ~task_t();

    virtual gboolean set_content(const char *);	    /* node content */
    virtual gboolean post_parse();	    	    /* post-parsing check */

    const char *id() const { return id_; }
    void set_id(const char *s) { id_ = s; }

    const char *name() const { return name_; }    
    void set_name(const char *s) { name_ = s; }
    
    const char *description() const { return description_; }    
    void set_description(const char *s) { description_ = s; }

    fileset_t *fileset() const { return fileset_; }
    void set_fileset(fileset_t *fs);
    
    // TODO: move into derived composite_task_t
    void add_subtask(task_t *subtask);
    
    // TODO: override in composite_task_t
    virtual void attach(target_t *);
    
    gboolean set_attribute(const char *name, const char *value);
    
    gboolean execute();
    gboolean execute_subtasks();

    char *expand(const char *str) const;
};

class task_class_t
{
private:
    hashtable_t<const char*, task_attr_t> *attrs_hashed_; /* task_attr_t's hashed on name */
    hashtable_t<const char*, task_child_t> *children_hashed_;    /* task_child_t's hashed on name */

protected:
    /*
     * These fields are protected so they can be initialised by derived
     * classes created by the TASK_DEFINE_CLASS macro.
     */
    task_attr_t *attrs_;     	    	    /* attributes and setter methods */
    task_child_t *children_;     	    /* child elements and adder methods */
    gboolean is_fileset_;    	    	    /* parse as a fileset */
    char *fileset_dir_name_; 	    	    /* name of base dir property */
    gboolean is_composite_;  	    	    /* accepts subtasks */

public:
    /* normal ctor */
    task_class_t();
    /* ctor for the TASK_DEFINE_CLASS macro */
    task_class_t(
    	task_attr_t *attrs,
	task_child_t *children,
	gboolean is_fileset,
	char *fileset_dir_name,
	gboolean is_composite)
     :  attrs_(attrs),
     	children_(children),
    	is_fileset_(is_fileset),
	fileset_dir_name_(fileset_dir_name),
	is_composite_(is_composite)
    {
    }

    /* dtor */
    virtual ~task_class_t();

    virtual void init();	    	    /* static ctor */
    virtual const char *name() const = 0;
    virtual task_t *create_task(project_t *) = 0;
    virtual void cleanup();    	    	    /* static dtor */

    void attributes_apply(
	void (*function)(const task_attr_t *ta, void *userdata), void *userdata);
    /* no task should ever need to call this */
    void add_attribute(const task_attr_t *ta);
    const task_attr_t *find_attr(const char *name) const;

    void add_child(const task_child_t *tc);
    const task_child_t *find_child(const char *name) const;

    gboolean is_fileset() const { return is_fileset_; }
    void set_is_fileset(gboolean b) { is_fileset_ = b; }
    
    char *fileset_dir_name() const { return fileset_dir_name_; }
    void set_fileset_dir_name(const char * s) { strassign(fileset_dir_name_, s); }

    gboolean is_composite() const { return is_composite_; }
    void set_is_composite(gboolean b) { is_composite_ = b; }
};


#define TASK_DEFINE_CLASS_BEGIN(nm, attrs, children, is_fileset, fileset_dir_name, is_composite) \
 \
class nm##_task_class_t : public task_class_t \
{ \
public: \
    nm##_task_class_t() \
     :  task_class_t((attrs), (children), (is_fileset), (fileset_dir_name), (is_composite)) \
    { \
    } \
    ~nm##_task_class_t() \
    { \
    } \
    const char *name() const \
    { \
    	return #nm ; \
    } \
    task_t *create_task(project_t *proj) \
    { \
    	return new nm##_task_t(this, proj); \
    }
#define TASK_DEFINE_CLASS_END(nm) \
}; \
nm##_task_class_t nm##_task_class;

#define TASK_DECLARE_CLASS(nm) \
 \
class nm##_task_class_t : public task_class_t \
{ \
public: \
    nm##_task_class_t(); \
    ~nm##_task_class_t(); \
    const char *name() const; \
    task_t *create_task(project_t *proj); \
}; \
extern nm##_task_class_t nm##_task_class




/* a task_scope_t is a stackable scope for task_ops_t definitions */
class task_scope_t
{
private:
    task_scope_t *parent_;
    hashtable_t<const char *, task_class_t> *taskdefs_;
    
public:
    task_scope_t(task_scope_t *parent);
    ~task_scope_t();
    
    gboolean add(task_class_t *);
    void remove(task_class_t *);
    task_class_t *find(const char *name) const;

    static void initialise_builtins();
    static void cleanup_builtins();
    static task_scope_t *builtins;
};


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _task_h_ */
