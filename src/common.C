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

#include "common.H"

CVSID("$Id: common.C,v 1.2 2002-03-29 13:56:49 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern char *argv0;

void *
operator new(size_t sz)
{
    void *x;
    
    if ((x = g_malloc0(sz)) == 0)
    {
    	static const char message[] = ": no memory, exiting\n";
	
	write(2, argv0, strlen(argv0));
	write(2, message, sizeof(message)-1);
	exit(1);
    }
    
    return x;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
strbool(const char *s, int deflt)
{
    int res = deflt;
    
    if (!strcasecmp(s, "true") ||
    	!strcasecmp(s, "yes") ||
    	!strcasecmp(s, "on") ||
    	!strcmp(s, "1"))
    	res = TRUE;
    else if (!strcasecmp(s, "false") ||
    	     !strcasecmp(s, "no") ||
    	     !strcasecmp(s, "off") ||
    	     !strcmp(s, "0"))
    	res = FALSE;
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
