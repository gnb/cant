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

CVSID("$Id: task_redirect.C,v 1.8 2002-04-13 12:30:42 gnb Exp $");

static const char tmpfile_proto[] = "/tmp/cant-redirect-propXXXXXX";


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class descriptor_var
{
private:
    int fd_;
    
public:
    descriptor_var()
    {
    	fd_ = -1;
    }
    ~descriptor_var()
    {
    	if (fd_ >= 0)
	{
	    close(fd_);
	    fd_ = -1;
	}
    }
    
    operator int() const
    {
    	return fd_;
    }
    
    descriptor_var &operator=(int fd)
    {
    	if (fd_ >= 0)
	    close(fd_);
	fd_ = fd;
	return *this;
    }
    
    int take()
    {
    	int d = fd_;
	fd_ = -1;
	return d;
    }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class filename_var
{
private:
    string_var filename_;
    
public:
    filename_var()
    {
    }
    ~filename_var()
    {
    	if (filename_.data() != 0)
	    unlink(filename_.data());
    }
    
    filename_var &operator=(const char *s)
    {
    	filename_ = s;
	return *this;
    }
    
    operator char *()
    {
    	return (char *)filename_.data();
    }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class redirect_task_t : public task_t
{
private:
    string_var output_file_;
    string_var output_property_;
    string_var input_file_;
    string_var input_property_;
    gboolean collapse_whitespace_:1;

public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

redirect_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
}

~redirect_task_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_output_file(const char *name, const char *value)
{
    output_file_ = value;
    return TRUE;
}

gboolean
set_output_property(const char *name, const char *value)
{
    output_property_ = value;
    return TRUE;
}

gboolean
set_input_file(const char *name, const char *value)
{
    input_file_ = value;
    return TRUE;
}

gboolean
set_input_property(const char *name, const char *value)
{
    input_property_ = value;
    return TRUE;
}

gboolean
set_collapse_whitespace(const char *name, const char *value)
{
    boolassign(collapse_whitespace_, value);
    return TRUE;
}

gboolean
post_parse()
{
    gboolean ret = TRUE;

    if (output_file_ != 0 && output_property_ != 0)
    {
	log::errorf("Cannot specify both \"output_file\" and \"output_property\"\n");
	ret = FALSE;
    }
    
    if (input_file_ != 0 && input_property_ != 0)
    {
	log::errorf("Cannot specify both \"input_file\" and \"input_property\"\n");
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


/*
 * I would use pipes for redirecting to or from properties,
 * except for the possibility of deadlocking when trying to
 * pass more data than the pipe's kernel buffer can hold.
 */
 
gboolean
exec()
{
    gboolean ret = TRUE;
    descriptor_var old_stdin, old_stdout;
    descriptor_var new_stdin, new_stdout;
    filename_var new_stdin_tmp, new_stdout_tmp;
    string_var exp;
    string_var output_property;
    
    /*
     * Redirect output to a file.
     *
     * TODO: append to the file.
     */
    exp = expand(output_file_);
    if (!exp.is_null())
    {
    	/* TODO: provide an attribute to control the mode */
	exp = file_normalise(exp, 0);
    	new_stdout = open(exp, O_RDWR|O_CREAT|O_TRUNC, 0600);
	if (new_stdout < 0)
	{
	    log::perror(exp);
	    return FALSE;
	}
    }

    /*
     * Redirect output to a property.
     */
    exp = expand(output_property_);
    if (!exp.is_null())
    {
	new_stdout_tmp = tmpfile_proto;
	new_stdout = mkstemp(new_stdout_tmp);
	if (new_stdout < 0)
	{
	    log::perror(exp);
	    return FALSE;
	}
	output_property = exp.take();
    }

    /*
     * Redirect input from a file.
     */
    exp = expand(input_file_);
    if (!exp.is_null())
    {
    	exp = file_normalise(exp, 0);
    	new_stdin = open(exp, O_RDONLY, 0);
	if (new_stdin < 0)
	{
	    log::perror(exp);
	    return FALSE;
	}
    }

    /*
     * Redirect input from a property.
     */
    exp = expand(input_property_);
    if (!exp.is_null())
    {
	const char *val;
	
	new_stdin_tmp = tmpfile_proto;
	new_stdin = mkstemp(new_stdin_tmp);
	if (new_stdin < 0)
	{
	    log::perror(new_stdin_tmp);
	    return FALSE;
	}

    	val = project_->get_property(exp);
	write(new_stdin, val, strlen(val));
	lseek(new_stdin, 0, SEEK_SET);
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
    if (!execute_subtasks())
	ret = FALSE;

    /*
     * Switch the file descriptors back.
     */
    if (old_stdin >= 0)
    	dup2(old_stdin, FILENO_STDIN);
    if (old_stdout >= 0)
    {
    	fflush(stdout);
    	dup2(old_stdout, FILENO_STDOUT);
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

	lseek(new_stdout, 0, SEEK_SET);
	fp = fdopen(new_stdout.take(), "r");
	while ((c = fgetc(fp)) != EOF)
	{
	    if (collapse_whitespace_)
	    {
		if (inws)
		{
	    	    if (!isspace(c))
		    {
			inws = FALSE;
			if (buf.length() > 0)
			    buf.append_char(' ');
			buf.append_char(c);
		    }
		}
		else
		{
	    	    if (isspace(c))
			inws = TRUE;
		    else
		    	buf.append_char(c);
		}
	    }
	    else
	    {
	    	buf.append_char(c);
	    }
	}
	fclose(fp);
	project_->set_propertym(output_property, buf.take());
    }
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

static task_attr_t redirect_attrs[] = 
{
    TASK_ATTR(redirect, output_file, 0),
    TASK_ATTR(redirect, output_property, 0),
    TASK_ATTR(redirect, input_file, 0),
    TASK_ATTR(redirect, input_property, 0),
    TASK_ATTR(redirect, collapse_whitespace, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(redirect,
			redirect_attrs,
			/*children*/0,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/TRUE)
TASK_DEFINE_CLASS_END(redirect)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
