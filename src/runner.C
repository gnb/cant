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

#include "runner.H"
#include "log.H"
#include "filename.H"

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

CVSID("$Id: runner.C,v 1.1 2002-04-21 04:01:40 gnb Exp $");

hashtable_t<char*, runner_creator_t> *runner_t::creators;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

runner_t::runner_t()
{
}

runner_t::~runner_t()
{
    if (command_ != 0)
	delete command_;
    if (environment_ != 0)
	delete environment_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
runner_t::init()
{
    directory_ = file_top_dir();
    return TRUE;
}

void
runner_t::set_command(strarray_t *cmd)
{
    command_ = cmd;
}

void
runner_t::set_environment(strarray_t *env)
{
    environment_ = env;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

runner_t *
runner_t::create(const char *name)
{
    runner_t *ru;
    runner_creator_t *creator;
    
    if (creators == 0 ||
    	(creator = creators->lookup((char*)name)) == 0)
    {
    	log::errorf("Unknown runner type \"%s\"\n", name);
    	return 0;
    }

    ru =  (*creator)();
    
    if (!ru->init())
    {
    	delete ru;
	return 0;
    }
    
    return ru;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
runner_t::setup_properties(props_t *props) const
{
}

char *
runner_t::describe() const
{
    return command_->join(" ");
}

strarray_t *
runner_t::extracted_dependencies() const
{
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
runner_t::add_creator(const char *name, runner_creator_t *creator)
{
    if (creators == 0)
    	creators = new hashtable_t<char*, runner_creator_t>;
    else if (creators->lookup((char *)name) != 0)
    {
    	log::errorf("runner class \"%s\" already registered, ignoring new definition\n",
	    	name);
    	return FALSE;
    }
    
    creators->insert(g_strdup(name), creator);
#if DEBUG
    fprintf(stderr, "runner_t::add_creator: adding \"%s\" -> %08lx\n",
    	name, (unsigned long)creator);
#endif
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Waits for a process to die and reaps it, returning the
 * exit status (if the program exited normally), >256
 * (if the program died via a signal) or -1 (system
 * call failed for various reasons).  The return value
 * is interpreted and a log message possibly emitted in
 * interpret_result().
 */

#define SIGNAL_FLAG 	0x100

int
runner_t::vulture(pid_t pid)
{
    int status;
    
    for (;;)
    {
    	status = 0;
	if (waitpid(pid, &status, 0) < 0)
	{
#ifdef ERESTARTSYS
	    if (errno == ERESTARTSYS)
		continue;
#endif
	    if (errno == EINTR)
		continue;
	    return -1;
	}

	if (WIFEXITED(status))
	    return WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
	    return (SIGNAL_FLAG|WTERMSIG(status));
	/* WIFSTOPPED() -- continue */
    }
    /* UNREACHED */
    return -1;
}

gboolean
runner_t::interpret_status(int status)
{
    if (status < 0)
    {
	log::perror("waitpid");
	return FALSE;
    }
    else if (status & SIGNAL_FLAG)
    {
	status &= ~SIGNAL_FLAG;
    	log::errorf("Command \"%s\" was terminated by signal %d (%s)\n",
	    	command_->nth(0),
		status,
		g_strsignal(status));
	return FALSE;
    }
    else if (status > 0)
    {
    	log::errorf("Command \"%s\" exited with status %d\n",
	    	command_->nth(0),
		status);
	return FALSE;
    }
    /* == 0 -> ok */
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define RUNNER_CLASS(nm)  	extern runner_creator_t runner_##nm##_create;
#include "builtin-runners.H"
#undef RUNNER_CLASS

void
runner_t::initialise_builtins()
{
#define RUNNER_CLASS(nm)  runner_t::add_creator(g_string(nm), &runner_##nm##_create);
#include "builtin-runners.H"
#undef RUNNER_CLASS
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
