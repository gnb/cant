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

#ifndef _strarray_h_
#define _strarray_h_ 1

#include "common.h"

typedef GPtrArray   strarray_t;

strarray_t *strarray_new(void);
void strarray_delete(strarray_t *);

int strarray_append(strarray_t *, const char *s);
int strarray_appendm(strarray_t *, char *s);
void strarray_remove(strarray_t *, int i);

#define strarray_join(sa, sep) \
    g_strjoinv((sep), (char **)(sa)->pdata)

#define strarray_nth(sa, i) \
    ((const char *)g_ptr_array_index((sa), (i)))
#define strarray_data(sa) \
    ((const char **)((sa)->pdata))


#endif /* _strarray_h_ */
