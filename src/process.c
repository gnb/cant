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

#include "cant.h"

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

CVSID("$Id: process.c,v 1.3 2001-11-13 04:08:05 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Waits for a process to die and reaps it, returning the
 * exit status (if the program exited normally), >256
 * (if the program died via a signal) or -1 (system
 * call failed for various reasons).
 */

#define SIGNAL_FLAG 	0x100

static int
vulture(pid_t pid)
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

/*
 * Runs the given command with the given environment
 * overrides, waits until it finishes, and returns
 * TRUE iff the process ran and reported no errors
 * in its exit status.
 */
  
gboolean
process_run(strarray_t *command, strarray_t *env)
{
    pid_t pid;
    int status;
    int i;
    
    /* TODO: handle input, output files */

    pid = fork();
    
    if (pid < 0)
    {
    	/* error */
    	logperror("fork");
	return FALSE;
    }
    
    if (pid == 0)
    {
    	/* child */
	if (env != 0)
	    for (i = 0 ; i < env->len ; i++)
		putenv((char *)strarray_nth(env, i));
	    
	execvp(strarray_nth(command, 0), (char * const *)strarray_data(command));
	perror(strarray_nth(command, 0));
	exit(1);
    }
    else
    {
    	/* parent */
	status = vulture(pid);
	
	if (status < 0)
	{
	    logperror("waitpid");
	    return FALSE;
	}
	else if (status & SIGNAL_FLAG)
	{
	    status &= ~SIGNAL_FLAG;
    	    logf("Command \"%s\" was terminated by signal %d (%s)\n",
	    	    strarray_nth(command, 0),
		    status,
		    g_strsignal(status));
	    return FALSE;
	}
	else if (status > 0)
	{
    	    logf("Command \"%s\" exited with status %d\n",
	    	    strarray_nth(command, 0),
		    status);
	    return FALSE;
	}
	/* == 0 -> ok */
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
