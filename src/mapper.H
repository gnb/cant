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

#ifndef _mapper_h_
#define _mapper_h_ 1

#include "common.H"
#include "hashtable.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
class mapper_t;
typedef mapper_t * (mapper_creator_t)(void);

class mapper_t
{
private:
    static hashtable_t<char*, mapper_creator_t> *creators;

protected:    
    char *from_;
    char *to_;

    mapper_t();

    // separate from ctor to allow it to fail gracefully
    virtual gboolean init() = 0;

public:
    virtual ~mapper_t();

    virtual char *map(const char *filename) = 0;
    
    static mapper_t *create(const char *name, const char *from, const char *to);

    static gboolean add_creator(const char *name, mapper_creator_t *);
    // TODO: remove_creator

    static void initialise_builtins();
};

#define MAPPER_DEFINE_CLASS(nm) \
\
mapper_t *mapper_##nm##_create(void) \
{ \
    return new mapper_##nm##_t(); \
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _mapper_h_ */
