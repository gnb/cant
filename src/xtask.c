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

#include "xtask.h"
#include "job.h"

CVSID("$Id: xtask.c,v 1.12 2001-11-16 05:31:45 gnb Exp $");

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

    xp->properties = props_new(project_get_props(task->project));
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

/* TODO: build env too */
static gboolean
xtask_build_command(task_t *task, strarray_t *command)
{
    xtask_private_t *xp = (xtask_private_t *)task->private;
    xtask_ops_t *xops = (xtask_ops_t *)task->ops;
    GList *iter;
    char *exp;

    if (xops->executable != 0)
    	strarray_appendm(command, props_expand(xp->properties, xops->executable));
    
    for (iter = xops->args ; iter != 0 ; iter = iter->next)
    {
    	xtask_arg_t *xa = (xtask_arg_t *)iter->data;

    	if (!condition_evaluate(&xa->condition, xp->properties))
	    continue;
	
    	switch (xa->type)
	{
	case XT_VALUE:	    /* <arg value=""> child */
	    /* TODO: <arg arglistref=""> child */
	    exp = props_expand(xp->properties, xa->data.arg);
	    strnullnorm(exp);
	    if (exp != 0)
		strarray_appendm(command, exp);
	    break;

	case XT_LINE:	    /* <arg line=""> child */
	    exp = props_expand(xp->properties, xa->data.arg);
	    strnullnorm(exp);
	    if (exp != 0)
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
	    break;
	    /* TODO: <env> child */
	
	case XT_FILESET:    /* <fileset> child */
    	    fileset_gather_mapped(xa->data.fileset, xp->properties,
	    	    	    	  command, /*mappers*/0);
	    break;

	case XT_FILES:	    /* <files> child */
	    if (task->fileset != 0)
    	    	fileset_gather_mapped(task->fileset, xp->properties,
		    	    	      command, xops->mappers);
	    break;
	}
    }
    
#if DEBUG
    {
    	char *commstr = strarray_join(command, "\" \"");
	fprintf(stderr, "xtask_build_command: \"%s\"\n", commstr);
	g_free(commstr);
    }
#endif

    return TRUE;
}



static gboolean
xtask_execute_command(task_t *task)
{
    xtask_private_t *xp = (xtask_private_t *)task->private;
    xtask_ops_t *xops = (xtask_ops_t *)task->ops;
    logmsg_t *logmsg = 0;
    strarray_t *command;
    strarray_t *depfiles;
    char *targfile = 0;
    
    /* build the command from args and properties */
    command = strarray_new();
    
    if (!xtask_build_command(task, command))
    {
    	strarray_delete(command);
	return FALSE;
    }
    
       
    /* try to build dependency information for the command */

    depfiles = strarray_new();

    if (xops->task_ops.is_fileset)
    {
	if (xops->foreach)
	{
	    GList *iter;
	    char *depfile;

	    depfile = props_expand(xp->properties, "${file}");
	    strarray_appendm(depfiles, depfile);
	    
	    for (iter = xops->dep_mappers ; iter != 0 ; iter = iter->next)
	    {
		mapper_t *ma = (mapper_t *)iter->data;

		if ((targfile = mapper_map(ma, depfile)) != 0)
	    	    break;
	    }
	    /* TODO: expand targfile */
	}
	else
	{
    	    fileset_gather_mapped(task->fileset, xp->properties,
	    	    	    	  depfiles, xops->mappers);

    	    targfile = props_expand(xp->properties, xops->dep_target);
    	}
    }

    strnullnorm(targfile);
    
    if (verbose)
    	logmsg = logmsg_newnm(strarray_join(command, " "));
    else if (xops->logmessage != 0)
    	logmsg = logmsg_newnm(props_expand(xp->properties, xops->logmessage));

    if (targfile == 0)
    {
    	/* no dependency information -- job barrier, serialised */
    	if (!job_immediate_command(command, /*env*/0, logmsg))
	    xp->result = FALSE;
    }
    else
    {
    	/* have dependency information -- schedule job for later */
    	job_t *job;
	int i;
	
	job = job_add_command(targfile, command, /*env*/0, logmsg);
	for (i = 0 ; i < depfiles->len ; i++)
	    job_add_depend(job, strarray_nth(depfiles, i));
    }

    strdelete(targfile);
    strarray_delete(depfiles);

    return xp->result;    /* keep going unless failure */
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
	    fileset_apply(task->fileset, xp->properties,
	    	    	  xtask_execute_one, task);
	}
	else
	{
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
xtask_arg_new(void)
{
    xtask_arg_t *xa;
    
    xa = new(xtask_arg_t);
    
    condition_init(&xa->condition);
    
    return xa;
}

static void
xtask_arg_delete(xtask_arg_t *xa)
{
    switch (xa->type)
    {
    case XT_VALUE:    	/* <arg value=""> child */
    case XT_LINE:    	/* <arg line=""> child */
	strdelete(xa->data.arg);
    	break;
    case XT_FILESET:    /* <fileset> child */
	if (xa->data.fileset != 0)
    	    fileset_delete(xa->data.fileset);
    	break;
    case XT_FILES:  	/* <files> child */
    	break;
    }
    
    condition_free(&xa->condition);
    
    g_free(xa);
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
    listdelete(xops->args, xtask_arg_t, xtask_arg_delete);
    listdelete(xops->mappers, mapper_t, mapper_delete);
    listdelete(xops->dep_mappers, mapper_t, mapper_delete);
    
    strdelete(xops->task_ops.name);
    strdelete(xops->executable);
    strdelete(xops->logmessage);
    strdelete(xops->dep_target);
    
    props_delete(xops->property_map);

    g_free(xops);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static xtask_arg_t *
xtask_ops_add_arg(xtask_ops_t *xops, xtask_arg_type_t type)
{
    xtask_arg_t *xa;
    
    xa = xtask_arg_new();
    
    xa->type = type;
    xops->args = g_list_append(xops->args, xa);
    
    return xa;
}

xtask_arg_t *
xtask_ops_add_line(xtask_ops_t *xops, const char *s)
{
    xtask_arg_t *xa;
    
    xa = xtask_ops_add_arg(xops, XT_LINE);
    strassign(xa->data.arg, s);
    
    return xa;
}

xtask_arg_t *
xtask_ops_add_value(xtask_ops_t *xops, const char *s)
{
    xtask_arg_t *xa;
    
    xa = xtask_ops_add_arg(xops, XT_VALUE);
    strassign(xa->data.arg, s);
    
    return xa;
}

xtask_arg_t *
xtask_ops_add_fileset(xtask_ops_t *xops, fileset_t *fs)
{
    xtask_arg_t *xa;
    
    xa = xtask_ops_add_arg(xops, XT_FILESET);
    xa->data.fileset = fs;
    
    return xa;
}

xtask_arg_t *
xtask_ops_add_files(xtask_ops_t *xops)
{
    xtask_arg_t *xa;
    
    xa = xtask_ops_add_arg(xops, XT_FILES);
    
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
