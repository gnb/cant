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

#include "cant.H"

CVSID("$Id: runner_simple.C,v 1.1 2002-04-21 04:01:40 gnb Exp $");

class runner_simple_t : public runner_t
{
public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

runner_simple_t()
{
}

~runner_simple_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Runs the given command with the given environment
 * overrides, waits until it finishes, and returns
 * TRUE iff the process ran and reported no errors
 * in its exit status.
 */

gboolean
run()
{
    pid_t pid;
    int i;
    
    pid = fork();
    
    if (pid < 0)
    {
    	/* error */
    	log::perror("fork");
	return FALSE;
    }
    else if (pid == 0)
    {
    	/* child */
    	if (directory_ != 0 && chdir(directory_) < 0)
	{
	    perror(directory_);
	    exit(1);
	}

	if (environment_ != 0)
	    for (i = 0 ; i < environment_->len ; i++)
		putenv((char *)environment_->nth(i));
	    
	execvp(command_->nth(0), (char * const *)command_->data());
	perror(command_->nth(0));
	exit(1);
    }
    else
    {
    	/* parent */
	return interpret_status(vulture(pid));
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

RUNNER_DEFINE_CLASS(simple);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
