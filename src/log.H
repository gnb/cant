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
#include "list.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class log_context_t
{
protected:
    static log_context_t *stack_;
    
    log_context_t *next_;
    log_context_t *prev_;

    log_context_t();
    virtual ~log_context_t();
    
public:    
    static log_context_t *top();

    virtual char *format() = 0;
    virtual void format(FILE *);
    virtual void format_ancestors(FILE *);
};


class log_simple_context_t : public log_context_t
{
private:
    const char *name_;

    char *format();
    void format(FILE *);

public:
    log_simple_context_t(const char *name)
     :  name_(name)
    {
    }
};

class log_file_context_t : public log_context_t
{
private:
    const char *filename_;
    unsigned int lineno_;

    char *format();
    void format(FILE *);

public:
    log_file_context_t(const char *filename, unsigned int lineno)
     :  filename_(filename),
     	lineno_(lineno)
    {
    }
};


class log_tree_context_t : public log_context_t
{
private:
    const char *name_;
    unsigned int nmessages_;
    unsigned int depth_;
    static unsigned int stack_depth_;

    char *format();
    void format(FILE *);
    void format_ancestors(FILE *);
    void format_self(FILE *);


public:
    log_tree_context_t(const char *name)
     :  name_(name),
        nmessages_(0)
    {
	depth_ = ++stack_depth_;
    }
    
    ~log_tree_context_t()
    {
	stack_depth_--;
    }
};



class log_message_t
{
private:
    /* saved formatted message */
    char *message_;
    gboolean add_newline_;
    /* snapshot of log context */
    char *context_;

public:
    log_message_t(const char *, gboolean addnl = FALSE);
    log_message_t(char *, gboolean addnl = FALSE);
    ~log_message_t();

    static log_message_t *newf(const char *fmt, ...) _printf_fmt(1, 2);

    void emit() const;
};



class log
{
private:
    log();  	// can't instantiate

public:

    enum severity_t
    {
	INFO,
	WARNING,
	ERROR,
	
	_NUM_SEVERITY
    };

    static void messagev(severity_t, const char *fmt, va_list) _printf_fmt(2,0);
    static void messagef(severity_t, const char *fmt, ...) _printf_fmt(2,3);
    static void errorf(const char *fmt, ...) _printf_fmt(1,2);
    static void warningf(const char *fmt, ...) _printf_fmt(1,2);
    static void infof(const char *fmt, ...) _printf_fmt(1,2);
    static void perror(const char *str);
    
    static void zero_message_counts();
    static unsigned int message_count(severity_t);
};


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _cant_log_h_ */
