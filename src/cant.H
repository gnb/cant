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
#include "task.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Data Structures
 */
 

typedef struct taglist_s    	taglist_t;
typedef struct tl_item_s    	tl_item_t;
typedef struct tagexp_s     	tagexp_t;
typedef struct tl_def_s     	tl_def_t;
typedef struct tl_def_tag_s 	tl_def_tag_t;


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
    GList *depends;
    condition_t condition;
    GList *tasks;
};

/*
 * Taglist is a sequence of tagged name=value pairs.
 * This data structure is used to store all sorts of
 * stuff, but especially the definitions of C libraries.
 */
struct taglist_s
{
    char *name_space;	    /* e.g. "library" -- fixed at creation */
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
    char *name_space;
    /* TODO: gboolean follow_depends:1; */
    /* TODO: gboolean reverse_order:1; */
    strarray_t *default_exps;
    hashtable_t<char *, strarray_t> *exps;
};


/*
 * Tl_def is used to define the allowable structure
 * of taglists, so various kinds of taglists can be
 * parsed generically.
 */
struct tl_def_s
{
    char *name;
    hashtable_t<const char *, tl_def_tag_t> *tags;
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
    props_expand(project_get_props((proj)), (str))
gboolean project_execute_target_by_name(project_t *, const char *);

/* target.c */
target_t *target_new(void);
void target_delete(target_t *);
void target_set_name(target_t *, const char *name);
void target_set_description(target_t *, const char *description);
void target_add_task(target_t *, task_t *task);
gboolean target_execute(target_t *targ);


tl_def_t *tl_def_new(const char *name_space);
void tl_def_delete(tl_def_t *);
tl_def_tag_t *tl_def_add_tag(tl_def_t*, const char *name,
    	    	    availability_t name_avail, availability_t value_avail);
const tl_def_tag_t *tl_def_find_tag(const tl_def_t *, const char *name);
tagexp_t *tagexp_new(const char *name_space);
void tagexp_delete(tagexp_t *te);
void tagexp_add_default_expansion(tagexp_t *te, const char *s);
void tagexp_add_expansion(tagexp_t *te, const char *tag, const char *exp);
taglist_t *taglist_new(const char *name_space);
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
