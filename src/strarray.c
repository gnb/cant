/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * Copyright (c) 2001 Greg Banks
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

#include "strarray.h"

CVSID("$Id: strarray.c,v 1.1 2001-11-07 08:36:00 gnb Exp $");

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
/*END*/
