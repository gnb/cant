/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * Copyright (c) 2001 Greg Banks
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

#include "common.h"

#if !THREADS_NONE

typedef struct queue_s	    queue_t;

queue_t *queue_new(unsigned int maxlen);
void queue_delete(queue_t *);

/* Blocking put and get */
void queue_put(queue_t *, void *);
void *queue_get(queue_t *);

/* Non-blocking put and get */
gboolean queue_tryput(queue_t *, void *);
/* Returns 0 if queue empty */
void *queue_tryget(queue_t *);

#endif /* !THREADS_NONE */

#endif /* _cant_queue_h_ */
