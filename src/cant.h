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

typedef struct taglist_s    	taglist_t;
typedef struct tl_item_s    	tl_item_t;
typedef struct tagexp_s     	tagexp_t;
typedef struct tl_def_s     	tl_def_t;
typedef struct tl_def_tag_s 	tl_def_tag_t;


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
    GHashTable *taglists;   	/* taglists in project scope */
    
    GHashTable *tl_defs;   	/* all taglistdefs */
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
    /* TODO: I *really* need to convert this to C++ */
    GList *subtasks;
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
#define TASK_GENERIC_CHILD(scope, name, flags) \
    { "*", scope ## _add_ ## name, flags}

struct task_ops_s
{
    char *name;
    
    void (*init)(void); 	    	    /* static initialiser */
    void (*new)(task_t *);  	    	    /* instance created */
    gboolean (*set_content)(task_t *, const char *);	/* node content */
    gboolean (*post_parse)(task_t *);	    /* post-parsing check */
    gboolean (*execute)(task_t *);  	    /* execute instance */
    void (*delete)(task_t *);	    	    /* instance deleted */
    task_attr_t *attrs;     	    	    /* attributes and setter methods */
    task_child_t *children;     	    /* child elements and adder methods */
    gboolean is_fileset;    	    	    /* parse as a fileset */
    char *fileset_dir_name; 	    	    /* name of base dir property */
    void (*cleanup)(task_ops_t*); 	    /* static dtor */
    gboolean is_composite;  	    	    /* accepts subtasks */
    
    /* Don't initialised these fields to anything except 0 */
    GHashTable *attrs_hashed;	    /* task_attr_t's hashed on name */
    GHashTable *children_hashed;    /* task_child_t's hashed on name */
};

struct task_scope_s
{
    task_scope_t *parent;
    GHashTable *taskdefs;
};

/*
 * Taglist is a sequence of tagged name=value pairs.
 * This data structure is used to store all sorts of
 * stuff, but especially the definitions of C libraries.
 */
struct taglist_s
{
    char *namespace;	    /* e.g. "library" -- fixed at creation */
    char *id;     	    /* must be fixed before attachment to project */
    int refcount;
    GList *items;   	    /* list of tl_item_t */
};

typedef enum
{
    TL_VALUE,
    TL_LINE,
    TL_FILE
} tl_item_type_t;

struct tl_item_s
{
    char *tag;
    char *name;
    tl_item_type_t type;
    char *value;
    condition_t condition;
};

/*
 * Tagexp is a definition of how to expand a taglist
 * to a sequence of strings in a atringarray.
 */
struct tagexp_s
{
    char *namespace;
    /* TODO: gboolean follow_depends:1; */
    /* TODO: gboolean reverse_order:1; */
    strarray_t *default_exps;
    GHashTable *exps;  	/* hash table of strarray_t, keyed on string */
};


/*
 * Tl_def is used to define the allowable structure
 * of taglists, so various kinds of taglists can be
 * parsed generically.
 */
struct tl_def_s
{
    char *name;
    GHashTable *tags;
    /* TODO: GList *depends */
};

typedef enum
{
    AV_MANDATORY,
    AV_OPTIONAL,
    AV_FORBIDDEN
} availability_t;
 
struct tl_def_tag_s
{
    char *name;
    availability_t name_avail;
    availability_t value_avail;
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
void parse_node_error(const xmlNode *, const char *fmt, ...);
void parse_error(const char *fmt, ...);
void parse_error_unknown_attribute(const xmlAttr *attr);
void parse_error_required_attribute(const xmlNode *node, const char *attrname);
void parse_error_unexpected_element(const xmlNode *node);
extern gboolean parse_condition(condition_t *cond, xmlNode *node);
extern taglist_t *parse_taglist(project_t *proj, xmlNode *node);
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
tl_def_t *project_find_tl_def(const project_t *proj, const char *name);
void project_add_tl_def(project_t *proj, tl_def_t *tld);
void project_remove_tl_def(project_t *proj, tl_def_t *tld);
taglist_t *project_find_taglist(project_t *proj, const char *namespace, const char *id);
void project_add_taglist(project_t*, taglist_t*);
void project_remove_taglist(project_t *proj, taglist_t *);
const char *project_get_property(project_t *, const char *name);
void project_set_property(project_t *, const char *name, const char *value);
void project_append_property(project_t *, const char *name, const char *value);
void project_add_fileset(project_t *, fileset_t *);
fileset_t *project_find_fileset(project_t *, const char *id);
#define project_get_props(proj)     ((proj)->fixed_properties)
#define project_expand(proj, str) \
    props_expand(project_get_props((proj)), (str))
gboolean project_execute_target_by_name(project_t *, const char *);

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
void task_add_subtask(task_t *task, task_t *subtask);
void task_attach(task_t *, target_t *);
gboolean task_set_attribute(task_t *, const char *name, const char *value);
void task_ops_attributes_apply(task_ops_t *ops,
    void (*function)(const task_attr_t *ta, void *userdata), void *userdata);
/* no task should ever need to call this */
void task_ops_add_attribute(task_ops_t *, const task_attr_t *ta);
const task_child_t *task_ops_find_child(const task_ops_t *ops, const char *name);
void task_ops_add_child(task_ops_t *, const task_child_t *tc);
gboolean task_execute(task_t *);
gboolean task_execute_subtasks(task_t *task);
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


tl_def_t *tl_def_new(const char *namespace);
void tl_def_delete(tl_def_t *);
tl_def_tag_t *tl_def_add_tag(tl_def_t*, const char *name,
    	    	    availability_t name_avail, availability_t value_avail);
const tl_def_tag_t *tl_def_find_tag(const tl_def_t *, const char *name);
tagexp_t *tagexp_new(const char *namespace);
void tagexp_delete(tagexp_t *te);
void tagexp_add_default_expansion(tagexp_t *te, const char *s);
void tagexp_add_expansion(tagexp_t *te, const char *tag, const char *exp);
taglist_t *taglist_new(const char *namespace);
void taglist_ref(taglist_t *tl);
void taglist_unref(taglist_t *tl);
void taglist_set_id(taglist_t *tl, const char *s);
tl_item_t *taglist_add_item(taglist_t *tl, const char *tag,
		const char *name, tl_item_type_t type, const char *value);
void taglist_gather(taglist_t *tl, tagexp_t *te,
		    props_t *props, strarray_t *sa);
void taglist_list_gather(GList *list_of_taglists, tagexp_t *te,
		    props_t *props, strarray_t *sa);


/* process.c */
gboolean process_run(strarray_t *command, strarray_t *env, const char *dir);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_h_ */
