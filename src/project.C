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
#include "tok.H"

CVSID("$Id: project.C,v 1.8 2002-04-07 08:28:51 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

project_t *
project_new(project_t *parent)
{
    project_t *proj;
    
    proj = new(project_t);
    
    proj->parent = parent;
    
    proj->targets = new hashtable_t<const char *, target_t>;
    proj->filesets = new hashtable_t<const char*, fileset_t>;
    proj->taglists = new hashtable_t<char*, taglist_t>;
    proj->tl_defs = new hashtable_t<const char*, tl_def_t>;
    proj->tscope = new task_scope_t(parent == 0 ? task_scope_t::builtins : parent->tscope);
    
    proj->properties = new props_t((parent == 0 ? 0 : parent->properties));
    proj->fixed_properties = new props_t(proj->properties);

    proj->fixed_properties->set("ant.version", VERSION);
    	
    return proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
project_delete_one_target(const char *key, target_t *value, void *userdata)
{
    target_delete(value);
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

void
project_delete(project_t *proj)
{
    strdelete(proj->name);
    strdelete(proj->description);
    
    proj->targets->foreach_remove(project_delete_one_target, 0);
    delete proj->targets;
    delete proj->tscope;
    
    proj->filesets->foreach_remove(project_unref_one_fileset, 0);
    delete proj->filesets;
    
    proj->tl_defs->foreach_remove(project_delete_one_tl_def, 0);
    delete proj->tl_defs;
    
    proj->taglists->foreach_remove(project_delete_one_taglist, 0);
    delete proj->taglists;
    
    delete proj->properties;
    delete proj->fixed_properties;
    
    g_free(proj);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_set_name(project_t *proj, const char *s)
{
    strassign(proj->name, s);
    proj->fixed_properties->set("ant.project.name", proj->name);
}

void
project_set_description(project_t *proj, const char *s)
{
    strassign(proj->description, s);
}

void
project_set_default_target(project_t *proj, const char *s)
{
    strassign(proj->default_target, s);
}

/*
 * Setup the magical variables _PATHUP and _PATHDOWN
 */
static void
project_update_magic_paths(project_t *proj)
{
    const char *d;
    estring path;
    tok_t tok(file_normalise(proj->basedir, 0), "/");

    /* TODO: We need something like a file_denormalise() */
    
    while ((d = tok.next()) != 0)
	path.append_string((!strcmp(d, ".") ? "./" : "../"));
    
    project_set_property(proj, "_pathup", path.data());
    project_set_property(proj, "topdir", path.data());
    
    path.truncate();
    path.append_string(proj->basedir);
    if (path.length())
    	path.append_char('/');
    project_set_property(proj, "_pathdown", path.data());
}


void
project_set_basedir(project_t *proj, const char *s)
{
    if (proj->parent != 0)
    {
	strdelete(proj->basedir);
	proj->basedir = file_normalise(s, proj->parent->basedir);
    }
    else
    {
    	strassign(proj->basedir, s);
    }
    proj->fixed_properties->set("basedir", proj->basedir);
    project_update_magic_paths(proj);
}

void
project_set_filename(project_t *proj, const char *s)
{
    strassign(proj->filename, s);

    proj->fixed_properties->setm("ant.file",
    	    	file_normalise(proj->filename, 0));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_override_properties(project_t *proj, props_t *props)
{
    proj->properties->copy_contents(props);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

target_t *
project_find_target(project_t *proj, const char *name)
{
    return proj->targets->lookup(name);
}

void
project_remove_target(project_t *proj, target_t *targ)
{
    proj->targets->remove(targ->name);
    targ->project = 0;
}

void
project_add_target(project_t *proj, target_t *targ)
{
    assert(targ != 0);
    assert(targ->name != 0);
    proj->targets->insert(targ->name, targ);
    targ->project = proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tl_def_t *
project_find_tl_def(const project_t *proj, const char *name)
{
    tl_def_t *tldef;
    
    for ( ; proj != 0 ; proj = proj->parent)
    {
    	if ((tldef = proj->tl_defs->lookup(name)) != 0)
	    return tldef;
    }
    
    return 0;
}

void
project_add_tl_def(project_t *proj, tl_def_t *tldef)
{
    proj->tl_defs->insert(tldef->name_space(), tldef);
}

void
project_remove_tl_def(project_t *proj, tl_def_t *tldef)
{
    proj->tl_defs->remove(tldef->name_space());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define build_taglist_key(name_space, id) \
    g_strconcat((name_space), "::", (id), 0)

taglist_t *
project_find_taglist(project_t *proj, const char *name_space, const char *id)
{
    char *key = build_taglist_key(name_space, id);
    taglist_t *tl;
    
    for ( ; proj != 0 ; proj = proj->parent)
    {
    	if ((tl = proj->taglists->lookup(key)) != 0)
	    break;
    }
    
    g_free(key);
    return tl;
}

void
project_add_taglist(project_t *proj, taglist_t *tl)
{
    char *key = build_taglist_key(tl->name_space(), tl->id());
    
    proj->taglists->insert(key, tl);
}

void
project_remove_taglist(project_t *proj, taglist_t *tl)
{
    char *key = build_taglist_key(tl->name_space(), tl->id());
    char *okey = 0;
    taglist_t *ovalue = 0;
    
    proj->taglists->lookup_extended(key, &okey, &ovalue);
    assert(okey != 0);
    assert(tl == ovalue);
    
    proj->taglists->remove(key);

    g_free(key);
    g_free(okey);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
project_get_property(project_t *proj, const char *name)
{
    return proj->fixed_properties->get(name);
}

void
project_set_property(project_t *proj, const char *name, const char *value)
{
    proj->properties->set(name, value);
}

void
project_append_property(project_t *proj, const char *name, const char *value)
{
    const char *oldval = proj->properties->get(name);
    if (oldval == 0)
	proj->properties->set(name, value);
    else
	proj->properties->setm(name, g_strconcat(oldval, value, 0));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_add_fileset(project_t *proj, fileset_t *fs)
{
    assert(fs->id() != 0);
    proj->filesets->insert(fs->id(), fs);
}

fileset_t *
project_find_fileset(project_t *proj, const char *id)
{
    return proj->filesets->lookup(id);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
project_execute_target_by_name(project_t *proj, const char *name)
{
    target_t *targ;
    
    if ((targ = project_find_target(proj, name)) == 0)
    {
    	logf("no such target \"%s\"\n", name);
    	return FALSE;
    }
    return target_execute(targ);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
