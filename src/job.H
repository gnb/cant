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

#ifndef _cant_job_h_
#define _cant_job_h_ 1

#include "common.H"
#include "strarray.H"
#include "list.H"
#include "log.H"
#include "string_var.H"

class job_op_t
{
private:
public:
    job_op_t();
    virtual ~job_op_t();
    
    virtual gboolean execute() = 0;
    virtual char *describe() const = 0;
};

/*
 * Most common case is a job which runs a UNIX command
 * constructed from taglists or other means.
 */
class command_job_op_t : public job_op_t
{
private:
    /* TODO: need to make a snapshot of log context */
    strarray_t *command_;
    strarray_t *env_;
    log_message_t *logmessage_;
    string_var directory_;

    gboolean execute();
    char *describe() const;

public:
    /* ctor */
    command_job_op_t(strarray_t *command, strarray_t *env, log_message_t *logmsg);
    /* dtor */
    ~command_job_op_t();
};

class job_t
{
private:
    enum state_t
    {
	UNKNOWN,
	RUNNABLE,
	RUNNING,
	UPTODATE,
	FAILED,

	NUM_STATES
    };
    
    string_var name_;
    unsigned int serial_;    	/* for preserving order */
    state_t state_;
    list_t<job_t> depends_up_;	    	/* job_t's that depend on me */
    list_t<job_t> depends_down_;     	/* job_t's I depend on */
    job_op_t *op_;
    gboolean result_;
    
    static int state_count_[NUM_STATES];

    void set_state(state_t);
    state_t calc_new_state() const;

    static int compare_by_serial(const job_t*, const job_t*);
    static void initialise_one(const char *key, job_t *job, void *userdata);
    static gboolean clear_one(const char *key, job_t *value, void *userdata);
#if !THREADS_NONE
    static void *worker_thread(void *arg);
    static void finish_job(job_t *);
    static void start_job(job_t *);
    static gboolean main_thread();
#endif
    static gboolean scalar();
    
#if DEBUG
    static void dump_one(const char *key, job_t *job, void *userdata);
    void dump() const;
    static void dump_all();
    static const char *state_name(state_t state);
    char *describe() const;
#endif

    /* ctor */
    job_t(const char *name);
    /* dtor */
    ~job_t();
    
public:
    const char *name() const { return name_; }

    /* call this once at start of programs to start worker threads */
    static gboolean init(unsigned int num_workers);
    
    /* return count of pending jobs */
    static gboolean pending();
    
    /* shut down all pending jobs */
    static void clear();
    
    /* execute all jobs */
    static gboolean run();

    /* add a pending job */
    static job_t *add(const char *name, job_op_t *);
    
    /* make this job depend on another job name */
    void add_depend(const char *depname);

    /* wait for all pending jobs, run this op and destroy it */
    static gboolean immediate(job_op_t *);
    
    friend class shut_mr_silly_compiler;
};


#endif /* _cant_job_h_ */
