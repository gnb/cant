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

#ifndef _cant_log_h_
#define _cant_log_h_ 1

#include "common.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct logmsg_s     logmsg_t;

/*
 * Format and emit a message immediately in the
 * current log context.
 */
void logf(const char *fmt, ...);
void logv(const char *fmt, va_list a);

/* log version of perror() */ 
void log_perror(const char *filename);

/*
 * Allocate and format a log message, including
 * the current log context, for later emission.
 */
logmsg_t *logmsg_newf(const char *fmt, ...);
/*
 * Allocate a new log message, taking over the
 * new string `str' as the text of the message.
 */
logmsg_t *logmsg_newm(char *str);
/*
 * Like logmsg_newm() except add a newline at
 * the end of the message -- useful for expanded
 * and un-newlined messages from XML attributes.
 */
logmsg_t *logmsg_newnm(char *str);
/* Emit the saved log message. */
void logmsg_emit(logmsg_t *);
/* Destroy a log message */
void logmsg_delete(logmsg_t *);

/*
 * Manipulate the current log context.  Note
 * that context names are *not* saved, they are
 * assumed to be allocated in some long-lived
 * structure e.g. tasks or targets.
 */
void log_push_context(const char *name);
void log_pop_context(void);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_log_h_ */
