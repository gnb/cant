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
#include "list.H"
#include "mapper.H"
#include "string_var.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
class fileset_t
{
public:

    enum _spec_flags
    {
    	/* Values for spec_t `flags' */
	FS_INCLUDE  	=(1<<0),
	FS_EXCLUDE  	=0,
	FS_FILE  	=(1<<1)
    };
    
    struct spec_t
    {
	~spec_t();

	unsigned flags_;
	string_var filename_;
	pattern_t pattern_;
	condition_t condition_;
    };

private:
    string_var id_;
    unsigned int refcount_;

    string_var directory_;
    
    list_t<spec_t> specs_;

    unsigned int default_excludes_:1;
    unsigned int case_sensitive_:1;

    spec_t *spec_add(unsigned flags, const char *pattern, const char *filename);

public:
    fileset_t();
    ~fileset_t();

    // reference counting. Object begins with a refcount of 1
    // and falling to 0 deletes it.
    void ref();
    void unref();

    const char *id() const { return id_; }
    void set_id(const char *id);
    
    const char *directory() const { return directory_; }
    void set_directory(const char *dir);

    gboolean default_excludes() const { return default_excludes_; }
    void set_default_excludes(gboolean b);

    gboolean case_sensitive() const { return case_sensitive_; }
    void set_case_sensitive(gboolean b);

    // add various children to the fileset
    spec_t *add_include(const char *s);
    spec_t *add_include_file(const char *s);
    spec_t *add_exclude(const char *s);
    spec_t *add_exclude_file(const char *s);

    // iterate the function `func' over all filenames which the fileset generates.
    void apply(const props_t *props,
    	       file_apply_proc_t func, void *userdata) const;

    // gather all filenames which the fileset generates, appending them to
    // the string array `sa', optionally filtering through the list of
    // mappers `mappers'.
    void gather_mapped(const props_t *props, strarray_t *sa,
		       const list_t<mapper_t> *mappers/*may be 0*/) const;

#if DEBUG
    void dump() const;
#endif
};

// for use in list_t.apply_remove()
void unref(fileset_t*);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _fileset_h_ */
