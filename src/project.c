/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * Copyright (c) 2001 Greg Banks
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

project_t *
project_new(void)
{
    project_t *proj;
    
    proj = g_new(project_t, 1);
    if (proj == 0)
    	fatal("No memory\n");

    memset(proj, 0, sizeof(*proj));
    
    proj->targets = g_hash_table_new(g_str_hash, g_str_equal);	
    proj->filesets = g_hash_table_new(g_str_hash, g_str_equal);	

    proj->properties = props_new(0);	/* TODO: inherit from system props */
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

void
project_delete(project_t *proj)
{
    strdelete(proj->name);
    strdelete(proj->description);
	
    g_hash_table_foreach_remove(proj->targets, project_delete_one_target, 0);
    g_hash_table_destroy(proj->targets);
    
    g_hash_table_foreach_remove(proj->filesets, project_delete_one_fileset, 0);
    g_hash_table_destroy(proj->filesets);
    
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

target_t *
project_find_target(project_t *proj, const char *name)
{
    return (target_t *)g_hash_table_lookup(proj->targets, name);
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
/*END*/
