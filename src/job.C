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
#include "hashtable.H"

#if !THREADS_NONE
#include "thread.H"
#include "queue.H"
#endif

CVSID("$Id: job.C,v 1.5 2002-03-29 17:57:11 gnb Exp $");


extern int process_run(strarray_t *command, strarray_t *env, const char *dir);

static hashtable_t<const char*, job_t> *all_jobs;
static GList *runnable_jobs;
int job_t::state_count_[job_t::NUM_STATES];

#if !THREADS_NONE
/*
 * Start queue is used to farm out jobs to worker threads.
 * It is at most one job long, to allow the main thread to
 * make decisions about downstream jobs at the latest
 * possible time.  Both put and get operations block.
 */
static queue_t<job_t> *start_queue;

/*
 * Finish queue is used to return finished jobs and their
 * associated results from worker threads back to the main
 * thread for incorporation in global data.  It's length
 * is bounded by num_workers + length(start_queue).  The
 * put operation should never block; both blocking and non-
 * blocking get operations are used at various times.
 */
static queue_t<job_t> *finish_queue;

static unsigned int num_workers;
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

job_op_t::job_op_t()
{
}

job_op_t::~job_op_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

command_job_op_t::command_job_op_t(
    strarray_t *command,
    strarray_t *env,
    logmsg_t *logmsg)
{
    command_ = command;
    env_ = env;
    logmessage_ = logmsg;
    directory_ = g_strdup(file_top_dir());
}

command_job_op_t::~command_job_op_t()
{
    if (command_ != 0)
	delete command_;
    if (env_ != 0)
	delete env_;
    strdelete(directory_);
	
    if (logmessage_ != 0)
	logmsg_delete(logmessage_);
}

gboolean
command_job_op_t::execute()
{
    if (logmessage_ != 0)
    	logmsg_emit(logmessage_);
    return process_run(command_, env_, directory_);
}

char *
command_job_op_t::describe() const
{
    return command_->join(" ");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

job_t::job_t(const char *name)
{
    strassign(name_, name);
    state_ = UNKNOWN;
    state_count_[UNKNOWN]++;
    
    all_jobs->insert(name_, this);
}

job_t::~job_t()
{
    if (op_ != 0)
    	delete op_;

    strdelete(name_);
    state_count_[state_]--;
    
    listclear(depends_up_);
    listclear(depends_down_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if DEBUG

const char *
job_t::state_name(job_t::state_t state)
{
    static const char *names[] = 
    {
    	"UNKNOWN",
	"RUNNABLE",
	"RUNNING",
	"UPTODATE",
	"FAILED"
    };
    return names[state];
}

char *
job_t::describe() const
{
    if (job->op_ == 0)
    	return g_strdup("-undefined-");
    return job->op_->describe();
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

job_t *
job_t::add(const char *name, job_op_t *op)
{
    job_t *job;
    static unsigned int serial = 0;

    if ((job = all_jobs->lookup(name)) != 0)
    {
    	if (job->op_ != 0)
	{
	    fprintf(stderr, "Duplicate job \"%s\"\n", name);
	    return 0;
	}
    }
    else
    {
	job = new job_t(name);
    }
	
    job->serial_ = ++serial;
    job->op_ = op;
    
#if DEBUG
    {
    	char *str = job->describe();
    	fprintf(stderr, "job_add: serial=%u name=\"%s\" description=\"%s\"\n",
	    	    job->serial_, job->name_, str);
	g_free(str);
    }
#endif
    
    return job;
}


void
job_t::add_depend(const char *depname)
{
    job_t *dep;
    
    if ((dep = all_jobs->lookup(depname)) == 0)
    {
    	/* create an undefined job for later definition */
	dep = new job_t(depname);
    }
    
    /* setup depends links */
    depends_down_ = g_list_append(depends_down_, dep);
    dep->depends_up_ = g_list_append(dep->depends_up_, this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

job_t::state_t
job_t::calc_new_state() const
{
    GList *iter;
    
    for (iter = depends_down_ ; iter != 0 ; iter = iter->next)
    {
    	job_t *down = (job_t *)iter->data;
	
	if (down->state_ == FAILED)
	    return FAILED;
	if (down->state_ != UPTODATE)
	    return UNKNOWN;
    }

    /* all deps are UPTODATE */
    
    if (file_exists(name_) == 0)
    	return UPTODATE;
    
    return RUNNABLE;
}

int
job_t::compare_by_serial(gconstpointer c1, gconstpointer c2)
{
    const job_t *j1 = (const job_t*)c1;
    const job_t *j2 = (const job_t*)c2;
    
    if (j1->serial_ > j2->serial_)
    	return 1;
    if (j1->serial_ < j2->serial_)
    	return -1;
    return 0;
}

void
job_t::set_state(job_t::state_t newstate)
{
    GList *iter;

    if (state_ == newstate)
    	return;     /* nothing to see here, move along */
	
    /* runnable_jobs keeps track of all RUNNABLE jobs in serial order */
    if (newstate == RUNNABLE)
	runnable_jobs = g_list_insert_sorted(runnable_jobs, this,
	    	    	    	    	     compare_by_serial);
    if (state_ == RUNNABLE)
	runnable_jobs = g_list_remove(runnable_jobs, this);

    /* keep track of how many jobs are in each state */
    state_count_[newstate]++;
    state_count_[state_]--;

    /* actually update the state variable */
    state_ = newstate;
    
#if DEBUG
    fprintf(stderr, "Main: job \"%s\" becomes %s\n",
    	    	    name_, state_name(state_));
#endif

    /* propagate the state change up the dependency graph */
    switch (newstate)
    {
    case FAILED:
    case UPTODATE:
    case UNKNOWN:
	for (iter = depends_up_ ; iter != 0 ; iter = iter->next)
	{
    	    job_t *up = (job_t *)iter->data;

	    up->set_state(up->calc_new_state());
	}
	break;
    default:
    	break;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
job_t::initialise_one(const char *key, job_t *job, void *userdata)
{
    if (job->depends_down_ == 0)
    {
    	if (job->op_ == 0 && file_exists(job->name_) < 0)
	{
	    fprintf(stderr, "No rule to make \"%s\"\n", job->name_);
	    job->set_state(FAILED);
	}
	else
	    job->set_state(UPTODATE);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG

static void
job_t::dump() const
{
    char *desc;
    GList *iter;
    
    desc = describe(job);
    fprintf(stderr, "    job 0x%08lx {\n\tserial = %u\n\tname = \"%s\"\n\tstate = %s\n\tdescription = \"%s\"\n\tdepends_down =",
    	       (unsigned long)this,
	       serial_,
	       name_,
	       job_state_name(job),
	       desc);
    for (iter = depends_down_ ; iter != 0 ; iter = iter->next)
    {
    	job_t *down = (job_t *)iter->data;
	
	fprintf(stderr, " \"%s\"", down->name_);
    }
    fprintf(stderr, "\n    }\n");
    g_free(desc);
}

static void
job_dump_one(const char *key, job_t *job, void *userdata)
{
    job->dump();
}

void
job_t::dump_all()
{
    fprintf(stderr, "all_jobs = \n");
    all_jobs->foreach(job_dump_one, 0);
    fflush(stderr);
}

#endif /* DEBUG */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if !THREADS_NONE

void *
job_t::worker_thread(void *arg)
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
    	job = start_queue->get();

    	/* run the job */
#if DEBUG
	fprintf(stderr, "Worker%d: starting job \"%s\"\n", threadno, job->name_);
#endif
	job->result_ = job->op_->execute();
#if DEBUG
	fprintf(stderr, "Worker%d: finished job \"%s\"\n", threadno, job->name_);
#endif
	
	/* put the finished job on the queue back to the main thread */
	finish_queue->put(job);
    }
}

void
job_t::finish_job(job_t *job)
{
#if DEBUG
    fprintf(stderr, "Main: received finished job \"%s\", %s\n",
    	    	job->name,
		(job->result ? "success" : "failure"));
#endif
    job->set_state((job->result_ ? UPTODATE : FAILED));
}

void
job_t::start_job(job_t *job)
{
    job->set_state(RUNNING);
    start_queue->put(job);
}

gboolean
job_t::main_thread()
{
    job_t *job;
        
#if DEBUG
    fprintf(stderr, "Main: starting\n");
#endif
    while ((state_count_[UNKNOWN] > 0 ||
            state_count_[RUNNABLE] > 0 ||
            state_count_[RUNNING] > 0 ) &&
	   state_count_[FAILED] == 0)
    {
    	/* Handle any finished jobs, to keep the finish queue short */
	while ((job = finish_queue->tryget()) != 0)
	    finish_job(job);	    /* may make some runnable */

    	if (runnable_jobs != 0)
	{
	    /* Start the first runnable job */
	    start_job((job_t *)runnable_jobs->data);
	}
	else
	{
	    /* Wait for something to finish to give us more runnables. */
	    finish_job(finish_queue->get());
	}
    }

    if (state_count_[FAILED] > 0)
    {
#if DEBUG
	fprintf(stderr, "Main: waiting for jobs to finish\n");
#endif
	while (state_count_[RUNNING] > 0)
	    finish_job(finish_queue->get());
#if DEBUG
	fprintf(stderr, "Main: all jobs finished\n");
#endif
    }

#if DEBUG
    fprintf(stderr, "Main: finishing\n");
#endif
    return (state_count_[FAILED] == 0);
}

#endif /* !THREADS_NONE */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
job_t::scalar()
{
    job_t *job;
        
#if DEBUG
    fprintf(stderr, "scalar: starting\n");
#endif
    while ((state_count_[UNKNOWN] > 0 ||
            state_count_[RUNNABLE] > 0 ) &&
	   state_count_[FAILED] == 0)
    {
    	assert(state_count_[RUNNABLE] > 0);
	assert(runnable_jobs != 0);
	
	/* Perform the first runnable job */
	job = (job_t *)runnable_jobs->data;
#if DEBUG
    	fprintf(stderr, "scalar: starting job \"%s\"\n", job->name_);
#endif    
	job->set_state(RUNNING);
	job->result_ = job->op_->execute();
	job->set_state((job->result_ ? UPTODATE : FAILED));
    }

#if DEBUG
    fprintf(stderr, "scalar: finishing\n");
#endif
    return (state_count_[FAILED] == 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
job_t::immediate(job_op_t *op)
{
    gboolean result = FALSE;
    
    /* be a job barrier */
    if (run())
    {
	/* execute the command immediately */
	result = op->execute();
    }

    /* clean up immediately */
    delete op;
    
    return result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
job_t::clear_one(const char *key, job_t *value, void *userdata)
{
    delete value;
    return TRUE;    /* remove me */
}

void
job_t::clear()
{
    all_jobs->foreach_remove(clear_one, 0);
    
    listclear(runnable_jobs);

    assert(state_count_[UNKNOWN] == 0);
    assert(state_count_[RUNNABLE] == 0);
    assert(state_count_[RUNNING] == 0);
    assert(state_count_[FAILED] == 0);
    assert(state_count_[UPTODATE] == 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
job_t::pending()
{
    return (all_jobs->size() > 0);
}


gboolean
job_t::run()
{
    gboolean res = FALSE;

    if (!pending())
    	return TRUE;	    /* no jobs: trivially true */
	
    all_jobs->foreach(initialise_one, 0);
#if DEBUG
    dump_all();
#endif

    if (state_count_[FAILED] == 0)
    {
#if !THREADS_NONE
	if (num_workers > 1)
    	    res = main_thread();
	else
#endif	
	res = scalar();
    }
    
    clear();
    
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
job_t::init(unsigned int nw)
{
#if !THREADS_NONE
    num_workers = nw;
    if (num_workers > 1)
    {
	unsigned int i;
	cant_thread_t thr;

	start_queue = new queue_t<job_t>(1);
	finish_queue = new queue_t<job_t>(num_workers + /*len(start_queue)*/1 + /*paranoia*/1);

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

    all_jobs = new hashtable_t<const char *, job_t>;

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
