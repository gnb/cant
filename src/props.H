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

#ifndef _props_h_
#define _props_h_ 1

#include "common.H"
#include "hashtable.H"
#include "estring.H"

class props_t
{
private:
    const props_t *parent_;	    /* inherits values from here */
    hashtable_t<char*, char> *values_;

    void expand_1(estring *rep, const char *str, int depth) const;

public:
    props_t(const props_t *parent);
    ~props_t();
    
    /*
     * Copy all the name/value pairs from `orig' to `this'.
     */
    void copy_contents(const props_t *orig);
    
    /*
     * Get a property value from this props_t or any of
     * its ancestors.  Set a property value in this props_t.
     */
    const char *get(const char *name) const;
    void set(const char *name, const char *value/*copies this value*/);
    /*
     * Same as set() except that `value' is not
     * g_strdup()ed but taken over (and g_free()d when
     * the props_t is deleted).
     */
    void setm(const char *name, char *value/*takes over this value*/);

    /*
     * Apply a function to all properties in this and
     * all ancestor props_t's, in indeterminate order.
     * Probably quite slow.
     */
    void apply(
	void (*func)(const char *name, const char *value, void *userdata),
	void *userdata) const;
    /*
     * Faster version that only applies the function to
     * properties defined in the given props_t and not
     * any of its ancestors.
     */
    void
    apply_local(
	void (*func)(const char *name, const char *value, void *userdata),
	void *userdata) const;

    /*
     * Replace all instances of "${property}" in `string' with
     * the value of the corresponding property, i.e. expand
     * all property references.  Returns a new string.
     */
    char *expand(const char *str) const;

    /*
     * Read a file containing property assignments in the form
     * of "NAME=VALUE" on each line, like a shell environment dump
     * or a Linux kernel .config file.  Returns TRUE on success.
     */
    gboolean read_shellfile(const char *filename);
};

#endif /* _props_h_ */
