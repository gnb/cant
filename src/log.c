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

#include "cant.h"

CVSID("$Id: log.c,v 1.4 2001-11-13 04:08:05 gnb Exp $");

static GList *log_context_stack;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
logv(const char *fmt, va_list args)
{
    /* TODO: handle embedded newlines */
    /* TODO: lock between threads */
    
    if (log_context_stack != 0)
    {
    	int n = g_list_length(log_context_stack);
	while (n--)
	    fputs("  ", stderr);
	fprintf(stderr, "[%s] ", (const char *)log_context_stack->data);
    }

    vfprintf(stderr, fmt, args);
    fflush(stderr);
}

void
logf(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    logv(fmt, args);
    va_end(args);
}

void
logperror(const char *filename)
{
    logf("%s: %s\n", filename, strerror(errno));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
log_push_context(const char *name)
{
    log_context_stack = g_list_prepend(log_context_stack, (gpointer)name);
}

void
log_pop_context(void)
{
    if (log_context_stack != 0)
    	log_context_stack = g_list_remove_link(log_context_stack, log_context_stack);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
