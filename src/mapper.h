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

#include "common.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
typedef struct mapper_s     	mapper_t;
typedef struct mapper_ops_s 	mapper_ops_t;


struct mapper_s
{
    mapper_ops_t *ops;
    char *from;
    char *to;
    void *private;
};

struct mapper_ops_s
{
    char *name;
    gboolean (*new)(mapper_t *);
    char *(*map)(mapper_t *, const char *filename);
    void (*delete)(mapper_t *);
};


mapper_t *mapper_new(const char *name, const char *from, const char *to);
void mapper_delete(mapper_t *);
char *mapper_map(mapper_t *, const char *filename);
void mapper_ops_register(mapper_ops_t *ops);
void mapper_ops_unregister(mapper_ops_t *ops);
void mapper_initialise_builtins(void);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _mapper_h_ */
