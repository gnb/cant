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

#ifndef _taglist_h_
#define _taglist_h_ 1

#include "common.H"
#include "strarray.H"
#include "hashtable.H"
#include "list.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

enum availability_t
{
    AV_MANDATORY,
    AV_OPTIONAL,
    AV_FORBIDDEN
};

class tagexp_t;
 
/*
 * Taglist is a sequence of tagged name=value pairs.
 * This data structure is used to store all sorts of
 * stuff, but especially the definitions of C libraries.
 */
class taglist_t
{
public:
    enum item_type_t
    {
	TL_VALUE,
	TL_LINE,
	TL_FILE
    };

    struct item_t
    {
    	item_t(const char *tag, const char *name,
	       item_type_t type, const char *value);
    	~item_t();
	
	char *tag_;
	char *name_;
	item_type_t type_;
	char *value_;
	condition_t condition_;
    };

private:
    char *name_space_;	    /* e.g. "library" -- fixed at creation */
    char *id_;     	    /* must be fixed before attachment to project */
    int refcount_;
    list_t<item_t> items_;

    item_t *add_item(const char *tag, const char *name,
    	    	     item_type_t type, const char *value);

public:
    taglist_t(const char *name_space);
    ~taglist_t();
    
    // TODO: class refcounted_t
    void ref();
    void unref();
    
    const char *name_space() const { return name_space_; }
    
    const char *id() const { return id_; }
    void set_id(const char *);
    
    item_t *add_value(const char *tag, const char *name, const char *value);
    item_t *add_file(const char *tag, const char *name, const char *value);
    item_t *add_line(const char *tag, const char *name, const char *value);

#if DEBUG
    // for diagnostics only
    list_iterator_t<item_t> first_item() const { return items_.first(); }
#endif

    void gather(const tagexp_t *te, const props_t *props, strarray_t *sa) const;
    static void list_gather(const list_t<taglist_t> *list, const tagexp_t *te,
		     const props_t *props, strarray_t *sa);


#if DEBUG
    void dump() const;
#endif
};

// for passing to list_t::apply_remove
void unref(taglist_t *);

/*
 * Tagexp is a definition of how to expand a taglist
 * to a sequence of strings in a atringarray.
 */
class tagexp_t
{
private:
    char *name_space_;
    /* TODO: gboolean follow_depends:1; */
    /* TODO: gboolean reverse_order:1; */
    strarray_t *default_expansions_;
    hashtable_t<char *, strarray_t> *expansions_;

public:
    tagexp_t(const char *name_space);
    ~tagexp_t();

    const char *name_space() const { return name_space_; }
    
    void add_default_expansion(const char *s);
    void add_expansion(const char *tag, const char *exp);
    const strarray_t *find_expansions(const char *tag) const;
};


/*
 * Tl_def is used to define the allowable structure
 * of taglists, so various kinds of taglists can be
 * parsed generically.
 */
class tl_def_t
{
public:
    struct tag_t
    {
    	tag_t(const char *name, availability_t name_avail, availability_t value_avail);
    	~tag_t();
		
	char *name_;
	availability_t name_avail_;
	availability_t value_avail_;
    };

private:
    char *name_space_;
    hashtable_t<const char *, tag_t> *tags_;
    /* TODO: list_t<something> *depends */
    
public:
    tl_def_t(const char *name_space);
    ~tl_def_t();
    
    const char *name_space() const { return name_space_; }

    tag_t *add_tag(const char *name,
    	    	    availability_t name_avail, availability_t value_avail);
    const tag_t *find_tag(const char *name) const;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _taglist_h_ */
