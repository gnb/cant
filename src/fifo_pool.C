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

#include "fifo_pool.H"
#include "string_var.H"
#include "log.H"

CVSID("$Id: fifo_pool.C,v 1.1 2002-04-21 04:01:40 gnb Exp $");

#if THREADS_NONE
#define LOCK
#define UNLOCK
#else
#define LOCK	    cant_mutex_lock(&lock_)
#define UNLOCK	    cant_mutex_unlock(&lock_)
#endif

fifo_pool_t *fifo_pool_t::instance_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fifo_pool_t::fifo_pool_t(const char *base, int pre_size)
 :  base_(base)
{
    assert(instance_ == 0);
    instance_ = this;

#if !THREADS_NONE
    cant_mutex_init(&lock_);
#endif

    int i;
    for (i = 0 ; i < pre_size ; i++)
    	create_unlocked();
}

static void
remove_one_fifo(char *fifo)
{
    unlink(fifo);
    g_free(fifo);
}

fifo_pool_t::~fifo_pool_t()
{
    assert(instance_ == this);
    instance_ = 0;

    available_.apply_remove(remove_one_fifo);
    inuse_.apply_remove(remove_one_fifo);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
fifo_pool_t::create_unlocked()
{
    string_var fifo = g_strconcat("/tmp/", base_.data(), "XXXXXX", 0);

    if (mktemp((char *)fifo.data()) == 0)
    {
	log::perror(fifo);
	return FALSE;
    }
    unlink(fifo);
    if (mknod(fifo, S_IFIFO|0600, 0) < 0)
    {
	log::perror(fifo);
	return FALSE;
    }
    available_.prepend(fifo.take());
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
fifo_pool_t::get()
{
    char *fifo;
    
    LOCK;
    if (available_.first() == 0 && !create_unlocked())
    {
    	UNLOCK;
	return 0;
    }
    fifo = available_.remove_head();
    UNLOCK;
    
    return fifo;
}

void
fifo_pool_t::put(const char *fifo)
{
    LOCK;
    inuse_.remove((char *)fifo);
    available_.prepend((char *)fifo);
    UNLOCK;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
