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

#include "common.h"

typedef struct props_s	props_t;

/*
 * Create and destroy property collections.
 */
props_t *props_new(props_t *parent);
void props_delete(props_t *);

/*
 * Copy all the name/value pairs from `orig' to `props'.
 */
void props_copy_contents(props_t *props, const props_t *orig);

/*
 * Get a property value from this props_t or any of
 * its ancestors.  Set a property value in this props_t.
 */
const char *props_get(const props_t *, const char *name);
void props_set(props_t *, const char *name, const char *value);
/*
 * Same as props_set() except that `value' is not
 * g_strdup()ed but taken over (and g_free()d when
 * the props_t is deleted).
 */
void props_setm(props_t *, const char *name, char *value);

/*
 * Apply a function to all properties in this and
 * all ancestor props_t's, in indeterminate order.
 * Probably quite slow.
 */
void props_apply(const props_t *, void (*func)(const char *name, const char *value,
    	    	    void *userdata), void *userdata);

/*
 * Faster version that only applies the function to
 * properties defined in the given props_t and not
 * any of its ancestors.
 */
void props_apply_local(const props_t *, void (*func)(const char *name,
    	    	    const char *value, void *userdata), void *userdata);

/*
 * Replace all instances of "${property}" in `string' with
 * the value of the corresponding property, i.e. expand
 * all property references.
 */
char *props_expand(const props_t *, const char *string);



#endif /* _props_h_ */
