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

#ifndef _fifo_pool_h_
#define _fifo_pool_h_ 1

#include "common.H"
#include "list.H"
#include "thread.H"
#include "string_var.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
class fifo_pool_t
{
private:
#if !THREADS_NONE
    cant_mutex_t lock_;
#endif
    string_var base_;
    list_t<char> available_;
    list_t<char> inuse_;
    
    static fifo_pool_t *instance_;
    
    gboolean create_unlocked();

public:
    fifo_pool_t(const char *base, int pre_size);
    ~fifo_pool_t();
    
    const char *get();
    void put(const char *);
    
    static fifo_pool_t *instance() { return instance_; }
};


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _fifo_pool_h_ */
