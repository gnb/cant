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

#ifndef _cant_h_
#define _cant_h_ 1

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
#include "task.H"
#include "taglist.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Data Structures
 */
 


struct project_t
{
    char *name;
    char *description;
    char *filename;
    char *default_target;
    char *basedir;
    project_t *parent;	    	/* inherits tscope, props etc */
    
    hashtable_t<const char*, target_t> *targets;
    hashtable_t<const char*, fileset_t> *filesets;   	/* <fileset>s in project scope */
    hashtable_t<char*, taglist_t> *taglists;   	/* taglists in project scope */
    
    hashtable_t<const char*, tl_def_t> *tl_defs;   	/* all taglistdefs */
    task_scope_t *tscope;   	/* scope for taskdefs */
    
    props_t *fixed_properties;	/* e.g. "basedir" which can't be overridden */
    props_t *properties;    	/* mutable properties from <property> element */
};

#define T_DEFINED   	    (1<<0)  	/* defined with <target> element */
#define T_DEPENDED_ON	    (1<<1)  	/* referenced in at least one `depends' attribute */

struct target_t
{
    char *name;
    char *description;
    project_t *project;
    unsigned flags;
    list_t<target_t> depends;
    condition_t condition;
    list_t<task_t> tasks;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Globals
 */
 
#ifdef _DEFINE_GLOBALS
#define EXTERN
#define EQUALS(x) = x
#else
#define EXTERN extern
#define EQUALS(x)
#endif

EXTERN char *argv0;
EXTERN gboolean verbose;
EXTERN project_t *project_globals;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Extern Functions
 */

/* cant.c */
/* TODO: gcc format decls */
void fatal(const char *fmt, ...);
#if DEBUG
void dump_project(project_t *proj);
#endif

/* buildfile.c */
extern gboolean parse_condition(condition_t *cond, xml_node_t *node);
extern taglist_t *parse_taglist(project_t *proj, xml_node_t *node);
extern fileset_t *parse_fileset(project_t *, xml_node_t *, const char *dirprop);	/* for e.g. <delete> */
mapper_t *parse_mapper(project_t *proj, xml_node_t *node);
extern task_t *parse_task(project_t *, xml_node_t *);	/* for recursives e.g. <condition> */
extern project_t *read_buildfile(const char *filename, project_t *parent);

/*project.c */
project_t *project_new(project_t *parent);
void project_delete(project_t *);
void project_set_name(project_t *, const char *name);
void project_set_description(project_t *, const char *description);
void project_set_default_target(project_t *, const char *s);
void project_set_basedir(project_t *, const char *s);
void project_set_filename(project_t *, const char *s);
void project_override_properties(project_t *proj, props_t *props);
target_t *project_find_target(project_t *, const char *name);
void project_add_target(project_t*, target_t*);
void project_remove_target(project_t *proj, target_t *targ);
tl_def_t *project_find_tl_def(const project_t *proj, const char *name);
void project_add_tl_def(project_t *proj, tl_def_t *tld);
void project_remove_tl_def(project_t *proj, tl_def_t *tld);
taglist_t *project_find_taglist(project_t *proj, const char *name_space, const char *id);
void project_add_taglist(project_t*, taglist_t*);
void project_remove_taglist(project_t *proj, taglist_t *);
const char *project_get_property(project_t *, const char *name);
void project_set_property(project_t *, const char *name, const char *value);
void project_append_property(project_t *, const char *name, const char *value);
void project_add_fileset(project_t *, fileset_t *);
fileset_t *project_find_fileset(project_t *, const char *id);
#define project_get_props(proj)     ((proj)->fixed_properties)
#define project_expand(proj, str) \
    project_get_props((proj))->expand((str))
gboolean project_execute_target_by_name(project_t *, const char *);

/* target.c */
target_t *target_new(void);
void target_delete(target_t *);
void target_set_name(target_t *, const char *name);
void target_set_description(target_t *, const char *description);
void target_add_task(target_t *, task_t *task);
gboolean target_execute(target_t *targ);
void target_add_depend(target_t *targ, target_t *dep);
#define target_is_defined(targ) \
    ((targ)->flags & T_DEFINED)
void target_set_is_defined(target_t *targ, gboolean b);
#define target_is_depended_on(targ) \
    ((targ)->flags & T_DEPENDED_ON)


/* process.c */
gboolean process_run(strarray_t *command, strarray_t *env, const char *dir);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_h_ */
