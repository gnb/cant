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

#ifndef _depfile_h_
#define _depfile_h_ 1

#include "common.H"
#include "estring.H"
#include "string_var.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
class depfile_reader_t
{
private:
    int mygetc(FILE *fp);
    gboolean read_word(FILE *fp, estring &e, char delim);
    int read_char(FILE *fp);


protected:    
    string_var filename_;
    unsigned int lineno_;

    virtual void begin_read();
    virtual void open_error();
    virtual void syntax_error();
    virtual void add_dep(const char *from, const char *to) = 0;
    virtual void end_read();
    
public:
    depfile_reader_t(const char *filename);
    virtual ~depfile_reader_t();

    gboolean read();
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _depfile_h_ */
