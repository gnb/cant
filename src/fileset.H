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

#ifndef _fileset_h_
#define _fileset_h_ 1

#include "common.H"
#include "filename.H"
#include "props.H"
#include "pattern.H"
#include "condition.H"
#include "strarray.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
typedef struct fileset_s    	fileset_t;
typedef struct fs_spec_s    	fs_spec_t;


struct fileset_s
{
    char *id;
    unsigned refcount;
    
    char *directory;
    
    GList *specs;   	    /* list of fs_spec_t */

    unsigned int default_excludes:1;
    unsigned int case_sensitive:1;
};


#define FS_INCLUDE  	(1<<0)
#define FS_EXCLUDE  	0
#define FS_FILE  	(1<<1)

struct fs_spec_s
{
    unsigned flags;
    char *filename;
    pattern_t pattern;
    condition_t condition;
};



fileset_t *fileset_new(void);
void fileset_ref(fileset_t *);
void fileset_unref(fileset_t *);
void fileset_set_id(fileset_t *, const char *id);
void fileset_set_directory(fileset_t *, const char *dir);
fs_spec_t *fileset_add_include(fileset_t *, const char *s);
fs_spec_t *fileset_add_include_file(fileset_t *, const char *s);
fs_spec_t *fileset_add_exclude(fileset_t *, const char *s);
fs_spec_t *fileset_add_exclude_file(fileset_t *, const char *s);
void fileset_set_default_excludes(fileset_t *, gboolean b);
void fileset_set_case_sensitive(fileset_t *, gboolean b);
void fileset_apply(const fileset_t *, const props_t *props,
    	    	  file_apply_proc_t, void *userdata);
void fileset_gather_mapped(const fileset_t *, const props_t *props,
    	    	    	  strarray_t *sa, GList *mappers/*may be 0*/);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _fileset_h_ */
