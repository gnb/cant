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

typedef struct
{
    char *message;
    char *file;
    gboolean append:1;
    gboolean newline:1;
} echo_private_t;

CVSID("$Id: task_echo.C,v 1.1 2002-03-29 12:36:27 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
echo_new(task_t *task)
{
    echo_private_t *ep;

    task->private_data = ep = new(echo_private_t);
    
    /* default values */
    ep->append = FALSE;
    ep->newline = TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
echo_set_message(task_t *task, const char *name, const char *value)
{
    echo_private_t *ep = (echo_private_t *)task->private_data;

    strassign(ep->message, value);
    return TRUE;
}

static gboolean
echo_set_file(task_t *task, const char *name, const char *value)
{
    echo_private_t *ep = (echo_private_t *)task->private_data;

    strassign(ep->file, value);
    return TRUE;
}
    
static gboolean
echo_set_append(task_t *task, const char *name, const char *value)
{
    echo_private_t *ep = (echo_private_t *)task->private_data;

    boolassign(ep->append, value);
    return TRUE;
}

static gboolean
echo_set_newline(task_t *task, const char *name, const char *value)
{
    echo_private_t *ep = (echo_private_t *)task->private_data;

    boolassign(ep->newline, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
echo_set_content(task_t *task, const char *content)
{
    echo_private_t *ep = (echo_private_t *)task->private_data;

    strassign(ep->message, content);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
echo_post_parse(task_t *task)
{
    echo_private_t *ep = (echo_private_t *)task->private_data;

    if (ep->message == 0)
    {
    	parse_error("Either \"message\" attribute or content must be set\n");
	return FALSE;
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
echo_execute(task_t *task)
{
    echo_private_t *ep = (echo_private_t *)task->private_data;
    char *expmsg;
    char *expfile;
    FILE *fp = stdout;

    expfile = task_expand(task, ep->file);
    strnullnorm(expfile);
    
    if (expfile != 0)
    {
    	if ((fp = fopen(expfile, (ep->append ? "a" : "w"))) == 0)
	{
	    log_perror(expfile);
	    return FALSE;
	}
	g_free(expfile);
    }

    expmsg = task_expand(task, ep->message);
    if (expmsg != 0)
    {
	fputs(expmsg, fp);
	if (ep->newline)
    	    fputc('\n', fp);
	g_free(expmsg);
    }

    if (fp != stdout)
    	fclose(fp);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
echo_delete(task_t *task)
{
    echo_private_t *ep = (echo_private_t *)task->private_data;
    
    strdelete(ep->message);
    strdelete(ep->file);

    task->private_data = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_attr_t echo_attrs[] = 
{
    TASK_ATTR(echo, message, 0),
    TASK_ATTR(echo, file, 0),
    TASK_ATTR(echo, append, 0),
    TASK_ATTR(echo, newline, 0),
    {0}
};

task_ops_t echo_ops = 
{
    "echo",
    /*init*/0,
    echo_new,
    echo_set_content,
    echo_post_parse,
    echo_execute,
    echo_delete,
    echo_attrs,
    /*children*/0,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
