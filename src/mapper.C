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

CVSID("$Id: mapper.C,v 1.4 2002-04-12 13:07:24 gnb Exp $");

hashtable_t<char*, mapper_creator_t> *mapper_t::creators;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_t::mapper_t()
{
}

mapper_t::~mapper_t()
{
    strdelete(from_);
    strdelete(to_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_t *
mapper_t::create(const char *name, const char *from, const char *to)
{
    mapper_t *ma;
    mapper_creator_t *creator;
    
    if (creators == 0 ||
    	(creator = creators->lookup((char*)name)) == 0)
    {
    	log::errorf("Unknown mapper type \"%s\"\n", name);
    	return 0;
    }

    ma =  (*creator)();
    
    strassign(ma->from_, from);
    strassign(ma->to_, to);
    
    if (!ma->init())
    {
    	delete ma;
	return 0;
    }
    
    return ma;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if 0
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
#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
mapper_t::add_creator(const char *name, mapper_creator_t *creator)
{
    if (creators == 0)
    	creators = new hashtable_t<char*, mapper_creator_t>;
    else if (creators->lookup((char *)name) != 0)
    {
    	log::errorf("mapper operations \"%s\" already registered, ignoring new definition\n",
	    	name);
    	return FALSE;
    }
    
    creators->insert(g_strdup(name), creator);
#if DEBUG
    fprintf(stderr, "mapper_t::add_creator: adding \"%s\" -> %08lx\n",
    	name, (unsigned long)creator);
#endif
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define MAPPER_CLASS(nm)  	extern mapper_creator_t mapper_##nm##_create;
#include "builtin-mappers.H"
#undef MAPPER_CLASS

void
mapper_t::initialise_builtins()
{
#define MAPPER_CLASS(nm)  mapper_t::add_creator(g_string(nm), &mapper_##nm##_create);
#include "builtin-mappers.H"
#undef MAPPER_CLASS
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
