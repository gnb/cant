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

#include "depfile.H"
#include "log.H"

CVSID("$Id: depfile.C,v 1.1 2002-04-21 04:01:40 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

depfile_reader_t::depfile_reader_t(const char *filename)
 :  filename_(filename),
    lineno_(0)
{
}

depfile_reader_t::~depfile_reader_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

// read a char, converting backslash at end of line to a space.
int
depfile_reader_t::mygetc(FILE *fp)
{
    int c = fgetc(fp);
    if (c == '\\')
    {
    	if ((c = fgetc(fp)) == '\n')
	{
	    c =  ' ';
	    lineno_++;
	}
	else
	{
	    ungetc(c, fp);
	    c =  '\\';
	}
    }
    if (c == '\n')
    	lineno_++;
    return c;
}

#define isws2(c) \
    ((c) == ' ' || (c) == '\t')

gboolean
depfile_reader_t::read_word(FILE *fp, estring &e, char delim)
{
    int c;
    
    // skip initial whitespace
    while ((c = mygetc(fp)) != EOF && isws2(c))
    	;
    if (c == EOF)
    	return FALSE;
	
    e.append_char(c);
    while ((c = mygetc(fp)) != EOF && !isspace(c) && (c != delim))
    	e.append_char(c);
    
    if (c == delim)
    	ungetc(c, fp);
	
    return TRUE;
}

int
depfile_reader_t::read_char(FILE *fp)
{
    int c;
    
    // skip initial whitespace
    while ((c = mygetc(fp)) != EOF && isws2(c))
    	;
    return c;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

// default definitions of virtual callbacks
void
depfile_reader_t::begin_read()
{
}

void
depfile_reader_t::end_read()
{
}

void
depfile_reader_t::open_error()
{
    log::perror(filename_);
}

void
depfile_reader_t::syntax_error()
{
    log_file_context_t context(filename_, lineno_);
    log::errorf("Syntax error in dependencies\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
depfile_reader_t::read()
{
    FILE *fp;
    int c;
    enum { FROM, COLON, TO, DONE } state = FROM;
    estring from, to;

    begin_read();
        
    if ((fp = fopen(filename_, "r")) == 0)
    {
    	open_error();
	return FALSE;
    }
    
    while (state != DONE)
    {
    	switch (state)
	{
	case FROM:
	    from.truncate();
    	    state = (read_word(fp, from, ':') ? COLON: DONE);
	    break;

    	case COLON:
    	    c = read_char(fp);
	    if (c != ':')
	    {
	    	syntax_error();
		state = DONE;
	    }
	    state = TO;
	    break;

    	case TO:
    	    to.truncate();
	    if ((c = read_char(fp)) == '\n')
	    {
		state = FROM;
		break;
	    }
	    to.append_char(c);
	    if (read_word(fp, to, '\n'))
		add_dep(from.data(), to.data());
	    break;
	}
    }

    end_read();
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
