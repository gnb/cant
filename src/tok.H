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

#ifndef _tok_h_
#define _tok_h_ 1

#include "common.H"

/*
 * A simple re-entrant string tokenizer, wraps strtok_r().
 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct tok_s	tok_t;

struct tok_s
{
    char *buf;
    char *state;
    const char *sep;
};

/* for allocating a new object */
tok_t *tok_new(const char *str, const char *sep);
tok_t *tok_new_m(char *str, const char *sep);
void tok_delete(tok_t *);

/* for an auto object */
void tok_init(tok_t *tok, const char *str, const char *sep);
void tok_init_m(tok_t *tok, char *str, const char *sep);
void tok_free(tok_t *);

const char *tok_next(tok_t *tok);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _tok_h_ */
