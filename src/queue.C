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

#include "queue.H"
#include "thread.H"

CVSID("$Id: queue.C,v 1.2 2002-03-29 16:33:50 gnb Exp $");

#if !THREADS_NONE

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

voidqueue_t::voidqueue_t(unsigned int maxlen)
{
    slots_ = new void*[maxlen];
    cant_sem_init(&empty_, /*value*/maxlen);
    cant_sem_init(&full_, /*value*/0);
    maxlen_ = maxlen;
}

voidqueue_t::~voidqueue_t()
{
    /* check for failure due to waiting threads */
    cant_sem_destroy(&empty_);
    cant_sem_destroy(&full_);
    delete[] slots_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
voidqueue_t::put_unlocked(void *item)
{
    slots_[putptr_] = item;
    
    if (++putptr_ == maxlen_)
    	putptr_ = 0;
}

void
voidqueue_t::put(void *item)
{
    cant_sem_wait(&empty_);
    put_unlocked(item);
    cant_sem_post(&full_);
}

gboolean
voidqueue_t::tryput(void *item)
{
    if (!cant_sem_trywait(&empty_))
    	return FALSE;
    put_unlocked(item);
    cant_sem_post(&full_);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void *
voidqueue_t::get_unlocked()
{
    void *item;
    
    item = slots_[getptr_];
    
    if (++getptr_ == maxlen_)
    	getptr_ = 0;
	
    return item;
}

void *
voidqueue_t::get()
{
    void *item;
    
    cant_sem_wait(&full_);
    item = get_unlocked();
    cant_sem_post(&empty_);
    
    return item;
}

void *
voidqueue_t::tryget()
{
    void *item;
    
    if (!cant_sem_trywait(&full_))
    	return 0;
    item = get_unlocked();
    cant_sem_post(&empty_);
    
    return item;
}

#endif /* !THREADS_NONE */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
