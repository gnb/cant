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

#ifndef _strarray_h_
#define _strarray_h_ 1

#include "common.H"

/*
 * Wrapper for glib's GPtrArray structure.
 *
 * Note that for memory efficiency, we inherit from GPtrArray rather
 * than contain a pointer to it.  The problem is that GPtrArray is
 * actually a public subset of a larger private structure, so you
 * must never, ever ever allocate strarray_t as auto variables or
 * struct member, in fact any way except dynamically with new().
 */

class strarray_t : public GPtrArray
{
public:
    /* ctor */
    strarray_t();
    /* dtor */
    ~strarray_t();
    
    void *operator new(size_t);
    void operator delete(void *);

    int append(const char *s);
    int appendm(char *s);
    void remove(int i);

    char *join(const char *sep)
    {
    	return g_strjoinv((sep), (char **)pdata);
    }

    const char *nth(unsigned i) const
    {
	return (const char *)g_ptr_array_index(this, i);
    }
    const char *operator[](unsigned i) const
    {
	return (const char *)g_ptr_array_index(this, i);
    }
    
    const char **data() const
    {
	return (const char **)(pdata);
    }

    /*
     * Splits `str' at seperator chars defined by `sep' into
     * a new strarray.  If `sep' is NULL, whitespace is used
     * as seperators.
     */
    static strarray_t *split(const char *str, const char *sep);
    /*
     * Like strarray_split but appends to strarray `sa' and
     * returns the number of strings appended.
     */
    int split_to(const char *str, const char *sep);
    /*
     * Like strarray_split_to but consumes its `str' argument.
     */
    int split_tom(char *str, const char *sep);


    void sort(int (*compare)(const char **, const char **)/*0=strcmp*/);
};

#endif /* _strarray_h_ */
