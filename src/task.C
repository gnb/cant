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

CVSID("$Id: task.C,v 1.6 2002-04-12 13:07:24 gnb Exp $");


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_t::task_t(task_class_t *tclass, project_t *proj)
{
    tclass_ = tclass;
    project_ = proj;
    set_name(tclass->name());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_t::~task_t()
{
    strdelete(id_);
    strdelete(name_);
    strdelete(description_);
    
    if (fileset_ != 0)
    	fileset_->unref();

    subtasks_.delete_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_t::add_subtask(task_t *subtask)
{
    assert(tclass_->is_composite());
    subtasks_.append(subtask);
}

void
task_t::set_fileset(fileset_t *fs)
{
    // TODO: refcount filesets
    // TODO: refcounted_t class, _var smart pointer
    if (fileset_ != 0)
    	fileset_->unref();
    fileset_ = fs;
    if (fileset_ != 0)
    	fileset_->ref();
}

gboolean
task_t::set_content(const char *content)
{
    return FALSE;   /* base class doesn't know what to do with content */
}

gboolean
task_t::post_parse()
{
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_t::attach(target_t *targ)
{
    list_iterator_t<task_t> iter;
    
    target_ = targ;
    for (iter = subtasks_.first() ; iter != 0 ; ++iter)
	(*iter)->attach(targ);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
task_t::set_attribute(const char *name, const char *value)
{
    const task_attr_t *ta = 0;

    if ((ta = tclass_->find_attr(name)) == 0)
    	return FALSE;

    return (this->*ta->setter)(name, value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
task_t::execute()
{
    /* TODO: filename, linenumber?? */
    log_tree_context_t context(name_);
        
    if (!exec())
    {
    	/* TODO: be more gracious, e.g. for <condition> */
    	log::errorf("FAILED\n");
	return FALSE;
    }

    return TRUE;
}

gboolean
task_t::execute_subtasks()
{
    list_iterator_t<task_t> iter;

    for (iter = subtasks_.first() ; iter != 0 ; ++iter)
    {
	task_t *subtask = *iter;

	if (!subtask->execute())
	    return FALSE;
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
task_t::expand(const char *str) const
{
    return project_expand(project_, str);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_class_t::task_class_t()
{
}

static gboolean
remove_one_attr(const char *key, task_attr_t *ta, void *userdata)
{
    g_free(ta->name);
    g_free(ta);

    return TRUE;    /* remove me */
}

static gboolean
remove_one_child(const char *key, task_child_t *tc, void *userdata)
{
    g_free(tc->name);
    g_free(tc);
    
    return TRUE;    /* remove me */
}

task_class_t::~task_class_t()
{
    if (attrs_hashed_ != 0)
    {
	attrs_hashed_->foreach_remove(remove_one_attr, 0);
	delete attrs_hashed_;
    }

    if (children_hashed_ != 0)
    {
	children_hashed_->foreach_remove(remove_one_child, 0);
	delete children_hashed_;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_class_t::add_attribute(const task_attr_t *proto)
{
    task_attr_t *ta;
    
    /* TODO: somehow we have to delete these again!! */
    ta = new(task_attr_t);
    *ta = *proto;
    ta->name = g_strdup(ta->name);
    
    if (attrs_hashed_ == 0)
	attrs_hashed_ = new hashtable_t<const char *, task_attr_t>;
    attrs_hashed_->insert(ta->name, ta);
}

const task_attr_t *
task_class_t::find_attr(const char *name) const
{
    task_attr_t *ta;
    
    if (attrs_hashed_ == 0)
    	return 0;
    if ((ta = attrs_hashed_->lookup(name)) != 0)
    	return ta;
    return attrs_hashed_->lookup("*");
}


typedef struct
{
    void (*function)(const task_attr_t *ta, void *userdata);
    void *userdata;
} task_class_foreach_attr_rec_t;

static void
task_class_attributes_apply_one(const char *key, task_attr_t *ta, void *userdata)
{
    task_class_foreach_attr_rec_t *recp = (task_class_foreach_attr_rec_t *)userdata;
    
    (*recp->function)(ta, recp->userdata);
}

void
task_class_t::attributes_apply(
    void (*function)(const task_attr_t *ta, void *userdata),
    void *userdata)
{
    if (attrs_hashed_ != 0)
    {
    	task_class_foreach_attr_rec_t rec;
	
	rec.function = function;
	rec.userdata = userdata;
	
    	attrs_hashed_->foreach(task_class_attributes_apply_one, &rec);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const task_child_t *
task_class_t::find_child(const char *name) const
{
    task_child_t *tc;
    
    if (children_hashed_ == 0)
    	return 0;
    if ((tc = children_hashed_->lookup(name)) != 0)
    	return tc;
    return children_hashed_->lookup("*");
}

void
task_class_t::add_child(const task_child_t *proto)
{
    task_child_t *tc;
    
    /* TODO: somehow we have to delete these again!! */
    tc = new(task_child_t);
    *tc = *proto;
    tc->name = g_strdup(tc->name);
    
    if (children_hashed_ == 0)
	children_hashed_ = new hashtable_t<const char *, task_child_t>;
    children_hashed_->insert(tc->name, tc);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_class_t::init()
{
    if (attrs_ != 0)
    {
	task_attr_t *ta;

	for (ta = attrs_ ; ta->name != 0 ; ta++)
	    add_attribute(ta);
    }
    
    if (children_ != 0)
    {
	task_child_t *tc;

	for (tc = children_ ; tc->name != 0 ; tc++)
	    add_child(tc);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_class_t::cleanup()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_scope_t::task_scope_t(task_scope_t *parent)
{
    parent_ = parent;
    taskdefs_ = new hashtable_t<const char *, task_class_t>;
}

static gboolean
cleanup_one_taskdef(const char *key, task_class_t *value, void *userdata)
{
    value->cleanup();
    return TRUE;    /* remove me */
}

task_scope_t::~task_scope_t()
{
    taskdefs_->foreach_remove(cleanup_one_taskdef, 0);
    delete taskdefs_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
task_scope_t::add(task_class_t *tclass)
{
    if (taskdefs_->lookup(tclass->name()) != 0)
    {
    	log::errorf("Task operations \"%s\" already registered, ignoring new definition\n",
	    	tclass->name());
    	return FALSE;
    }
    
    taskdefs_->insert(tclass->name(), tclass);
#if DEBUG
    fprintf(stderr, "task_scope_t::register_class: registering \"%s\"\n",
    	    	tclass->name());
#endif

    tclass->init();
	
    return TRUE;
}

void
task_scope_t::remove(task_class_t *tclass)
{
    taskdefs_->remove(tclass->name());
}

task_class_t *
task_scope_t::find(const char *name) const
{
    const task_scope_t *ts;
    
    for (ts = this ; ts != 0 ; ts = ts->parent_)
    {
    	task_class_t *tclass;
	
	if ((tclass = ts->taskdefs_->lookup(name)) != 0)
	    return tclass;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define TASK_CLASS(t)  	TASK_DECLARE_CLASS(t);
#include "builtin-tasks.H"
#undef TASK_CLASS

task_scope_t *task_scope_t::builtins;

void
task_scope_t::initialise_builtins()
{
    builtins = new task_scope_t(/*parent*/0);
    
#define TASK_CLASS(t)  builtins->add(&t##_task_class);
#include "builtin-tasks.H"
#undef TASK_CLASS
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
