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

#include "common.h"
#include "estring.h"
#include "props.h"
#include "strarray.h"
#include "pattern.h"
#include "filename.h"
#include "fileset.h"
#include "condition.h"
#include "mapper.h"
#include "xml.h"
#include "log.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Data Structures
 */
 
typedef struct project_s    	project_t;
typedef struct target_s    	target_t;
typedef struct task_s    	task_t;
typedef struct task_attr_s    	task_attr_t;
typedef struct task_child_s    	task_child_t;
typedef struct task_ops_s    	task_ops_t;
typedef struct task_scope_s 	task_scope_t;


struct project_s
{
    char *name;
    char *description;
    char *filename;
    char *default_target;
    char *basedir;
    project_t *parent;	    	/* inherits tscope, props etc */
    GHashTable *targets;
    GHashTable *filesets;   	/* <fileset>s in project scope */
    task_scope_t *tscope;   	/* scope for taskdefs */
    props_t *fixed_properties;	/* e.g. "basedir" which can't be overridden */
    props_t *properties;    	/* mutable properties from <property> element */
};

#define T_DEFINED   	    (1<<0)  	/* defined with <target> element */
#define T_DEPENDED_ON	    (1<<1)  	/* referenced in at least one `depends' attribute */

struct target_s
{
    char *name;
    char *description;
    project_t *project;
    unsigned flags;
    GList *depends;
    condition_t condition;
    GList *tasks;
};

struct task_s
{
    char *id;
    char *name;
    char *description;
    project_t *project;
    target_t *target;
    fileset_t *fileset;     /* for directory-based tasks */
    task_ops_t *ops;
    void *private;
};

#define TT_REQUIRED 	(1<<0)

struct task_attr_s
{
    char *name;
    gboolean (*setter)(task_t *task, const char *name, const char *value);
    unsigned flags;
};
#define TASK_ATTR(scope, name, flags) \
    { g_string(name), scope ## _set_ ## name, flags}

struct task_child_s
{
    char *name;
    gboolean (*adder)(task_t *task, xmlNode *);
    unsigned flags;
};
#define TASK_CHILD(scope, name, flags) \
    { g_string(name), scope ## _add_ ## name, flags}

struct task_ops_s
{
    char *name;
    
    void (*init)(void); 	    	    /* static initialiser */
    void (*new)(task_t *);  	    	    /* instance created */
    gboolean (*post_parse)(task_t *);	    /* post-parsing check */
    gboolean (*execute)(task_t *);  	    /* execute instance */
    void (*delete)(task_t *);	    	    /* instance deleted */
    task_attr_t *attrs;     	    	    /* attributes and setter methods */
    task_child_t *children;     	    /* child elements and adder methods */
    gboolean is_fileset;    	    	    /* parse as a fileset */
    char *fileset_dir_name; 	    	    /* name of base dir property */
    
    /* Don't initialised these fields to anything except 0 */
    GHashTable *attrs_hashed;	    /* task_attr_t's hashed on name */
    GHashTable *children_hashed;    /* task_child_t's hashed on name */
};

struct task_scope_s
{
    task_scope_t *parent;
    GHashTable *taskdefs;
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Extern Functions
 */

/* cant.c */
/* TODO: gcc format decls */
void fatal(const char *fmt, ...);

/* buildfile.c */
void parse_error(const char *fmt, ...);
extern gboolean parse_condition(condition_t *cond, xmlNode *node);
extern fileset_t *parse_fileset(project_t *, xmlNode *, const char *dirprop);	/* for e.g. <delete> */
mapper_t *parse_mapper(project_t *proj, xmlNode *node);
extern task_t *parse_task(project_t *, xmlNode *);	/* for recursives e.g. <condition> */
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
const char *project_get_property(project_t *, const char *name);
void project_set_property(project_t *, const char *name, const char *value);
void project_add_fileset(project_t *, fileset_t *);
fileset_t *project_find_fileset(project_t *, const char *id);
#define project_get_props(proj)     ((proj)->fixed_properties)
#define project_expand(proj, str) \
    props_expand(project_get_props((proj)), (str))

/* target.c */
target_t *target_new(void);
void target_delete(target_t *);
void target_set_name(target_t *, const char *name);
void target_set_description(target_t *, const char *description);
void target_add_task(target_t *, task_t *task);
gboolean target_execute(target_t *targ);

/* task.c */
task_t *task_new(void);
void task_delete(task_t *);
void task_set_id(task_t *, const char *id);
void task_set_name(task_t *, const char *s);
void task_set_description(task_t *, const char *s);
gboolean task_set_attribute(task_t *, const char *name, const char *value);
void task_ops_attributes_apply(task_ops_t *ops,
    void (*function)(const task_attr_t *ta, void *userdata), void *userdata);
/* no task should ever need to call this */
void task_ops_add_attribute(task_ops_t *, const task_attr_t *ta);
gboolean task_execute(task_t *);
void task_initialise_builtins(void);

/* a task_scope_t is a stackable scope for task_ops_t definitions */
extern task_scope_t *tscope_builtins;
task_scope_t *tscope_new(task_scope_t *parent);
void tscope_delete(task_scope_t *);
gboolean tscope_register(task_scope_t *, task_ops_t *ops);
void tscope_unregister(task_scope_t *, task_ops_t *ops);
task_ops_t *tscope_find(task_scope_t *, const char *name);


#define task_expand(task, str) \
    project_expand((task)->project, (str))

/* process.c */
gboolean process_run(strarray_t *command, strarray_t *env);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_h_ */
