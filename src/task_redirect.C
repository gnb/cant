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
#include <fcntl.h>

CVSID("$Id: task_redirect.C,v 1.1 2002-03-29 12:36:27 gnb Exp $");

typedef struct
{
    char *output_file;
    char *output_property;
    char *input_file;
    char *input_property;
    gboolean collapse_whitespace:1;
} redirect_private_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
redirect_new(task_t *task)
{
    task->private_data = new(redirect_private_t);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
redirect_set_output_file(task_t *task, const char *name, const char *value)
{
    redirect_private_t *rp = (redirect_private_t *)task->private_data;
    
    strassign(rp->output_file, value);
    return TRUE;
}

static gboolean
redirect_set_output_property(task_t *task, const char *name, const char *value)
{
    redirect_private_t *rp = (redirect_private_t *)task->private_data;

    strassign(rp->output_property, value);
    return TRUE;
}

static gboolean
redirect_set_input_file(task_t *task, const char *name, const char *value)
{
    redirect_private_t *rp = (redirect_private_t *)task->private_data;
    
    strassign(rp->input_file, value);
    return TRUE;
}

static gboolean
redirect_set_input_property(task_t *task, const char *name, const char *value)
{
    redirect_private_t *rp = (redirect_private_t *)task->private_data;

    strassign(rp->input_property, value);
    return TRUE;
}

static gboolean
redirect_set_collapse_whitespace(task_t *task, const char *name, const char *value)
{
    redirect_private_t *rp = (redirect_private_t *)task->private_data;

    boolassign(rp->collapse_whitespace, value);
    return TRUE;
}

static gboolean
redirect_post_parse(task_t *task)
{
    redirect_private_t *rp = (redirect_private_t *)task->private_data;
    gboolean ret = TRUE;

    if (rp->output_file != 0 && rp->output_property != 0)
    {
	parse_error("Cannot specify both \"output_file\" and \"output_property\"\n");
	ret = FALSE;
    }
    
    if (rp->input_file != 0 && rp->input_property != 0)
    {
	parse_error("Cannot specify both \"input_file\" and \"input_property\"\n");
	ret = FALSE;
    }
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifndef FILENO_STDIN
#define FILENO_STDIN 0
#endif
#ifndef FILENO_STDOUT
#define FILENO_STDOUT 1
#endif

static const char tmpfile_proto[] = "/tmp/cant-redirect-propXXXXXX";

/*
 * I would use pipes for redirecting to or from properties,
 * except for the possibility of deadlocking when trying to
 * pass more data than the pipe's kernel buffer can hold.
 */
 
static gboolean
redirect_execute(task_t *task)
{
    redirect_private_t *rp = (redirect_private_t *)task->private_data;
    GList *iter;
    gboolean ret = TRUE;
    int old_stdin = -1, old_stdout = -1;
    int new_stdin = -1, new_stdout = -1;
    char *new_stdin_tmp = 0, *new_stdout_tmp = 0;
    char *exp;
    char *output_property = 0;
    
    /*
     * Redirect output to a file.
     *
     * TODO: append to the file.
     */
    exp = task_expand(task, rp->output_file);
    strnullnorm(exp);
    if (exp != 0)
    {
    	/* TODO: provide an attribute to control the mode */
	exp = file_normalise_m(exp, 0);
    	new_stdout = open(exp, O_RDWR|O_CREAT|O_TRUNC, 0600);
	if (new_stdout < 0)
	{
	    log_perror(exp);
	    ret = FALSE;
	    goto cleanups;
	}
	g_free(exp);
    }

    /*
     * Redirect output to a property.
     */
    exp = task_expand(task, rp->output_property);
    strnullnorm(exp);
    if (exp != 0)
    {
	new_stdout_tmp = g_strdup(tmpfile_proto);
	new_stdout = mkstemp(new_stdout_tmp);
	if (new_stdout < 0)
	{
	    log_perror(exp);
	    ret = FALSE;
	    goto cleanups;
	}
	output_property = exp;
    }

    /*
     * Redirect input from a file.
     */
    exp = task_expand(task, rp->input_file);
    strnullnorm(exp);
    if (exp != 0)
    {
    	exp = file_normalise_m(exp, 0);
    	new_stdin = open(exp, O_RDONLY, 0);
	if (new_stdin < 0)
	{
	    log_perror(exp);
	    ret = FALSE;
	    goto cleanups;
	}
	g_free(exp);
    }

    /*
     * Redirect input from a property.
     */
    exp = task_expand(task, rp->input_property);
    strnullnorm(exp);
    if (exp != 0)
    {
	const char *val;
	
	new_stdin_tmp = g_strdup(tmpfile_proto);
	new_stdin = mkstemp(new_stdin_tmp);
	if (new_stdin < 0)
	{
	    log_perror(new_stdin_tmp);
	    ret = FALSE;
	    goto cleanups;
	}

    	val = props_get(project_get_props(task->project), exp);
	write(new_stdin, val, strlen(val));
	lseek(new_stdin, 0, SEEK_SET);
	
	g_free(exp);
    }

    /*
     * Actually switch the file descriptors.
     */
    if (new_stdin != 0)
    {
    	old_stdin = dup(FILENO_STDIN);
	dup2(new_stdin, FILENO_STDIN);
    }
    if (new_stdout != 0)
    {
    	fflush(stdout);
    	old_stdout = dup(FILENO_STDOUT);
	dup2(new_stdout, FILENO_STDOUT);
    }

    /*
     * Execute the subtasks
     *
     * TODO: need to be a job barrier because the job code
     * isn't and shouldn't be clever enough to stash away
     * file descriptors.
     */
    if (!task_execute_subtasks(task))
	ret = FALSE;

    /*
     * Switch the file descriptors back.
     */
    if (old_stdin >= 0)
    {
    	dup2(old_stdin, FILENO_STDIN);
	close(old_stdin);
    }
    if (old_stdout >= 0)
    {
    	fflush(stdout);
    	dup2(old_stdout, FILENO_STDOUT);
	close(old_stdout);
    }
    
    /*
     * Gather the result of the output property
     */
    if (output_property != 0)
    {
    	gboolean inws = FALSE;
    	int c;
	estring buf;
	FILE *fp;

    	estring_init(&buf);
	lseek(new_stdout, 0, SEEK_SET);
	fp = fdopen(new_stdout, "r");
	while ((c = fgetc(fp)) != EOF)
	{
	    if (rp->collapse_whitespace)
	    {
		if (inws)
		{
	    	    if (!isspace(c))
		    {
			inws = FALSE;
			if (buf.length > 0)
			    estring_append_char(&buf, ' ');
			estring_append_char(&buf, c);
		    }
		}
		else
		{
	    	    if (isspace(c))
			inws = TRUE;
		    else
		    	estring_append_char(&buf, c);
		}
	    }
	    else
	    {
	    	estring_append_char(&buf, c);
	    }
	}
	fclose(fp);
	new_stdout = -1;
	props_setm(task->project->properties, output_property, buf.data);
    }

    /*
     * Close the new files or pipes and unlink tmp files
     */
cleanups:    
    if (new_stdin >= 0)
	close(new_stdin);
    
    if (new_stdin_tmp != 0)
    {
    	unlink(new_stdin_tmp);
    	g_free(new_stdin_tmp);
    }

    if (new_stdout >= 0)
	close(new_stdout);
	
    if (new_stdout_tmp != 0)
    {
    	unlink(new_stdout_tmp);
    	g_free(new_stdout_tmp);
    }

    if (output_property != 0)
    	g_free(output_property);

    if (exp != 0)
    	g_free(exp);

    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
redirect_delete(task_t *task)
{
    redirect_private_t *rp = (redirect_private_t *)task->private_data;
    
    strdelete(rp->output_file);
    strdelete(rp->output_property);
    strdelete(rp->input_file);
    strdelete(rp->input_property);

    g_free(rp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_attr_t redirect_attrs[] = 
{
    TASK_ATTR(redirect, output_file, 0),
    TASK_ATTR(redirect, output_property, 0),
    TASK_ATTR(redirect, input_file, 0),
    TASK_ATTR(redirect, input_property, 0),
    TASK_ATTR(redirect, collapse_whitespace, 0),
    {0}
};

task_ops_t redirect_ops = 
{
    "redirect",
    /*init*/0,
    redirect_new,
    /*set_content*/0,
    redirect_post_parse,
    redirect_execute,
    redirect_delete,
    redirect_attrs,
    /*children*/0,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0,
    /*cleanup*/0,
    /*is_composite*/TRUE
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/