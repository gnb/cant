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

#include "common.h"
#include "filename.h"
#include "props.h"
#include "pattern.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
typedef struct fileset_s    	fileset_t;
typedef struct fs_spec_s    	fs_spec_t;


struct fileset_s
{
    char *id;
    props_t *props; 	/* scope for if/unless conditionals */
    
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



void fs_spec_set_if_condition(fs_spec_t *fss, const char *prop);
void fs_spec_set_unless_condition(fs_spec_t *fss, const char *prop);

fileset_t *fileset_new(props_t *);
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _fileset_h_ */
