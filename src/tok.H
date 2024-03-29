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

class tok_t
{
public:
    /* ctors */
    tok_t(const char *str, const char *sep = 0);    /* const=>copy */
    tok_t(char *str, const char *sep = 0);  	    /* nonconst=>don't copy */
    /* dtor */
    ~tok_t();

    /*
     * Returns next token or 0 when last token reached.
     */
    const char *next();

private:
    void init(char *str, const char *sep);
    
    char *buf_;
    char *state_;
    const char *sep_;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _tok_h_ */
