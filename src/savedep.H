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

#ifndef _savedep_h_
#define _savedep_h_ 1

#include "common.H"
#include "hashtable.H"
#include "strarray.H"
#include "string_var.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 

class savedep_t
{
public:
    enum quality_t
    {
    	NONE,	    	// no such dependency
    	IMPLICIT,   	// e.g. foo.o: foo.c
	LOADED,     	// extracted in previous run, loaded from state
    	EXTRACTED,  	// from gcc -MD
	EXPLICIT    	// user specified in buildfile
    };
    typedef void (*apply_func_t)(const char *from, const char *to,
    	    	    	        quality_t q, void *closure);

private:
    string_var filename_;
    hashtable_t<char*, hashtable_t<char*, quality_t> > *deps_;
    
    static savedep_t *instance_;

    gboolean load();
    gboolean save() const;
    
public:
    savedep_t(const char *filename);
    ~savedep_t();

    // make `from' depend on `to'
    void add(const char *from, const char *to, quality_t);
    // make `from' depend on each of `to'
    void add(const char *from, strarray_t *to, quality_t);
    
    quality_t get_quality(const char *from, const char *to);
    
    void apply(apply_func_t function, void *closure) const;
    void from_apply(const char *from, apply_func_t function, void *closure) const;
    
    static savedep_t *instance() { return instance_; }
    
#if DEBUG
    void dump() const;
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _savedep_h_ */
