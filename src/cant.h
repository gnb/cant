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

#ifndef _cant_h_
#define _cant_h_ 1

#include "common.h"
#include "estring.h"
#include "props.h"
#include "strarray.h"
#include <regex.h>
#include "xml.h"

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
typedef struct xtask_arg_s    	xtask_arg_t;
typedef struct xtask_ops_s    	xtask_ops_t;
typedef struct pattern_s    	pattern_t;
typedef struct fileset_s    	fileset_t;
typedef struct fs_spec_s    	fs_spec_t;

struct project_s
{
    char *name;
    char *description;
    char *filename;
    char *default_target;
    char *basedir;
    GHashTable *targets;
    GHashTable *filesets;   	/* <fileset>s in project scope */
    props_t *fixed_properties;	/* e.g. "basedir" which can't be overridden */
    props_t *properties;    	/* mutable properties from <property> element */
};

#define T_DEFINED   	    (1<<0)  	/* defined with <target> element */
#define T_DEPENDED_ON	    (1<<1)  	/* referenced in at least one `depends' attribute */
#define T_IFCOND	    (1<<2)  	/* `if' attribute specified */
#define T_UNLESSCOND	    (1<<3)  	/* `unless' attribute specified */

struct target_s
{
    char *name;
    char *description;
    project_t *project;
    unsigned flags;
    GList *depends;
    char *condition;	    	/* property referenced in `if' or `unless' attr */
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

typedef void (*task_setter_proc_t)(task_t *task, const char *name, const char *value);

struct task_attr_s
{
    char *name;
    task_setter_proc_t setter;
    unsigned flags;
};
#define TASK_ATTR(scope, name, flags) \
    { g_string(name), scope ## _set_ ## name, flags}

struct task_child_s
{
    char *name;
    void (*adder)(task_t *task, xmlNode *);
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

#define XT_WHITESPACE	    (1<<0)  	/* escape whitespace in `arg' */
#define XT_IFCOND	    (1<<1)  	/* `if' condition specified */
#define XT_UNLESSCOND	    (1<<2)  	/* `unless' condition specified */

struct xtask_arg_s
{
    unsigned flags;
    
    char *arg;
    fileset_t *fileset;
    
    char *condition;
};

struct xtask_ops_s
{
    task_ops_t task_ops;
    
    char *executable;
    char *logmessage;
    GList *args;    	    	/* list of xtask_arg_t */
    props_t *property_map;	/* maps attributes to local property *name*s */
    
    gboolean foreach:1;
};

struct pattern_s
{
    regex_t regex;
#if DEBUG
    char *pattern;
#endif
};

struct fileset_s
{
    char *id;
    project_t *project;
    
    char *directory;
    
    GList *specs;   	    /* list of fs_spec_t */

    gboolean default_excludes:1;
    gboolean case_sensitive:1;
};


#define FS_INCLUDE  	(1<<0)
#define FS_FILE  	(1<<1)
#define FS_IFCOND  	(1<<2)
#define FS_UNLESSCOND  	(1<<3)
#define FS_FILEREAD  	(1<<4)

struct fs_spec_s
{
    unsigned flags;
    
    char *filename;
    GList *specs;   	/* list of specs from file */
    
    pattern_t pattern;
    
    char *condition;
};



typedef gboolean (*file_apply_proc_t)(const char *filename, void *userdata);

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
extern fileset_t *parse_fileset(project_t *, xmlNode *, const char *dirprop);	/* for e.g. <delete> */
extern task_t *parse_task(project_t *, xmlNode *);	/* for recursives e.g. <condition> */
extern project_t *read_buildfile(const char *filename);

/*project.c */
project_t *project_new(void);
void project_delete(project_t *);
void project_set_name(project_t *, const char *name);
void project_set_description(project_t *, const char *description);
void project_set_default_target(project_t *, const char *s);
void project_set_basedir(project_t *, const char *s);
void project_set_filename(project_t *, const char *s);
void project_override_properties(project_t *proj, props_t *props);
target_t *project_find_target(project_t *, const char *name);
void project_add_target(project_t*, target_t*);
const char *project_get_property(project_t *, const char *name);
void project_set_property(project_t *, const char *name, const char *value);
void project_add_fileset(project_t *, fileset_t *);
fileset_t *project_find_fileset(project_t *, const char *id);
#define project_expand(proj, str) \
    props_expand((proj)->fixed_properties, (str))

/* target.c */
target_t *target_new(void);
void target_delete(target_t *);
void target_set_name(target_t *, const char *name);
void target_set_description(target_t *, const char *description);
void target_set_if_condition(target_t *, const char *cond);
void target_set_unless_condition(target_t *, const char *cond);
void target_add_task(target_t *, task_t *task);

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
void task_ops_register(task_ops_t *);
void task_ops_unregister(task_ops_t *);
task_ops_t *task_ops_find(const char *name);
gboolean task_execute(task_t *);
void task_initialise_builtins(void);

#define task_expand(task, str) \
    project_expand((task)->project, (str))

/* xtask.c */
xtask_ops_t *xtask_ops_new(const char *name);
void xtask_ops_delete(xtask_ops_t *xops);
void xtask_arg_set_if_condition(xtask_arg_t *xa, const char *prop);
void xtask_arg_set_unless_condition(xtask_arg_t *xa, const char *prop);
xtask_arg_t *xtask_ops_add_line(xtask_ops_t *xops, const char *s);
xtask_arg_t *xtask_ops_add_value(xtask_ops_t *xops, const char *s);
xtask_arg_t *xtask_ops_add_fileset(xtask_ops_t *xops, fileset_t *fs);
void xtask_ops_add_attribute(xtask_ops_t *xops, const char *attr,
    const char *prop, gboolean required);


/* fileset.c */
void fs_spec_set_if_condition(fs_spec_t *fss, const char *prop);
void fs_spec_set_unless_condition(fs_spec_t *fss, const char *prop);

fileset_t *fileset_new(project_t *);
void fileset_delete(fileset_t *);
void fileset_set_id(fileset_t *, const char *id);
void fileset_set_directory(fileset_t *, const char *dir);
fs_spec_t *fileset_add_include(fileset_t *, const char *s);
fs_spec_t *fileset_add_include_file(fileset_t *, const char *s);
fs_spec_t *fileset_add_exclude(fileset_t *, const char *s);
fs_spec_t *fileset_add_exclude_file(fileset_t *, const char *s);
void fileset_set_default_excludes(fileset_t *, gboolean b);
void fileset_set_case_sensitive(fileset_t *, gboolean b);
int fileset_apply(fileset_t *, file_apply_proc_t, void *userdata);
GList *fileset_gather(fileset_t *);

/* pattern.c */
void pattern_init(pattern_t *, const char *s, gboolean case_sens);
void pattern_free(pattern_t *);
pattern_t *pattern_new(const char *s, gboolean case_sens);
void pattern_delete(pattern_t *);
gboolean pattern_match(const pattern_t *, const char *filename);

/* log.c */    
void logf(const char *fmt, ...);
void logv(const char *fmt, va_list a);
void logperror(const char *filename);
void log_push_context(const char *name);
void log_pop_context(void);

/* filename.c */
char *file_make_absolute(const char *filename);
int file_build_tree(const char *dirname, mode_t mode);	/* make sequence of directories */
mode_t file_mode_from_string(const char *str, mode_t base, mode_t deflt);
int file_apply_children(const char *filename, file_apply_proc_t, void *userdata);
int file_is_directory(const char *filename);

/* process.c */
int process_run(strarray_t *command, strarray_t *env);
void process_log_status(const char *command, int status);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_h_ */
