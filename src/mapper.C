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

#include "mapper.H"
#include "log.H"
#include "hashtable.H"

CVSID("$Id: mapper.C,v 1.2 2002-03-29 16:12:31 gnb Exp $");

static hashtable_t<const char*, mapper_ops_t> *mapper_ops_all;

static mapper_ops_t *mapper_ops_find(const char *name);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: fmeh */
extern void parse_error(const char *fmt, ...);

mapper_t *
mapper_new(const char *name, const char *from, const char *to)
{
    mapper_t *ma;
    mapper_ops_t *ops;
    
    if ((ops = mapper_ops_find(name)) == 0)
    {
    	parse_error("Unknown mapper type \"%s\"\n", name);
    	return 0;
    }
	
    ma = new(mapper_t);
    
    ma->ops = ops;
    strassign(ma->from, from);
    strassign(ma->to, to);
    
    if (!(*ma->ops->ctor)(ma))
    {
    	mapper_delete(ma);
    	return 0;
    }
    
    return ma;
}

void
mapper_delete(mapper_t *ma)
{
    (*ma->ops->dtor)(ma);
    
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
    	mapper_ops_all = new hashtable_t<const char*, mapper_ops_t>;
    else if (mapper_ops_all->lookup(ops->name) != 0)
    {
    	logf("mapper operations \"%s\" already registered, ignoring new definition\n",
	    	ops->name);
    	return;
    }
    
    mapper_ops_all->insert(ops->name, ops);
#if DEBUG
    fprintf(stderr, "mapper_ops_register: registering \"%s\"\n", ops->name);
#endif
}

void
mapper_ops_unregister(mapper_ops_t *ops)
{
    if (mapper_ops_all != 0)
	mapper_ops_all->remove(ops->name);
}

static mapper_ops_t *
mapper_ops_find(const char *name)
{
    return mapper_ops_all->lookup(name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define MAPPEROPS(m)  	extern mapper_ops_t m;
#include "builtin-mappers.H"
#undef MAPPEROPS

void
mapper_initialise_builtins(void)
{
#define MAPPEROPS(m)  mapper_ops_register(&m);
#include "builtin-mappers.H"
#undef MAPPEROPS
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
