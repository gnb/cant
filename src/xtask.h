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

#ifndef _cant_xtask_h_
#define _cant_xtask_h_ 1

#include "cant.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
typedef struct xtask_arg_s    	xtask_arg_t;
typedef struct xtask_ops_s    	xtask_ops_t;

typedef enum
{
    XT_VALUE,	    /* argument used directly, whitespace-safe */
    XT_LINE,	    /* argument is split on whitespace */
    XT_FILE,	    /* agument is filename, to be canonicalised & absoluted. */
    XT_FILESET,     /* insert list of files from fileset child. */
    /* TODO: XT_FILELIST */
    /* TODO: XT_ENV */
    XT_FILES	    /* insert list of files from task's own fileset */
} xtask_arg_type_t;

struct xtask_arg_s
{
    xtask_arg_type_t type;
    
    union
    {
	char *arg;	    	/* for XT_VALUE, XT_LINE */
	fileset_t *fileset;     /* for XT_FILESET */
    } data;
    
    condition_t condition;
};

struct xtask_ops_s
{
    task_ops_t task_ops;
    
    char *executable;
    char *logmessage;
    GList *args;    	    	/* list of xtask_arg_t */
    props_t *property_map;	/* maps attributes to local property *name*s */
    GList *mappers;     	/* list of mapper_t: args to files */
    GList *dep_mappers;     	/* list of mapper_t: depfiles to targfiles */
    char *dep_target;
    
    gboolean foreach:1;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* xtask.c */
xtask_ops_t *xtask_ops_new(const char *name);
void xtask_ops_delete(xtask_ops_t *xops);
xtask_arg_t *xtask_ops_add_line(xtask_ops_t *xops, const char *s);
xtask_arg_t *xtask_ops_add_value(xtask_ops_t *xops, const char *s);
xtask_arg_t *xtask_ops_add_file(xtask_ops_t *xops, const char *s);
xtask_arg_t *xtask_ops_add_fileset(xtask_ops_t *xops, fileset_t *fs);
xtask_arg_t *xtask_ops_add_files(xtask_ops_t *xops);
void xtask_ops_add_attribute(xtask_ops_t *xops, const char *attr,
    const char *prop, gboolean required);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_xtask_h_ */
