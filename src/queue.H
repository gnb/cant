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

#ifndef _cant_queue_h_
#define _cant_queue_h_ 1

#include "common.H"

#if !THREADS_NONE

#include "thread.H"

// thread-safe producer-consumer queue
class voidqueue_t
{
private:
    void **slots_;  	    	/* queue slots */
    unsigned putptr_;
    unsigned int getptr_;
    unsigned int maxlen_;
    cant_sem_t empty_;    	/* counts empty slots */
    cant_sem_t full_;     	/* counts full slots */

    void put_unlocked(void *);
    void *get_unlocked();

public:
    /* ctor */
    voidqueue_t(unsigned int maxlen);
    /* dtor */
    ~voidqueue_t();

    /* Blocking put and get */
    void put(void *);
    void *get();

    /* Non-blocking put and get */
    gboolean tryput(void *);
    /* Returns 0 if queue empty */
    void *tryget();
};

// typesafe template wrapper for voidqueue
template<class T> class queue_t
{
private:
    voidqueue_t queue_;
    
public:
    /* ctor */
    queue_t(unsigned int maxlen)
     :  queue_(maxlen)
    {
    }
    
    /* dtor */
    ~queue_t()
    {
    }

    /* Blocking put and get */
    void put(T *t)
    {
    	queue_.put((void *)t);
    }
    
    T *get()
    {
    	return (T *)queue_.get();
    }

    /* Non-blocking put and get */
    gboolean tryput(T *t)
    {
    	return queue_.tryput((void *)t);
    }
    
    /* Returns 0 if queue empty */
    T *tryget()
    {
    	return (T *)queue_.tryget();
    }
};


#endif /* !THREADS_NONE */

#endif /* _cant_queue_h_ */
