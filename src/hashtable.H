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

#ifndef _hashtable_h_
#define _hashtable_h_ 1

#include "common.H"

/*
 * Wrapper for glib's GHashTable structure.
 *
 * Note that for memory efficiency, we inherit from GHashTable rather
 * than contain a pointer to it.  The problem is that GHashTable is
 * an opaque private structure, so you must never, ever ever allocate
 * hashtable_t as auto variables or struct member, in fact any way
 * except dynamically with new().
 */

// fool the compiler into believing we have enough information
// about GHashTable to be able to derive from it.
struct _GHashTable { int dummy_; };

template<class K> class hashtable_ops_t
{
public:
    static GHashFunc hash;
    static GCompareFunc compare;
};

template<class K/*key*/, class T/*item*/> class hashtable_t : public GHashTable
{
public:
    /* ctor */
    hashtable_t()
    {
    }
    /* dtor */
    ~hashtable_t()
    {
    }
    
    void *operator new(size_t)
    {
    	return g_hash_table_new(
	    	    hashtable_ops_t<K>::hash,
		    hashtable_ops_t<K>::compare);
    }
    
    void operator delete(void *x)
    {
    	g_hash_table_destroy((GHashTable *)x);
    }

    void insert(K key, T *value)
    {
    	g_hash_table_insert(this, (gpointer)key, (gpointer)value);
    }
    
    void remove(K key)
    {
    	g_hash_table_remove(this, (gconstpointer)key);
    }
    
    T *lookup(K key)
    {
    	return (T *)g_hash_table_lookup(this, (gconstpointer)key);
    }
    
    gboolean lookup_extended(K key, K *orig_key_ret, T **value_ret)
    {
    	return g_hash_table_lookup_extended(this, (gconstpointer)key,
	    	    	(gpointer *)orig_key_ret, (gpointer *)value_ret);
    }
    
    void foreach(void (*func)(K, T*, void *closure), void *closure)
    {
    	g_hash_table_foreach(this, (GHFunc)*func, closure);
    }
    
    void foreach_remove(gboolean (*func)(K, T*, void *closure), void *closure)
    {
    	g_hash_table_foreach_remove(this, (GHRFunc)func, closure);
    }
    
    guint size() const
    {
    	return g_hash_table_size((GHashTable*)this);
    }
};



#endif /* _hashtable_h_ */
