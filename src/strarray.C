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

CVSID("$Id: strarray.C,v 1.1 2002-03-29 12:36:26 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

strarray_t *
strarray_new(void)
{
    return g_ptr_array_new();
}

void
strarray_delete(strarray_t *sa)
{
    int i;
    
    for (i = 0 ; i < sa->len ; i++)
    	if (g_ptr_array_index(sa, i) != 0)
    	    g_free(g_ptr_array_index(sa, i));
    g_ptr_array_free(sa, /*free_seg*/TRUE);
}

int
strarray_appendm(strarray_t *sa, char *s)
{
    int i = sa->len;
    g_ptr_array_add(sa, s);
    return i;
}

int
strarray_append(strarray_t *sa, const char *s)
{
    return strarray_appendm(sa, (s == 0 ? 0 : g_strdup(s)));
}

void
strarray_remove(strarray_t *sa, int i)
{
    if (g_ptr_array_index(sa, i) != 0)
    	g_free(g_ptr_array_index(sa, i));
    g_ptr_array_remove_index(sa, i);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


int
strarray_split_tom(strarray_t *sa, char *str, const char *sep)
{
    /* tokenise value on whitespace */
    const char *x;
    int oldlen = sa->len;
    tok_t tok;
    
    if (sep == 0)
    	sep = " \t\n\r";
    tok_init_m(&tok, str, sep);
    while ((x = tok_next(&tok)) != 0)
	strarray_append(sa, x);
    tok_free(&tok);
    return sa->len - oldlen;
}


int
strarray_split_to(strarray_t *sa, const char *str, const char *sep)
{
    return strarray_split_tom(sa, g_strdup(str), sep);
}

strarray_t *
strarray_split(const char *str, const char *sep)
{
    strarray_t *sa;
    
    sa = strarray_new();
    strarray_split_to(sa, str, sep);
    return sa;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
strarray_default_compare(const char **s1, const char **s2)
{
    return strcmp(*s1, *s2);
}

void
strarray_sort(strarray_t *sa, int (*compare)(const char **, const char **))
{
    if (compare == 0)
    	compare = strarray_default_compare;
    qsort(sa->pdata, sa->len, sizeof(char*),
    	    	(int (*)(const void*, const void*))compare);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
