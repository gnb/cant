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

#include "cant.h"

CVSID("$Id: xtask.c,v 1.5 2001-11-10 03:17:24 gnb Exp $");

typedef struct
{
    gboolean result;
    props_t *properties;    /* local properties, overriding the project */
} xtask_private_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
xtask_new(task_t *task)
{
    xtask_private_t *xp;
    
    task->private = xp = new(xtask_private_t);

    xp->properties = props_new(task->project->fixed_properties);    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
xtask_generic_setter(task_t *task, const char *name, const char *value)
{
    xtask_private_t *xp = (xtask_private_t *)task->private;
    xtask_ops_t *xops = (xtask_ops_t *)task->ops;
    const char *propname;
    
    if ((propname = props_get(xops->property_map, name)) == 0)
    	return FALSE;
    props_set(xp->properties, propname, value);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
xtask_append_file_estring(const char *filename, void *userdata)
{
    estring *ep = (estring *)userdata;
    
    if (ep->length > 0)
	estring_append_char(ep, ' ');

    /* TODO: escape whitespace in filename */
    estring_append_string(ep, filename);
    
    return TRUE;   /* keep going */
}

static gboolean
xtask_append_file_strarray(const char *filename, void *userdata)
{
    strarray_t *sa = (strarray_t *)userdata;
    
    strarray_append(sa, filename);
    
    return TRUE;   /* keep going */
}

static gboolean
xtask_execute_command(task_t *task)
{
    xtask_private_t *xp = (xtask_private_t *)task->private;
    xtask_ops_t *xops = (xtask_ops_t *)task->ops;
    strarray_t *command;
    GList *iter;
    char *exp;
    int status;
    
    if (xops->logmessage != 0)
    {
    	char *logexp = props_expand(xp->properties, xops->logmessage);
	logf("%s\n", logexp);
	g_free(logexp);
    }

    /* build the command from args and properties */
    command = strarray_new();
    
    if (xops->executable != 0)
    	strarray_appendm(command, props_expand(xp->properties, xops->executable));
    
    for (iter = xops->args ; iter != 0 ; iter = iter->next)
    {
    	xtask_arg_t *xa = (xtask_arg_t *)iter->data;
	
	if (xa->fileset != 0)
	{
	    /* <fileset> child */
	    fileset_apply(xa->fileset, xtask_append_file_strarray, command);
	}
	else
	{
	    /* <arg> child */
	    /* TODO: implement "if", "unless" conditions */
	    /* TODO: implement XT_WHITESPACE */
	    exp = props_expand(xp->properties, xa->arg);
	    
	    if (exp != 0 && *exp == '\0')
	    {
	    	g_free(exp);
		exp = 0;
	    }
	    
	    if (exp != 0)
	    {
	    	if (xa->flags & XT_WHITESPACE)
		    strarray_appendm(command, exp);
		else
		{
		    /* tokenise value on whitespace */
		    char *x, *buf2 = exp;
		    
		    while ((x = strtok(buf2, " \t\n\r")) != 0)
		    {
		    	buf2 = 0;
			strarray_append(command, x);
		    }
		    g_free(exp);
		}
	    }
	}
	/* TODO: <arg arglistref=""> child */
	/* TODO: <env> child */
    }
    
    /* execute the command */
#if DEBUG
    {
    	int i;
	fprintf(stderr, "xtask_execute_command:");
	for (i = 0 ; i < command->len ; i++)
	    fprintf(stderr, " \"%s\"", strarray_nth(command, i));
	fprintf(stderr, "\n");
    }
#endif

    if ((status = process_run(command, 0)) != 0)
    {
    	process_log_status(strarray_nth(command, 0), status);
	xp->result = FALSE;
    }
    
    strarray_delete(command);
    
    return (status == 0);
}

static gboolean
xtask_execute_one(const char *filename, void *userdata)
{
    task_t *task = (task_t *)userdata;
    xtask_private_t *xp = (xtask_private_t *)task->private;
    
    props_set(xp->properties, "file", filename);
    return xtask_execute_command(task);
}

static gboolean
xtask_execute(task_t *task)
{
    xtask_private_t *xp = (xtask_private_t *)task->private;
    xtask_ops_t *xops = (xtask_ops_t *)task->ops;
    
#if DEBUG
    fprintf(stderr, "xtask_execute: executing \"%s\"\n", task->name);
#endif

    /* default result */
    xp->result = TRUE;
    
    if (xops->task_ops.is_fileset)
    {
    	if (xops->foreach)
	{
	    /* run the command once for each file in the fileset */
	    fileset_apply(task->fileset, xtask_execute_one, task);
	}
	else
	{
    	    /* construct the "files" property */
    	    estring files;

    	    /* This is of course *NOT* whitespace-safe */
	    estring_init(&files);
	    fileset_apply(task->fileset, xtask_append_file_estring, &files);
	    props_setm(xp->properties, "files", files.data);

    	    /* run the command just once */
	    xtask_execute_command(task);
	}
    }
    else
    {
    	/* run the command just once */
	xtask_execute_command(task);
    }
    
    /* wipe out any temporary properties */
    props_setm(xp->properties, "file", 0);
    props_setm(xp->properties, "files", 0);
    
    return xp->result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
xtask_delete(task_t *task)
{
    xtask_private_t *xp = (xtask_private_t *)task->private;
    
#if DEBUG
    fprintf(stderr, "xtask_delete: deleting \"%s\"\n", task->name);
#endif

    props_delete(xp->properties);
	
    g_free(xp);

    task->private = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static xtask_arg_t *
xtask_arg_new(
    unsigned flags,
    const char *arg,
    fileset_t *fs)
{
    xtask_arg_t *xa;
    
    xa = new(xtask_arg_t);
    
    xa->flags = flags;
    strassign(xa->arg, arg);
    xa->fileset = fs;
    
    return xa;
}

static void
xtask_arg_delete(xtask_arg_t *xa)
{
    strdelete(xa->arg);
    
    if (xa->fileset != 0)
    {
    	fileset_delete(xa->fileset);
	xa->fileset = 0;
    }
    
    strdelete(xa->condition);
    
    g_free(xa);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
_xtask_arg_set_condition(xtask_arg_t *xa, unsigned flags, const char *prop)
{
    strassign(xa->condition, prop);
    xa->flags = (xa->flags & ~(XT_IFCOND|XT_UNLESSCOND)) | flags;
}


void
xtask_arg_set_if_condition(xtask_arg_t *xa, const char *prop)
{
    _xtask_arg_set_condition(xa, XT_IFCOND, prop);
}

void
xtask_arg_set_unless_condition(xtask_arg_t *xa, const char *prop)
{
    _xtask_arg_set_condition(xa, XT_UNLESSCOND, prop);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_ops_t *
xtask_ops_new(const char *name)
{
    xtask_ops_t *xops;

    xops = new(xtask_ops_t);
    
    strassign(xops->task_ops.name, name);

    xops->task_ops.init = 0;
    xops->task_ops.new = xtask_new;
    xops->task_ops.post_parse = 0;
    xops->task_ops.execute = xtask_execute;
    xops->task_ops.delete = xtask_delete;
    
    xops->property_map = props_new(0);

    return xops;
}

/* TODO: get this called!!!! */
void
xtask_ops_delete(xtask_ops_t *xops)
{
    while (xops->args != 0)
    {
    	xtask_arg_delete((xtask_arg_t *)xops->args->data);
    	xops->args = g_list_remove_link(xops->args, xops->args);
    }
    
    strdelete(xops->task_ops.name);
    strdelete(xops->executable);
    strdelete(xops->logmessage);

    props_delete(xops->property_map);

    g_free(xops);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

xtask_arg_t *
xtask_ops_add_line(xtask_ops_t *xops, const char *s)
{
    xtask_arg_t *xa;
    
    xa = xtask_arg_new(/*flags*/0, s, /*fileset*/0);
    
    xops->args = g_list_append(xops->args, xa);
    
    return xa;
}

xtask_arg_t *
xtask_ops_add_value(xtask_ops_t *xops, const char *s)
{
    xtask_arg_t *xa;
    
    xa = xtask_arg_new(XT_WHITESPACE, s, /*fileset*/0);
    
    xops->args = g_list_append(xops->args, xa);
    
    return xa;
}

xtask_arg_t *
xtask_ops_add_fileset(xtask_ops_t *xops, fileset_t *fs)
{
    xtask_arg_t *xa;
    
    xa = xtask_arg_new(/*flags*/0, /*arg*/0, fs);
    
    xops->args = g_list_append(xops->args, xa);
    
    return xa;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
xtask_ops_add_attribute(
    xtask_ops_t *xops,
    const char *attr,
    const char *prop,
    gboolean required)
{
    task_attr_t proto;
    
    proto.name = (char *)attr;
    proto.setter = xtask_generic_setter;
    proto.flags = (required ? TT_REQUIRED : 0);

    task_ops_add_attribute((task_ops_t *)xops, &proto);
    
    props_set(xops->property_map, attr, prop);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
