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
#include "string_var.H"
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
#include "target.H"
#include "project.H"

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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Extern Functions
 */

/* buildfile.c */
extern gboolean parse_condition(condition_t *cond, xml_node_t *node);
extern taglist_t *parse_taglist(project_t *proj, xml_node_t *node);
extern fileset_t *parse_fileset(project_t *, xml_node_t *, const char *dirprop);	/* for e.g. <delete> */
mapper_t *parse_mapper(project_t *proj, xml_node_t *node);
extern task_t *parse_task(project_t *, xml_node_t *);	/* for recursives e.g. <condition> */
extern project_t *read_project(const char *filename, project_t *parent, gboolean isglobal);
extern project_t *read_buildfile(const char *filename, project_t *parent);

/* process.c */
gboolean process_run(strarray_t *command, strarray_t *env, const char *dir);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_h_ */
