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

#include "tok.h"

CVSID("$Id: tok.c,v 1.1 2002-02-08 07:46:39 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tok_t *
tok_new(const char *str, const char *sep)
{
    return tok_new_m(g_strdup(str), sep);
}

tok_t *
tok_new_m(char *str, const char *sep)
{
    tok_t *tok;
    
    tok = new(tok_t);
    tok_init_m(tok, str, sep);
    return tok;
}

void
tok_delete(tok_t *tok)
{
    tok_free(tok);
    g_free(tok);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
tok_init(tok_t *tok, const char *str, const char *sep)
{
    tok_init_m(tok, g_strdup(str), sep);
}

void
tok_init_m(tok_t *tok, char *str, const char *sep)
{
    tok->buf = str;
    tok->state = 0;
    tok->sep = sep;
}

void
tok_free(tok_t *tok)
{
    g_free(tok->buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
tok_next(tok_t *tok)
{
    return strtok_r((tok->state == 0 ? tok->buf : 0), tok->sep, &tok->state);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
