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

CVSID("$Id: task_echo.C,v 1.3 2002-04-12 13:07:24 gnb Exp $");

class echo_task_t : public task_t
{
private:
    char *message_;
    char *file_;
    gboolean append_:1;
    gboolean newline_:1;
    
public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

echo_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
    /* default values */
    append_ = FALSE;
    newline_ = TRUE;
}

~echo_task_t()
{
    strdelete(message_);
    strdelete(file_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_message(const char *name, const char *value)
{
    strassign(message_, value);
    return TRUE;
}

gboolean
set_file(const char *name, const char *value)
{
    strassign(file_, value);
    return TRUE;
}
    
gboolean
set_append(const char *name, const char *value)
{
    boolassign(append_, value);
    return TRUE;
}

gboolean
set_newline(const char *name, const char *value)
{
    boolassign(newline_, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_content(const char *content)
{
    strassign(message_, content);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
post_parse()
{
    if (message_ == 0)
    {
    	log::errorf("Either \"message\" attribute or content must be set\n");
	return FALSE;
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
exec()
{
    char *expmsg;
    char *expfile;
    FILE *fp = stdout;

    expfile = expand(file_);
    strnullnorm(expfile);
    
    if (expfile != 0)
    {
    	if ((fp = fopen(expfile, (append_ ? "a" : "w"))) == 0)
	{
	    log::perror(expfile);
	    return FALSE;
	}
	g_free(expfile);
    }

    expmsg = expand(message_);
    if (expmsg != 0)
    {
	fputs(expmsg, fp);
	if (newline_)
    	    fputc('\n', fp);
	g_free(expmsg);
    }

    if (fp != stdout)
    	fclose(fp);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

static task_attr_t echo_attrs[] = 
{
    TASK_ATTR(echo, message, 0),
    TASK_ATTR(echo, file, 0),
    TASK_ATTR(echo, append, 0),
    TASK_ATTR(echo, newline, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(echo,
			echo_attrs,
			/*children*/0,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/FALSE)
TASK_DEFINE_CLASS_END(echo)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
