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

#include "project.H"
#include "estring.H"
#include "filename.H"
#include "tok.H"

CVSID("$Id: project.C,v 1.11 2002-04-13 02:30:18 gnb Exp $");

project_t *project_t::globals_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

project_t::project_t(project_t *parent)
{
    if (globals_ == 0)
    {
    	// Can't create any projects unless there's a globals_
	// so first project created becomes globals_.
    	assert(parent == 0);
	globals_ = this;
    }
    else if (parent == 0)
	parent = globals_;
    parent_ = parent;
    
    targets_ = new hashtable_t<const char *, target_t>;
    filesets_ = new hashtable_t<const char*, fileset_t>;
    taglists_ = new hashtable_t<char*, taglist_t>;
    tl_defs_ = new hashtable_t<const char*, tl_def_t>;
    tscope_ = new task_scope_t(parent == 0 ? task_scope_t::builtins : parent->tscope_);
    
    properties_ = new props_t((parent == 0 ? 0 : parent->properties_));
    fixed_properties_ = new props_t(properties_);

    fixed_properties_->set("ant.version", VERSION);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
project_delete_one_target(const char *key, target_t *value, void *userdata)
{
    delete value;
    return TRUE;    /* so remove it already */
}

static gboolean
project_unref_one_fileset(const char *key, fileset_t *value, void *userdata)
{
    value->unref();
    return TRUE;    /* so remove it already */
}

static gboolean
project_delete_one_taglist(char *key, taglist_t *value, void *userdata)
{
    g_free(key);
    value->unref();
    return TRUE;    /* so remove it already */
}

static gboolean
project_delete_one_tl_def(const char *key, tl_def_t *value, void *userdata)
{
    delete value;
    return TRUE;    /* so remove it already */
}

project_t::~project_t()
{
    strdelete(name_);
    strdelete(description_);
    
    targets_->foreach_remove(project_delete_one_target, 0);
    delete targets_;
    delete tscope_;
    
    filesets_->foreach_remove(project_unref_one_fileset, 0);
    delete filesets_;
    
    tl_defs_->foreach_remove(project_delete_one_tl_def, 0);
    delete tl_defs_;
    
    taglists_->foreach_remove(project_delete_one_taglist, 0);
    delete taglists_;
    
    delete properties_;
    delete fixed_properties_;
    
    if (globals_ == this)
    	globals_ = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_t::set_name(const char *s)
{
    strassign(name_, s);
    fixed_properties_->set("ant.project.name", name_);
}

void
project_t::set_description(const char *s)
{
    strassign(description_, s);
}

void
project_t::set_default_target(const char *s)
{
    strassign(default_target_, s);
}

/*
 * Setup the magical variables _PATHUP and _PATHDOWN
 */
void
project_t::update_magic_paths()
{
    const char *d;
    estring path;
    tok_t tok(file_normalise(basedir_, 0), "/");

    /* TODO: We need something like a file_denormalise() */
    
    while ((d = tok.next()) != 0)
	path.append_string((!strcmp(d, ".") ? "./" : "../"));
    
    set_property("_pathup", path.data());
    set_property("topdir", path.data());
    
    path.truncate();
    path.append_string(basedir_);
    if (path.length())
    	path.append_char('/');
    set_property("_pathdown", path.data());
}


void
project_t::set_basedir(const char *s)
{
    if (parent_ != 0)
    {
	strdelete(basedir_);
	basedir_ = file_normalise(s, parent_->basedir_);
    }
    else
    {
    	strassign(basedir_, s);
    }
    fixed_properties_->set("basedir", basedir_);
    update_magic_paths();
}

void
project_t::set_filename(const char *s)
{
    strassign(filename_, s);
    fixed_properties_->setm("ant.file", file_normalise(filename_, 0));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_t::override_properties(props_t *props)
{
    properties_->copy_contents(props);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

target_t *
project_t::find_target(const char *name) const
{
    return targets_->lookup(name);
}

void
project_t::remove_target(target_t *targ)
{
    targets_->remove(targ->name());
    targ->set_project(0);
}

void
project_t::add_target(target_t *targ)
{
    assert(targ != 0);
    assert(targ->name() != 0);
    targets_->insert(targ->name(), targ);
    targ->set_project(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

task_class_t *
project_t::find_task_class(const char *name) const
{
    return tscope_->find(name);
}

void
project_t::add_task_class(task_class_t *tclass)
{
    tscope_->add(tclass);
}

void
project_t::remove_task_class(task_class_t *tclass)
{
    tscope_->remove(tclass);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tl_def_t *
project_t::find_tl_def(const char *name) const
{
    tl_def_t *tldef;
    const project_t *proj;
    
    for (proj = this ; proj != 0 ; proj = proj->parent_)
    {
    	if ((tldef = proj->tl_defs_->lookup(name)) != 0)
	    return tldef;
    }
    
    return 0;
}

void
project_t::add_tl_def(tl_def_t *tldef)
{
    tl_defs_->insert(tldef->name_space(), tldef);
}

void
project_t::remove_tl_def(tl_def_t *tldef)
{
    tl_defs_->remove(tldef->name_space());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define build_taglist_key(name_space, id) \
    g_strconcat((name_space), "::", (id), 0)

taglist_t *
project_t::find_taglist(const char *name_space, const char *id) const
{
    char *key = build_taglist_key(name_space, id);
    taglist_t *tl;
    const project_t *proj;
    
    for (proj = this ; proj != 0 ; proj = proj->parent_)
    {
    	if ((tl = proj->taglists_->lookup(key)) != 0)
	    break;
    }
    
    g_free(key);
    return tl;
}

void
project_t::add_taglist(taglist_t *tl)
{
    char *key = build_taglist_key(tl->name_space(), tl->id());
    
    taglists_->insert(key, tl);
}

void
project_t::remove_taglist(taglist_t *tl)
{
    char *key = build_taglist_key(tl->name_space(), tl->id());
    char *okey = 0;
    taglist_t *ovalue = 0;
    
    taglists_->lookup_extended(key, &okey, &ovalue);
    assert(okey != 0);
    assert(tl == ovalue);
    
    taglists_->remove(key);

    g_free(key);
    g_free(okey);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_t::set_property(const char *name, const char *value)
{
    properties_->set(name, value);
}

void
project_t::set_propertym(const char *name, char *value)
{
    properties_->setm(name, value);
}

void
project_t::append_property(const char *name, const char *value)
{
    const char *oldval = properties_->get(name);
    if (oldval == 0)
	properties_->set(name, value);
    else
	properties_->setm(name, g_strconcat(oldval, value, 0));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t *
project_t::find_fileset(const char *id) const
{
    return filesets_->lookup(id);
}

void
project_t::add_fileset(fileset_t *fs)
{
    assert(fs->id() != 0);
    filesets_->insert(fs->id(), fs);
}

void
project_t::remove_fileset(fileset_t *fs)
{
    filesets_->remove(fs->id());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
check_one_target(const char *key, target_t *targ, void *userdata)
{
    gboolean *successp = (gboolean *)userdata;
    
    if (targ->is_depended_on() && !targ->is_defined())
    {
    	log::errorf("Target \"%s\" is depended on but never defined\n",
	    	    	targ->name());
	*successp = FALSE;
    }
}


gboolean
project_t::check_dangling_targets() const
{
    gboolean success = TRUE;
    
    targets_->foreach(check_one_target, &success);
    
    return success;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
project_t::execute_target_by_name(const char *name)
{
    target_t *targ;
    
    if ((targ = find_target(name)) == 0)
    {
    	log::errorf("no such target \"%s\"\n", name);
    	return FALSE;
    }
    return targ->execute();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
