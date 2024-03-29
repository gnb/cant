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

#ifndef _common_h_
#define _common_h_ 1

/* Include autoconf defines */
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#include <glib.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define strassign(v, s) \
    do { \
	if ((v) != 0) \
	    g_free((v)); \
	(v) = g_strdup((s)); \
    } while(0)
    
#define strdelete(v) \
    do { \
    	if ((v) != 0) \
	    g_free((v)); \
	(v) = 0; \
    } while(0)

#define strnullnorm(v) \
    do { \
	if ((v) != 0 && *(v) == '\0') \
	{ \
	    g_free((v)); \
	    (v) = 0; \
	} \
    } while(0)

#define listdelete(v,type,dtor) \
    do { \
	while ((v) != 0) \
	{ \
    	    dtor((type *)(v)->data); \
    	    (v) = g_list_remove_link((v), (v)); \
	} \
    } while(0)

#define listclear(v) \
    do { \
	while ((v) != 0) \
	{ \
    	    (v) = g_list_remove_link((v), (v)); \
	} \
    } while(0)

#define boolassign(bv, s) \
    (bv) = strbool((s), (bv))

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if defined(__ELF__) && defined(__GNUC__)
#define _CVSID(u,s) __asm__ (".ident \"" s "\"" )
#else
#define _CVSID(u,s) static const char * const __cvsid##u[2] = {(s), (const char*)__cvsid}
#endif
#define CVSID(s) _CVSID(,s)


#ifdef __GNUC__
#define _printf_fmt(fmtidx,firsta) \
    __attribute (( format(printf, fmtidx, firsta) ))
#else
#define _printf_fmt(fmtidx,firsta)
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern void *operator new(size_t sz);

extern int strbool(const char *s, int deflt/*may be -1*/);

extern void strfree(char*);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


#endif /* _common_h_ */
