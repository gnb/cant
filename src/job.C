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

#include "job.H"
#include "thread.H"
#include "filename.H"

#if !THREADS_NONE
#include "thread.H"
#include "queue.H"
#endif

CVSID("$Id: job.C,v 1.2 2002-03-29 13:57:32 gnb Exp $");


typedef enum
{
    UNKNOWN,
    RUNNABLE,
    RUNNING,
    UPTODATE,
    FAILED,
    
    NUM_STATES
} job_state_t;

struct job_s
{
    char *name;
    unsigned int serial;    	/* for preserving order */
    job_state_t state;
    GList *depends_up;	    	/* job_t's that depend on me */
    GList *depends_down;     	/* job_t's I depend on */
    job_ops_t *ops;
    void *userdata;
    gboolean result;
};


extern int process_run(strarray_t *command, strarray_t *env, const char *dir);

static GHashTable *all_jobs;
static GList *runnable_jobs;
static int state_count[NUM_STATES];

#if !THREADS_NONE
/*
 * Start queue is used to farm out jobs to worker threads.
 * It is at most one job long, to allow the main thread to
 * make decisions about downstream jobs at the latest
 * possible time.  Both put and get operations block.
 */
static queue_t *start_queue;

/*
 * Finish queue is used to return finished jobs and their
 * associated results from worker threads back to the main
 * thread for incorporation in global data.  It's length
 * is bounded by num_workers + length(start_queue).  The
 * put operation should never block; both blocking and non-
 * blocking get operations are used at various times.
 */
static queue_t *finish_queue;

static unsigned int num_workers;
#endif

typedef struct
{
    /* TODO: need to make a snapshot of log context */
    strarray_t *command;
    strarray_t *env;
    logmsg_t *logmessage;
    char *directory;
} job_process_private_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
job_process_execute(void *userdata)
{
    job_process_private_t *jpp = (job_process_private_t *)userdata;
    
    if (jpp->logmessage != 0)
    	logmsg_emit(jpp->logmessage);
    return process_run(jpp->command, jpp->env, jpp->directory);
}

static char *
job_process_describe(void *userdata)
{
    job_process_private_t *jpp = (job_process_private_t *)userdata;

    return jpp->command->join(" ");
}

static void
job_process_delete(void *userdata)
{
    job_process_private_t *jpp = (job_process_private_t *)userdata;

    if (jpp->command != 0)
	delete jpp->command;
    if (jpp->env != 0)
	delete jpp->env;
    strdelete(jpp->directory);
	
    if (jpp->logmessage != 0)
	logmsg_delete(jpp->logmessage);
	
    g_free(jpp);    
}

static job_ops_t job_process_ops = 
{
    job_process_execute,
    job_process_describe,
    job_process_delete
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static job_t *
job_new(const char *name)
{
    job_t *job;
    
    job = g_new0(job_t, 1);
    
    strassign(job->name, name);
    job->state = UNKNOWN;
    state_count[UNKNOWN]++;
    
    g_hash_table_insert(all_jobs, job->name, job);

    return job;
}

static void
job_delete(job_t *job)
{
    if (job->ops != 0 && job->ops->dtor != 0)
    	(*job->ops->dtor)(job->userdata);

    strdelete(job->name);
    state_count[job->state]--;
    
    while (job->depends_up != 0)
    	job->depends_up = g_list_remove_link(job->depends_up, job->depends_up);
    while (job->depends_down != 0)
    	job->depends_down = g_list_remove_link(job->depends_down, job->depends_down);

    g_free(job);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static job_t *
job_find(const char *name)
{
    return (job_t *)g_hash_table_lookup(all_jobs, name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if DEBUG

static const char *
job_state_name(const job_t *job)
{
    static const char *names[] = 
    {
    	"UNKNOWN",
	"RUNNABLE",
	"RUNNING",
	"UPTODATE",
	"FAILED"
    };
    return names[job->state];
}

static char *
job_describe(const job_t *job)
{
    if (job->ops == 0)
    	return g_strdup("-undefined-");
    if (job->ops->describe == 0)
    	return g_strdup("-undescribed-");
    return (*job->ops->describe)(job->userdata);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

job_t *
job_add(const char *name, job_ops_t *ops, void *userdata)
{
    job_t *job;
    static unsigned int serial = 0;

    if ((job = job_find(name)) != 0)
    {
    	if (job->ops != 0)
	{
	    fprintf(stderr, "Duplicate job \"%s\"\n", name);
	    return 0;
	}
    }
    else
    {
	job = job_new(name);
    }
	
    job->serial = ++serial;
    job->ops = ops;
    job->userdata = userdata;
    
#if DEBUG
    {
    	char *str = job_describe(job);
    	fprintf(stderr, "job_add: serial=%u name=\"%s\" description=\"%s\"\n",
	    	    job->serial, job->name, str);
	g_free(str);
    }
#endif
    
    return job;
}


job_t *
job_add_command(
    const char *name, 
    strarray_t *command,
    strarray_t *env,
    logmsg_t *logmessage)
{
    job_process_private_t *jpp;
    
    jpp = new(job_process_private_t);
    jpp->command = command;
    jpp->env = env;
    jpp->logmessage = logmessage;
    jpp->directory = g_strdup(file_top_dir());
    
    return job_add(name, &job_process_ops, jpp);
}

void
job_add_depend(job_t *job, const char *depname)
{
    job_t *dep;
    
    if ((dep = job_find(depname)) == 0)
    {
    	/* create an undefined job for later definition */
	dep = job_new(depname);
    }
    
    /* setup depends links */
    job->depends_down = g_list_append(job->depends_down, dep);
    dep->depends_up = g_list_append(dep->depends_up, job);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static job_state_t
job_calc_new_state(job_t *job)
{
    GList *iter;
    
    for (iter = job->depends_down ; iter != 0 ; iter = iter->next)
    {
    	job_t *down = (job_t *)iter->data;
	
	if (down->state == FAILED)
	    return FAILED;
	if (down->state != UPTODATE)
	    return UNKNOWN;
    }

    /* all deps are UPTODATE */
    
    if (file_exists(job->name) == 0)
    	return UPTODATE;
    
    return RUNNABLE;
}

static int
job_compare_by_serial(gconstpointer c1, gconstpointer c2)
{
    const job_t *j1 = (const job_t*)c1;
    const job_t *j2 = (const job_t*)c2;
    
    if (j1->serial > j2->serial)
    	return 1;
    if (j1->serial < j2->serial)
    	return -1;
    return 0;
}

static void
job_set_state(job_t *job, job_state_t newstate)
{
    GList *iter;

    if (job->state == newstate)
    	return;     /* nothing to see here, move along */
	
    /* runnable_jobs keeps track of all RUNNABLE jobs in serial order */
    if (newstate == RUNNABLE)
	runnable_jobs = g_list_insert_sorted(runnable_jobs, job,
	    	    	    	    	     job_compare_by_serial);
    if (job->state == RUNNABLE)
	runnable_jobs = g_list_remove(runnable_jobs, job);

    /* keep track of how many jobs are in each state */
    state_count[newstate]++;
    state_count[job->state]--;

    /* actually update the state variable */
    job->state = newstate;
    
#if DEBUG
    fprintf(stderr, "Main: job \"%s\" becomes %s\n",
    	    	    job->name, job_state_name(job));
#endif

    /* propagate the state change up the dependency graph */
    switch (newstate)
    {
    case FAILED:
    case UPTODATE:
    case UNKNOWN:
	for (iter = job->depends_up ; iter != 0 ; iter = iter->next)
	{
    	    job_t *up = (job_t *)iter->data;

	    job_set_state(up, job_calc_new_state(up));
	}
	break;
    default:
    	break;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
job_initialise_one(gpointer key, gpointer value, gpointer userdata)
{
    job_t *job = (job_t *)value;
    
    if (job->depends_down == 0)
    {
    	if (job->ops == 0 && file_exists(job->name) < 0)
	{
	    fprintf(stderr, "No rule to make \"%s\"\n", job->name);
	    job_set_state(job, FAILED);
	}
	else
	    job_set_state(job, UPTODATE);
    }
}

static void
job_initialise_states(void)
{
    g_hash_table_foreach(all_jobs, job_initialise_one, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG

static void
job_dump(const job_t *job)
{
    char *desc;
    GList *iter;
    
    desc = job_describe(job);
    fprintf(stderr, "    job 0x%08lx {\n\tserial = %u\n\tname = \"%s\"\n\tstate = %s\n\tdescription = \"%s\"\n\tdepends_down =",
    	       (unsigned long)job,
	       job->serial,
	       job->name,
	       job_state_name(job),
	       desc);
    for (iter = job->depends_down ; iter != 0 ; iter = iter->next)
    {
    	job_t *down = (job_t *)iter->data;
	
	fprintf(stderr, " \"%s\"", down->name);
    }
    fprintf(stderr, "\n    }\n");
    g_free(desc);
}

static void
job_dump_one(gpointer key, gpointer value, gpointer userdata)
{
    job_dump((job_t *)value);
}

void
job_dump_all(void)
{
    fprintf(stderr, "all_jobs = \n");
    g_hash_table_foreach(all_jobs, job_dump_one, 0);
    fflush(stderr);
}

#endif /* DEBUG */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if !THREADS_NONE

static void *
worker_thread(void *arg)
{
    job_t *job;
#if DEBUG
    int threadno = (int)arg;
#endif
    
#if DEBUG
    fprintf(stderr, "Worker%d: pid=%d\n", threadno, (int)getpid());
#endif

    for (;;)
    {
    	/* block until a job is available from the main thread */
    	job = (job_t *)queue_get(start_queue);

    	/* run the job */
#if DEBUG
	fprintf(stderr, "Worker%d: starting job \"%s\"\n", threadno, job->name);
#endif
	job->result = (*job->ops->execute)(job->userdata);
#if DEBUG
	fprintf(stderr, "Worker%d: finished job \"%s\"\n", threadno, job->name);
#endif
	
	/* put the finished job on the queue back to the main thread */
	queue_put(finish_queue, job);
    }
}

static void
finish_job(job_t *job)
{
#if DEBUG
    fprintf(stderr, "Main: received finished job \"%s\", %s\n",
    	    	job->name,
		(job->result ? "success" : "failure"));
#endif
    job_set_state(job, (job->result ? UPTODATE : FAILED));
}

static void
start_job(job_t *job)
{
    job_set_state(job, RUNNING);
    queue_put(start_queue, job);
}

static gboolean
main_thread(void)
{
    job_t *job;
        
#if DEBUG
    fprintf(stderr, "Main: starting\n");
#endif
    while ((state_count[UNKNOWN] > 0 ||
            state_count[RUNNABLE] > 0 ||
            state_count[RUNNING] > 0 ) &&
	   state_count[FAILED] == 0)
    {
    	/* Handle any finished jobs, to keep the finish queue short */
	while ((job = (job_t *)queue_tryget(finish_queue)) != 0)
	    finish_job(job);	    /* may make some runnable */

    	if (runnable_jobs != 0)
	{
	    /* Start the first runnable job */
	    start_job((job_t *)runnable_jobs->data);
	}
	else
	{
	    /* Wait for something to finish to give us more runnables. */
	    finish_job((job_t *)queue_get(finish_queue));
	}
    }

    if (state_count[FAILED] > 0)
    {
#if DEBUG
	fprintf(stderr, "Main: waiting for jobs to finish\n");
#endif
	while (state_count[RUNNING] > 0)
	    finish_job((job_t *)queue_get(finish_queue));
#if DEBUG
	fprintf(stderr, "Main: all jobs finished\n");
#endif
    }

#if DEBUG
    fprintf(stderr, "Main: finishing\n");
#endif
    return (state_count[FAILED] == 0);
}

#endif /* !THREADS_NONE */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
scalar(void)
{
    job_t *job;
        
#if DEBUG
    fprintf(stderr, "scalar: starting\n");
#endif
    while ((state_count[UNKNOWN] > 0 ||
            state_count[RUNNABLE] > 0 ) &&
	   state_count[FAILED] == 0)
    {
    	assert(state_count[RUNNABLE] > 0);
	assert(runnable_jobs != 0);
	
	/* Perform the first runnable job */
	job = (job_t *)runnable_jobs->data;
#if DEBUG
    	fprintf(stderr, "scalar: starting job \"%s\"\n", job->name);
#endif    
	job_set_state(job, RUNNING);
	job->result = (*job->ops->execute)(job->userdata);
	job_set_state(job, (job->result ? UPTODATE : FAILED));
    }

#if DEBUG
    fprintf(stderr, "scalar: finishing\n");
#endif
    return (state_count[FAILED] == 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
job_immediate(job_ops_t *ops, void *userdata)
{
    gboolean result = FALSE;
    
    /* be a job barrier */
    if (job_run())
    {
	/* execute the command immediately */
	result = (*ops->execute)(userdata);
    }

    /* clean up immediately */
    if (ops->dtor != 0)
    	(*ops->dtor)(userdata);
    
    return result;
}

gboolean
job_immediate_command(
    strarray_t *command,
    strarray_t *env,
    logmsg_t *logmessage)
{
    gboolean result = FALSE;
    
    /* be a job barrier */
    if (job_run())
    {
	/* execute the command immediately */
	if (logmessage != 0)
	{
	    logmsg_emit(logmessage);
	    logmsg_delete(logmessage);
	}
	result = process_run(command, env, file_top_dir());
    }
    
    /* clean up immediately */
    delete command;
    if (env != 0)
	delete env;
    
    return result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
job_clear_one(gpointer key, gpointer value, gpointer userdata)
{
    job_delete((job_t *)value);
    return TRUE;    /* remove me */
}

void
job_clear(void)
{
    g_hash_table_foreach_remove(all_jobs, job_clear_one, 0);
    
    while (runnable_jobs != 0)
    	runnable_jobs = g_list_remove_link(runnable_jobs, runnable_jobs);

    assert(state_count[UNKNOWN] == 0);
    assert(state_count[RUNNABLE] == 0);
    assert(state_count[RUNNING] == 0);
    assert(state_count[FAILED] == 0);
    assert(state_count[UPTODATE] == 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
job_pending(void)
{
    return (g_hash_table_size(all_jobs) > 0);
}


gboolean
job_run(void)
{
    gboolean res = FALSE;

    if (!job_pending())
    	return TRUE;	    /* no jobs: trivially true */
	
    job_initialise_states();
#if DEBUG
    job_dump_all();
#endif

    if (state_count[FAILED] == 0)
    {
#if !THREADS_NONE
	if (num_workers > 1)
    	    res = main_thread();
	else
#endif	
	res = scalar();
    }
    
    job_clear();
    
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
job_init(unsigned int nw)
{
#if !THREADS_NONE
    num_workers = nw;
    if (num_workers > 1)
    {
	unsigned int i;
	cant_thread_t thr;

	start_queue = queue_new(1);
	finish_queue = queue_new(num_workers + /*len(start_queue)*/1 + /*paranoia*/1);

	/*
	 * TODO: start worker threads on demand, i.e. when there
	 *       are less than num_workers threads and put()ing
	 *       on the start queue would block.
	 */
	for (i = 0 ; i < num_workers ; i++)
	{
    	    if (cant_thread_create(&thr, worker_thread, (void*)(i+1)) < 0)
	    {
		perror("thread_create");
		return FALSE;
	    }
	}
    }
#endif

    all_jobs = g_hash_table_new(g_str_hash, g_str_equal);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
