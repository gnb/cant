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

#include "cant.h"

CVSID("$Id: project.c,v 1.10 2001-11-21 16:31:34 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

project_t *
project_new(project_t *parent)
{
    project_t *proj;
    
    proj = new(project_t);
    
    proj->parent = parent;
    
    proj->targets = g_hash_table_new(g_str_hash, g_str_equal);	
    proj->filesets = g_hash_table_new(g_str_hash, g_str_equal);	
    proj->taglists = g_hash_table_new(g_str_hash, g_str_equal);	
    proj->tl_defs = g_hash_table_new(g_str_hash, g_str_equal);	
    proj->tscope = tscope_new((parent == 0 ? tscope_builtins : parent->tscope));
    
    proj->properties = props_new((parent == 0 ? 0 : parent->properties));
    proj->fixed_properties = props_new(proj->properties);

    props_set(proj->fixed_properties, "ant.version", VERSION);
    	
    return proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
project_delete_one_target(gpointer key, gpointer value, gpointer userdata)
{
    target_delete((target_t *)value);
    return TRUE;    /* so remove it already */
}

static gboolean
project_delete_one_fileset(gpointer key, gpointer value, gpointer userdata)
{
    fileset_delete((fileset_t *)value);
    return TRUE;    /* so remove it already */
}

static gboolean
project_delete_one_taglist(gpointer key, gpointer value, gpointer userdata)
{
    taglist_unref((taglist_t *)value);
    return TRUE;    /* so remove it already */
}

static gboolean
project_delete_one_tl_def(gpointer key, gpointer value, gpointer userdata)
{
    tl_def_delete((tl_def_t *)value);
    return TRUE;    /* so remove it already */
}

void
project_delete(project_t *proj)
{
    strdelete(proj->name);
    strdelete(proj->description);
    
    g_hash_table_foreach_remove(proj->targets, project_delete_one_target, 0);
    g_hash_table_destroy(proj->targets);
    tscope_delete(proj->tscope);
    
    g_hash_table_foreach_remove(proj->filesets, project_delete_one_fileset, 0);
    g_hash_table_destroy(proj->filesets);
    
    g_hash_table_foreach_remove(proj->tl_defs, project_delete_one_tl_def, 0);
    g_hash_table_destroy(proj->tl_defs);
    
    g_hash_table_foreach_remove(proj->taglists, project_delete_one_taglist, 0);
    g_hash_table_destroy(proj->taglists);
    
    props_delete(proj->properties);
    props_delete(proj->fixed_properties);
    
    g_free(proj);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_set_name(project_t *proj, const char *s)
{
    strassign(proj->name, s);
    props_set(proj->fixed_properties, "ant.project.name", proj->name);
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

void
project_set_basedir(project_t *proj, const char *s)
{
    strassign(proj->basedir, s);
    props_set(proj->fixed_properties, "basedir", proj->basedir);
}

void
project_set_filename(project_t *proj, const char *s)
{
    strassign(proj->filename, s);

    props_setm(proj->fixed_properties, "ant.file",
    	    	file_make_absolute(proj->filename));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_override_properties(project_t *proj, props_t *props)
{
    props_copy_contents(proj->properties, props);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

target_t *
project_find_target(project_t *proj, const char *name)
{
    return (target_t *)g_hash_table_lookup(proj->targets, name);
}

void
project_remove_target(project_t *proj, target_t *targ)
{
    g_hash_table_remove(proj->targets, targ->name);
    targ->project = 0;
}

void
project_add_target(project_t *proj, target_t *targ)
{
    assert(targ != 0);
    assert(targ->name != 0);
    g_hash_table_insert(proj->targets, targ->name, targ);
    targ->project = proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tl_def_t *
project_find_tl_def(const project_t *proj, const char *name)
{
    tl_def_t *tldef;
    
    for ( ; proj != 0 ; proj = proj->parent)
    {
    	if ((tldef = g_hash_table_lookup(proj->tl_defs, name)) != 0)
	    return tldef;
    }
    
    return 0;
}

void
project_add_tl_def(project_t *proj, tl_def_t *tldef)
{
    g_hash_table_insert(proj->tl_defs, tldef->name, tldef);
}

void
project_remove_tl_def(project_t *proj, tl_def_t *tldef)
{
    g_hash_table_remove(proj->tl_defs, tldef->name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define build_taglist_key(namespace, id) \
    g_strconcat((namespace), "::", (id), 0)

taglist_t *
project_find_taglist(project_t *proj, const char *namespace, const char *id)
{
    char *key = build_taglist_key(namespace, id);
    taglist_t *tl;
    
    for ( ; proj != 0 ; proj = proj->parent)
    {
    	if ((tl = g_hash_table_lookup(proj->taglists, key)) != 0)
	    break;
    }
    
    g_free(key);
    return tl;
}

void
project_add_taglist(project_t *proj, taglist_t *tl)
{
    char *key = build_taglist_key(tl->namespace, tl->id);
    
    g_hash_table_insert(proj->taglists, key, tl);
}

void
project_remove_taglist(project_t *proj, taglist_t *tl)
{
    char *key = build_taglist_key(tl->namespace, tl->id);
    gpointer okey = 0;
    gpointer ovalue;
    
    g_hash_table_lookup_extended(proj->taglists, key, 
    	    &okey, &ovalue);
    assert(okey != 0);
    
    g_hash_table_remove(proj->taglists, key);

    g_free(key);
    g_free(okey);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
project_get_property(project_t *proj, const char *name)
{
    return props_get(proj->fixed_properties, name);
}

void
project_set_property(project_t *proj, const char *name, const char *value)
{
    props_set(proj->properties, name, value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
project_add_fileset(project_t *proj, fileset_t *fs)
{
    assert(fs->id != 0);
    g_hash_table_insert(proj->filesets, fs->id, fs);
}

fileset_t *
project_find_fileset(project_t *proj, const char *id)
{
    return (fileset_t *)g_hash_table_lookup(proj->filesets, id);
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
