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

#include "fileset.H"
#include "estring.H"
#include "tok.H"
#include "mapper.H"
#include "log.H"
#include "globber.H"
#include <dirent.h>

CVSID("$Id: fileset.C,v 1.9 2002-04-07 06:22:51 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t::fileset_t()
{
    refcount_ = 1;
    directory_ = g_strdup(".");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t::~fileset_t()
{
    specs_.delete_all();
    strdelete(id_);
    strdelete(directory_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_t::ref()
{
    refcount_++;
}

void
fileset_t::unref()
{
    if (--refcount_ == 0)
    	delete this;
}

void
unref(fileset_t *fs)
{
    fs->unref();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_t::set_id(const char *s)
{
    strassign(id_, s);
}

void
fileset_t::set_directory(const char *dir)
{
    strassign(directory_, dir);
}

void
fileset_t::set_default_excludes(gboolean b)
{
    default_excludes_ = b;
}

void
fileset_t::set_case_sensitive(gboolean b)
{
    case_sensitive_ = b;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t::spec_t *
fileset_t::spec_add(
    unsigned flags,
    const char *pattern,
    const char *filename)
{
    spec_t *fss;
    
    fss = new spec_t;
    
    fss->flags_ = flags;
    
    if (pattern != 0)
    	fss->pattern_.set_pattern(pattern,
	    	    	(case_sensitive_ ? PAT_CASE : 0));
    
    strassign(fss->filename_, filename);

    specs_.append(fss);
        
    return fss;
}

fileset_t::spec_t::~spec_t()
{
    strdelete(filename_);
}

fileset_t::spec_t *
fileset_t::add_include(const char *pattern)
{
    return spec_add(FS_INCLUDE, /*pattern*/0, pattern);
}

fileset_t::spec_t *
fileset_t::add_include_file(const char *filename)
{
    return spec_add(FS_INCLUDE|FS_FILE, /*pattern*/0, filename);
}

fileset_t::spec_t *
fileset_t::add_exclude(const char *pattern)
{
    return spec_add(FS_EXCLUDE, pattern, /*filename*/0);
}

fileset_t::spec_t *
fileset_t::add_exclude_file(const char *filename)
{
    return spec_add(FS_EXCLUDE|FS_FILE, /*pattern*/0, filename);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileset_t::apply(
    const props_t *props,
    file_apply_proc_t func,
    void *userdata) const
{
    list_iterator_t<spec_t> iter;
    list_iterator_t<char> fniter;
    char *dir_e;
    
    dir_e = props->expand(directory_);
    globber_t globber(dir_e, case_sensitive_);
    g_free(dir_e);
    
    for (iter = specs_.first() ; iter != 0 ; ++iter )
    {
    	spec_t *fss = *iter;
	
	if (!fss->condition_.evaluate(props))
	    continue;

    	switch (fss->flags_ & (FS_FILE|FS_INCLUDE))
	{
	case FS_FILE|FS_INCLUDE:
	    globber.include_file(fss->filename_);
	    break;
	case FS_FILE|FS_EXCLUDE:
	    globber.exclude_file(fss->filename_);
	    break;
	case FS_INCLUDE:
	    globber.include(fss->filename_);
	    break;
	case FS_EXCLUDE:
	    globber.exclude(&fss->pattern_);
	    break;
	}
    }
    
    for (fniter = globber.first_filename() ; fniter != 0 ; ++fniter)
    {
	if (!(*func)(*fniter, userdata))
	    break;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    const list_t<mapper_t> *mappers;
    strarray_t *strarray;
} fs_gather_rec_t;

static gboolean
fileset_gather_one(const char *filename, void *userdata)
{
    fs_gather_rec_t *rec = (fs_gather_rec_t *)userdata;

    if (rec->mappers == 0)
    {
	rec->strarray->append(filename);
    }
    else
    {
    	list_iterator_t<mapper_t> iter;
	char *mapped = 0;

	for (iter = rec->mappers->first() ; iter != 0 ; ++iter)
	{
	    if ((mapped = (*iter)->map(filename)) != 0)
		break;
	}

	if (mapped != 0)
	    rec->strarray->appendm(mapped);
    }
    
    return TRUE;   /* keep going */
}

void
fileset_t::gather_mapped(
    const props_t *props,
    strarray_t *sa,
    const list_t<mapper_t> *mappers) const
{
    fs_gather_rec_t rec;
    
    rec.mappers = mappers;
    rec.strarray = sa;
    
    apply(props, fileset_gather_one, &rec);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
