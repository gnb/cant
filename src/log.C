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

#include "log.H"

CVSID("$Id: log.C,v 1.1 2002-03-29 12:36:26 gnb Exp $");

typedef struct log_context_s	log_context_t;

struct logmsg_s
{
    /* saved formatted message */
    char *message;
    gboolean add_nl;
    /* snapshot of log context */
    int depth;
    const char *context;    	/* not saved */
};

struct log_context_s
{
    const char *name;
    int nmessages;
};

static GList *log_context_stack;    /* stack of log_context_s */

/* TODO: fmeh, do this properly */
extern gboolean verbose;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
log_show_context(int depth, const char *context)
{
    while (depth--)
	fputs("  ", stderr);
    fprintf(stderr, "[%s] ", context);
}

void
logv(const char *fmt, va_list args)
{
    /* TODO: handle embedded newlines */
    /* TODO: lock between threads */
    
    if (log_context_stack != 0)
    {
	/* show all intervening contexts since we last emitted a message */
    	GList *iter, *prev = 0;
    	log_context_t *lc;
	int n;

	for (iter = log_context_stack, n = g_list_length(log_context_stack) ;
	     iter != 0 && ((log_context_t *)iter->data)->nmessages == 0 ;
	     prev = iter, iter = iter->next, n--)
	    ;

    	if (iter != log_context_stack)
	    n++;
	for (iter = prev ;
	     iter != 0 && iter != log_context_stack ;
	     iter = iter->prev, n++)
	{
	    lc = (log_context_t *)iter->data;
	    log_show_context(n, lc->name);
	    fputc('\n', stderr);
	    lc->nmessages++;
	}

	lc = (log_context_t *)log_context_stack->data;
	log_show_context(n, lc->name);
	lc->nmessages++;
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
log_perror(const char *filename)
{
    logf("%s: %s\n", filename, strerror(errno));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

logmsg_t *
logmsg_newm(char *str)
{
    logmsg_t *lm;
    
    lm = new(logmsg_t);
    lm->depth = g_list_length(log_context_stack);
    lm->context = ((log_context_t *)log_context_stack->data)->name;
    lm->message = str;
    
    return lm;
}

logmsg_t *
logmsg_newnm(char *str)
{
    logmsg_t *lm = logmsg_newm(str);
    
    lm->add_nl = TRUE;
    
    return lm;
}

logmsg_t *
logmsg_newf(const char *fmt, ...)
{
    logmsg_t *lm;
    va_list args;
    
    va_start(args, fmt);
    lm = logmsg_newm(g_strdup_vprintf(fmt, args));
    va_end(args);
    
    return lm;
}

void
logmsg_delete(logmsg_t *lm)
{
    strdelete(lm->message);
    g_free(lm);
}

void
logmsg_emit(logmsg_t *lm)
{
    log_show_context(lm->depth, lm->context);
    if (lm->message != 0)
	fputs(lm->message, stderr);
    if (lm->add_nl)
    	fputc('\n', stderr);
    fflush(stderr);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
log_push_context(const char *name)
{
    log_context_t *lc;
    
    lc = new(log_context_t);
    lc->name = name;
    
    log_context_stack = g_list_prepend(log_context_stack, lc);
}

void
log_pop_context(void)
{
    if (log_context_stack != 0)
    {
	log_context_t *lc = (log_context_t *)log_context_stack->data;
    	log_context_stack = g_list_remove_link(log_context_stack, log_context_stack);
	g_free(lc);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
