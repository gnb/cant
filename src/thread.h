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

#ifndef _cant_thread_h_
#define _cant_thread_h_ 1

#include "common.h"

#if THREADS_POSIX

#include <pthread.h>
#include <semaphore.h>

/* Trivial threads portability layer */
typedef sem_t	    	    	cant_sem_t;
typedef pthread_mutex_t	    	cant_mutex_t;
typedef pthread_t   	    	cant_thread_t;

#define cant_sem_init(s, v) 	sem_init((s), 0, (v))
#define cant_sem_wait(s)    	sem_wait((s))
#define cant_sem_trywait(s)    	(sem_trywait((s)) == 0)
#define cant_sem_post(s)    	sem_post((s))
#define cant_sem_destroy(s)    	sem_destroy((s))

/*
#define cant_mutex_init(m) 	pthread_mutex_init((m), 0)
#define cant_mutex_lock(m)    	pthread_mutex_lock((m))
#define cant_mutex_unlock(m)    pthread_mutex_unlock((m))
*/

#define cant_thread_create(th,fn,arg) \
    	(pthread_create((th), 0, (fn), (arg)) ? -1 : 0)

#endif	/* THREADS_POSIX */


#endif /* _cant_thread_h_ */
