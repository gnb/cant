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

#include "strarray.H"
#include "tok.H"

CVSID("$Id: strarray.C,v 1.3 2002-03-29 13:57:32 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void *
strarray_t::operator new(size_t)
{
    return g_ptr_array_new();
}

void
strarray_t::operator delete(void *x)
{
    g_ptr_array_free((GPtrArray *)x, /*free_seg*/TRUE);
}

strarray_t::strarray_t()
{
}

strarray_t::~strarray_t()
{
    int i;
    
    for (i = 0 ; i < len ; i++)
    	if (g_ptr_array_index(this, i) != 0)
    	    g_free(g_ptr_array_index(this, i));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
strarray_t::appendm(char *s)
{
    int i = len;
    
    g_ptr_array_add(this, s);
    return i;
}

int
strarray_t::append(const char *s)
{
    return appendm((s == 0 ? 0 : g_strdup(s)));
}

void
strarray_t::remove(int i)
{
    if (g_ptr_array_index(this, i) != 0)
    	g_free(g_ptr_array_index(this, i));
    g_ptr_array_remove_index(this, i);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


int
strarray_t::split_tom(char *str, const char *sep)
{
    const char *x;
    int oldlen = len;
    /* tokenise value on whitespace */
    tok_t tok(str, sep);
    
    while ((x = tok.next()) != 0)
	append(x);
    return len - oldlen;
}


int
strarray_t::split_to(const char *str, const char *sep)
{
    return split_tom(g_strdup(str), sep);
}

strarray_t *
strarray_t::split(const char *str, const char *sep)
{
    strarray_t *sa;
    
    sa = new(strarray_t);
    sa->split_to(str, sep);
    return sa;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
strarray_default_compare(const char **s1, const char **s2)
{
    return strcmp(*s1, *s2);
}

void
strarray_t::sort(int (*compare)(const char **, const char **))
{
    if (compare == 0)
    	compare = strarray_default_compare;
    qsort(pdata, len, sizeof(char*),
    	    	(int (*)(const void*, const void*))compare);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
