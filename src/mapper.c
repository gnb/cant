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

#include "mapper.h"
#include "log.h"

CVSID("$Id: mapper.c,v 1.3 2001-11-14 06:30:26 gnb Exp $");

static GHashTable *mapper_ops_all;

static mapper_ops_t *mapper_ops_find(const char *name);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_t *
mapper_new(const char *name, const char *from, const char *to)
{
    mapper_t *ma;
    mapper_ops_t *ops;
    
    if ((ops = mapper_ops_find(name)) == 0)
    	return 0;
	
    ma = new(mapper_t);
    
    ma->ops = ops;
    strassign(ma->from, from);
    strassign(ma->to, to);
    
    (*ma->ops->new)(ma);
    
    return ma;
}

void
mapper_delete(mapper_t *ma)
{
    (*ma->ops->delete)(ma);
    
    strdelete(ma->from);
    strdelete(ma->to);
    
    g_free(ma);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
mapper_map(mapper_t *ma, const char *filename)
{
    char *res;
    
    res = (*ma->ops->map)(ma, filename);
    
#if DEBUG
    fprintf(stderr, "mapper_map: %s: \"%s\" -> \"%s\"\n",
    	    ma->ops->name, filename, res);
#endif

    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mapper_ops_register(mapper_ops_t *ops)
{
    if (mapper_ops_all == 0)
    	mapper_ops_all = g_hash_table_new(g_str_hash, g_str_equal);
    else if (g_hash_table_lookup(mapper_ops_all, ops->name) != 0)
    {
    	logf("mapper operations \"%s\" already registered, ignoring new definition\n",
	    	ops->name);
    	return;
    }
    
    g_hash_table_insert(mapper_ops_all, ops->name, ops);
#if DEBUG
    fprintf(stderr, "mapper_ops_register: registering \"%s\"\n", ops->name);
#endif
}

void
mapper_ops_unregister(mapper_ops_t *ops)
{
    if (mapper_ops_all != 0)
	g_hash_table_remove(mapper_ops_all, ops->name);
}

static mapper_ops_t *
mapper_ops_find(const char *name)
{
    return (mapper_ops_t *)g_hash_table_lookup(mapper_ops_all, name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define MAPPEROPS(m)  	extern mapper_ops_t m;
#include "builtin-mappers.h"
#undef MAPPEROPS

void
mapper_initialise_builtins(void)
{
#define MAPPEROPS(m)  mapper_ops_register(&m);
#include "builtin-mappers.h"
#undef MAPPEROPS
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
