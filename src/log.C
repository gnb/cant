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

CVSID("$Id: log.C,v 1.4 2002-04-13 12:26:29 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


log_message_t::log_message_t(const char *msg, gboolean addnl)
{
    message_ = g_strdup(msg);
    add_newline_ = addnl;
    context_ = log_context_t::top()->format();
}

log_message_t::log_message_t(char *msg, gboolean addnl)
{
    message_ = msg;
    add_newline_ = addnl;
    context_ = log_context_t::top()->format();
}

log_message_t::~log_message_t()
{
    strdelete(message_);
    strdelete(context_);
}


log_message_t *
log_message_t::newf(const char *fmt, ...)
{
    log_message_t *lm;
    va_list args;
    
    va_start(args, fmt);
    lm = new log_message_t(g_strdup_vprintf(fmt, args));
    va_end(args);
    
    return lm;
}

void
log_message_t::emit() const
{
    if (context_ != 0)
	fputs(context_, stderr);
    if (message_ != 0)
	fputs(message_, stderr);
    if (add_newline_)
    	fputc('\n', stderr);
    fflush(stderr); /* JIC */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

log_context_t *log_context_t::stack_;

log_context_t::log_context_t()
{
    next_ = stack_;
    if (stack_ != 0)
    	stack_->prev_ = this;
    prev_ = 0;
    stack_ = this;
}

log_context_t::~log_context_t()
{
    assert(this == stack_);
    stack_ = next_;
    if (stack_ != 0)
    	stack_->prev_ = 0;
}

log_context_t *
log_context_t::top()
{
    return stack_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
log_context_t::format(FILE *fp)
{
    char *str = format();
    fputs(str, fp);
    g_free(str);
}

void
log_context_t::format_ancestors(FILE *fp)
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
log_simple_context_t::format()
{
    return g_strdup_printf("%s: ", name_);
}

void
log_simple_context_t::format(FILE *fp)
{
    fprintf(fp, "%s: ", name_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
log_file_context_t::format()
{
    return (lineno_ ? 
    	    	g_strdup_printf("%s:%d: ", filename_, lineno_) : 
    	    	g_strdup_printf("%s: ", filename_));
}

void
log_file_context_t::format(FILE *fp)
{
    if (lineno_)
	fprintf(fp, "%s:%d: ", filename_, lineno_);
    else
	fprintf(fp, "%s: ", filename_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define INDENT_STR  "  "

unsigned int log_tree_context_t::stack_depth_ = 0;

void
log_tree_context_t::format_self(FILE *fp)
{
    int i;

    for (i = depth_ ; i ; i--)
	fputs(INDENT_STR, fp);
    fprintf(fp, "[%s] ", name_);
    nmessages_++;
}

void
log_tree_context_t::format_ancestors(FILE *fp)
{
    if (nmessages_)
    	return;
    if (next_ != 0)
    	next_->format_ancestors(fp);
    format_self(fp);
    fputc('\n', fp);
}


char *
log_tree_context_t::format()
{
    int len;
    char *buf, *p;
    int i;
    
    if (next_ != 0)
	next_->format_ancestors(stderr);    //stderr=HACK

    len = strlen(INDENT_STR)*depth_ + 4 + strlen(name_);
    p = buf = new char[len];

    for (i = depth_ ; i ; i--)
    {
    	strcpy(p, INDENT_STR);
	p += strlen(p);
    }
    sprintf(p, "[%s] ", name_);

    nmessages_++;
    return buf;
}

void
log_tree_context_t::format(FILE *fp)
{
    if (next_ != 0)
	next_->format_ancestors(fp);
    format_self(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *severity_strs[log::_NUM_SEVERITY] = { 0, "WARNING", "ERROR" };
static unsigned int message_counts[log::_NUM_SEVERITY];

void
log::messagev(log::severity_t sev, const char *fmt, va_list args)
{
    log_context_t::top()->format(stderr);
    if (severity_strs[sev] != 0)
    {
    	fputs(severity_strs[sev], stderr);
	fputs(": ", stderr);
    }
    message_counts[sev]++;
    vfprintf(stderr, fmt, args);
    fflush(stderr); 	/* JIC */
}

void
log::messagef(log::severity_t sev, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    messagev(sev, fmt, args);
    va_end(args);
}

void
log::errorf(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    messagev(ERROR, fmt, args);
    va_end(args);
}

void
log::warningf(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    messagev(WARNING, fmt, args);
    va_end(args);
}

void
log::infof(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    messagev(INFO, fmt, args);
    va_end(args);
}

void
log::perror(const char *filename)
{
    errorf("%s: %s\n", filename, strerror(errno));
}

void
log::zero_message_counts()
{
    memset(message_counts, 0, sizeof(message_counts));
}

unsigned int
log::message_count(log::severity_t sev)
{
    return message_counts[sev];
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
