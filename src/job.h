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

#ifndef _cant_job_h_
#define _cant_job_h_ 1

#include "common.h"
#include "strarray.h"

typedef struct job_s job_t;
typedef struct job_ops_s job_ops_t;

struct job_ops_s
{
    gboolean (*execute)(void *userdata);
    char *(*describe)(void *userdata);
    void (*delete)(void *userdata);
};

gboolean job_init(unsigned int num_workers);
job_t *job_add(const char *name, job_ops_t *ops, void *userdata);
job_t *job_add_command(const char *name, strarray_t *command, strarray_t *env);
void job_add_depend(job_t *job, const char *depname);
gboolean job_pending(void);
void job_clear(void);
gboolean job_run(void);
gboolean job_immediate(job_ops_t *ops, void *userdata);
gboolean job_immediate_command(strarray_t *command, strarray_t *env);


#endif /* _cant_job_h_ */
