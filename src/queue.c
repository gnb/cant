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

#include "queue.h"
#include "thread.h"

CVSID("$Id: queue.c,v 1.1 2001-11-13 03:02:55 gnb Exp $");

#if !THREADS_NONE

struct queue_s
{
    void **slots;  	    	/* queue slots */
    unsigned putptr;
    unsigned int getptr;
    unsigned int maxlen;
    cant_sem_t empty;    	/* counts empty slots */
    cant_sem_t full;     	/* counts full slots */
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

queue_t *
queue_new(unsigned int maxlen)
{
    queue_t *q;
    
    if ((q = g_new0(queue_t, 1)) == 0)
    	return 0;
    if ((q->slots = g_new0(void*, maxlen)) == 0)
    {
    	g_free(q);
    	return 0;
    }
    cant_sem_init(&q->empty, /*value*/maxlen);
    cant_sem_init(&q->full, /*value*/0);
    q->maxlen = maxlen;
    
    return q;
}

void
queue_delete(queue_t *q)
{
    /* check for failure due to waiting threads */
    cant_sem_destroy(&q->empty);
    cant_sem_destroy(&q->full);
    g_free(q->slots);
    g_free(q);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
queue_put_unlocked(queue_t *q, void *item)
{
    q->slots[q->putptr] = item;
    
    if (++q->putptr == q->maxlen)
    	q->putptr = 0;
}

void
queue_put(queue_t *q, void *item)
{
    cant_sem_wait(&q->empty);
    queue_put_unlocked(q, item);
    cant_sem_post(&q->full);
}

gboolean
queue_tryput(queue_t *q, void *item)
{
    if (!cant_sem_trywait(&q->empty))
    	return FALSE;
    queue_put_unlocked(q, item);
    cant_sem_post(&q->full);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void *
queue_get_unlocked(queue_t *q)
{
    void *item;
    
    item = q->slots[q->getptr];
    
    if (++q->getptr == q->maxlen)
    	q->getptr = 0;
	
    return item;
}

void *
queue_get(queue_t *q)
{
    void *item;
    
    cant_sem_wait(&q->full);
    item = queue_get_unlocked(q);
    cant_sem_post(&q->empty);
    
    return item;
}

void *
queue_tryget(queue_t *q)
{
    void *item;
    
    if (!cant_sem_trywait(&q->full))
    	return 0;
    item = queue_get_unlocked(q);
    cant_sem_post(&q->empty);
    
    return item;
}

#endif /* !THREADS_NONE */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
