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
#include <parser.h>

CVSID("$Id: buildfile.c,v 1.7 2001-11-13 03:02:55 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* static xmlParserCtxt *parser_context; */
static int num_errs;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

    	/* TODO: abort parse */

void
parse_error(const char *fmt, ...)
{
    va_list args;
    
    fprintf(stderr, "ERROR: ");
    va_start(args, fmt);
#if 0
    fprintf(stderr, "%s:%d: ",
    	parser_context->input->filename,
	parser_context->input->line);
#endif
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    num_errs++;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
parse_property(project_t *proj, xmlNode *node)
{
    char *name = 0;
    xmlAttr *attr;

    /* a lot hinges on whether attribute "name" is present */    
    name = xmlGetProp(node, "name");

    if (name != 0)
    {
    	/* exactly one of "value", "location" and "refid" may be present */
	char *value = xmlGetProp(node, "value");
	char *location = xmlGetProp(node, "location");
	char *refid = xmlGetProp(node, "refid");
	int n = 0;
	
	if (value != 0) n++;
	if (location != 0) n++;
	if (refid != 0) n++;

	if (n != 1)
	    parse_error("You must specify exactly one of \"value\", \"location\", and \"refid\"\n");
    	else if (value != 0)
	{
	    project_set_property(proj, name, value);
	}
	else if (location != 0)
	{
	    /* TODO: translate DOS format filenames */
	    if (location[0] == '/')
	    	project_set_property(proj, name, location);
	    else
	    {
	    	char *abs = g_strconcat(proj->basedir, "/", location, 0);
	    	project_set_property(proj, name, abs);
		g_free(abs);
	    }
	}
	else if (refid != 0)
	{
	    /* TODO */
	    fprintf(stderr, "<property refid=> not implemented\n");
	}
	
	if (value != 0)
    	    xmlFree(value);
	if (location != 0)
    	    xmlFree(location);
	if (refid != 0)
    	    xmlFree(refid);
    }
    else
    {
    	/* exactly one of "resource", "file" and "environment" may be present */
	char *resource = xmlGetProp(node, "resource");
	char *file = xmlGetProp(node, "file");
	char *environment = xmlGetProp(node, "environment");
	int n = 0;
	
	if (resource != 0) n++;
	if (file != 0) n++;
	if (environment != 0) n++;

	if (n != 1)
	    parse_error("You must specify exactly one of \"resource\", \"file\", and \"environment\"\n");
    	else if (resource != 0)
	{
	    /* TODO */
	    fprintf(stderr, "<property resource=> not implemented\n");
	}
	else if (file != 0)
	{
	    /* TODO */
	    fprintf(stderr, "<property file=> not implemented\n");
	}
	else if (environment != 0)
	{
	    /* TODO */
	    fprintf(stderr, "<property environment=> not implemented\n");
	}
	
	if (resource != 0)
    	    xmlFree(resource);
	if (file != 0)
    	    xmlFree(file);
	if (environment != 0)
    	    xmlFree(environment);
    }

    /* scan the attributes anyway, to detect misspellings etc */
    for (attr = node->properties ; attr != 0 ; attr = attr->next)
    {
    	char *value = cantXmlAttrValue(attr);
	
    	if (!strcmp(attr->name, "name") ||
	    !strcmp(attr->name, "value") ||
	    !strcmp(attr->name, "location") ||
	    !strcmp(attr->name, "refid") ||
	    !strcmp(attr->name, "resource") ||
	    !strcmp(attr->name, "file") ||
	    !strcmp(attr->name, "environment") ||
	    !strcmp(attr->name, "classpath") ||
	    !strcmp(attr->name, "classpathref"))
	    ;
	else
	    parse_error("Unknown attribute \"%s\" in \"property\"\n",
	    	    	attr->name);	/* TODO: warning? */

    	xmlFree(value);
    }
    

    if (name != 0)
    	xmlFree(name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
parse_xtaskdef(project_t *proj, xmlNode *node)
{
    char *buf;
    xtask_ops_t *xops;
    xmlNode *child;
    xtask_arg_t *xa;
    fileset_t *fs;
    
#if DEBUG
    fprintf(stderr, "Parsing xtaskdef\n");
#endif

    if ((buf = xmlGetProp(node, "name")) == 0)
    {
    	parse_error("Required attribute \"name\" missing\n");
	return;
    }

    xops = xtask_ops_new(buf);
    xmlFree(buf);

    /* TODO: xtask_ops_set_*() functions */
    xops->executable = xml2g(xmlGetProp(node, "executable"));
    xops->logmessage = xml2g(xmlGetProp(node, "logmessage"));
    xops->task_ops.is_fileset = cantXmlGetBooleanProp(node, "fileset", FALSE);
    xops->task_ops.fileset_dir_name = "dir";	/* TODO */
    xops->foreach = cantXmlGetBooleanProp(node, "foreach", FALSE);
    xops->dep_target = xml2g(xmlGetProp(node, "deptarget"));

    /* TODO: syntax check other attributes */
    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;

    	xa = 0;
    	if (!strcmp(child->name, "arg"))
	{
	    if ((buf = xmlGetProp(child, "value")) != 0)
	    {
	    	xa = xtask_ops_add_value(xops, buf);
	    	xmlFree(buf);
	    }
	    else if ((buf = xmlGetProp(child, "line")) != 0)
	    {
	    	xa = xtask_ops_add_line(xops, buf);
	    	xmlFree(buf);
	    }
	    else
	    	parse_error("One of \"line\" or \"value\" must be set\n");
	}
	else if (!strcmp(child->name, "attr"))
	{
	    char *from = 0, *to = 0;
	    
	    if ((from = xmlGetProp(child, "attribute")) == 0)
	    	parse_error("Required attribute \"attribute\" missing\n");
	    else if ((to = xmlGetProp(child, "property")) == 0)
	    	to = xmlMemStrdup(from);

    	    if (from != 0)
    	    	xtask_ops_add_attribute(xops, from, to,
		    	cantXmlGetBooleanProp(child, "required", FALSE));
	    /* TODO: specify default value */
	    if (from != 0)
		xmlFree(from);
	    if (to != 0)
		xmlFree(to);
	}
	else if (!strcmp(child->name, "fileset"))
	{
	    if ((fs = parse_fileset(proj, child, "dir")) != 0)
	    	xa = xtask_ops_add_fileset(xops, fs);
	}
	else if (!strcmp(child->name, "files"))
	{
	    xa = xtask_ops_add_files(xops);
	}
	else if (!strcmp(child->name, "mapper"))
	{
	    mapper_t *ma;
	    
	    if ((ma = parse_mapper(proj, child)) != 0)
		xops->mappers = g_list_append(xops->mappers, ma);
	}
	else if (!strcmp(child->name, "depmapper"))
	{
	    mapper_t *ma;
	    
	    if ((ma = parse_mapper(proj, child)) != 0)
		xops->dep_mappers = g_list_append(xops->dep_mappers, ma);
	}
	else
	    parse_error("Unexpected child \"%s\"\n", child->name);

    	if (xa == 0)
	    continue;

    	/* TODO: need a condition.c with condition_parse() */
    	if ((buf = xmlGetProp(child, "if")) != 0)
	{
	    xtask_arg_set_if_condition(xa, buf);
	    xmlFree(buf);
	}	    
    	else if ((buf = xmlGetProp(child, "unless")) != 0)
	{
	    xtask_arg_set_unless_condition(xa, buf);
	    xmlFree(buf);
	}	    
	
    }    

    /* TODO: handle duplicate registrations cleanly */
    task_ops_register((task_ops_t *)xops);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t *
parse_fileset(project_t *proj, xmlNode *node, const char *dirprop)
{
    fileset_t *fs;
    fs_spec_t *fss;
    char *buf, *buf2, *x;
    xmlNode *child;
    static const char sep[] = ", \t\n\r";

#if DEBUG
    fprintf(stderr, "parsing fileset\n");
#endif

    if ((buf = xmlGetProp(node, "refid")) != 0)
    {
    	/* Handle reference to an existing fileset */
    	if ((buf2 = xmlGetProp(node, "id")) != 0)
	{
	    parse_error("Cannot specify both \"id\" and \"refid\"\n");
	    xmlFree(buf);
	    xmlFree(buf2);
	    return 0;
	}
	
	if ((fs = project_find_fileset(proj, buf)) == 0)
	{
	    /*
    	     * TODO: order dependency.... May need to scan the project
	     *       for non-tasks first before parsing tasks.
	     */
	    parse_error("Cannot find fileset \"%s\" to satisfy \"refid\"\n", buf);
	    xmlFree(buf);
	    return 0;
	}
	/* TODO: refcount to prevent problems deleting the project */
	
    	xmlFree(buf);
	return fs;
    }

    /* Handle definition of a new fileset */
    if (dirprop != 0 && (buf = xmlGetProp(node, dirprop)) == 0)
    {
    	parse_error("Required attribute \"%s\" not present\n", dirprop);
    	return 0;
    }

    fs = fileset_new(proj);
    fileset_set_directory(fs, buf);
    xmlFree(buf);


    if ((buf = xmlGetProp(node, "id")) != 0)
    {
    	strassign(fs->id, buf);
    	xmlFree(buf);
    }
    

    fileset_set_case_sensitive(fs,
    	    	    cantXmlGetBooleanProp(node, "casesensitive", TRUE));
    fileset_set_default_excludes(fs,
    	    	    cantXmlGetBooleanProp(node, "defaultexcludes", TRUE));


    /*
     * "includes" attribute
     */
    buf = buf2 = xmlGetProp(node, "includes");
    if (buf != 0)
	while ((x = strtok(buf2, sep)) != 0)
	{
	    buf2 = 0;
	    fileset_add_include(fs, x);
	}
    xmlFree(buf);

    /*
     * "includesfile" attribute
     */
    buf = xmlGetProp(node, "includesfile");
    if (buf != 0)
	fileset_add_include_file(fs, buf);
    xmlFree(buf);
    

    /*
     * "excludes" attribute
     */
    buf = buf2 = xmlGetProp(node, "excludes");
    if (buf != 0)
	while ((x = strtok(buf2, sep)) != 0)
	{
	    buf2 = 0;
	    fileset_add_exclude(fs, x);
	}
    xmlFree(buf);

    /*
     * "excludesfile" attribute
     */
    buf = xmlGetProp(node, "excludesfile");
    if (buf != 0)
	fileset_add_exclude_file(fs, buf);
    xmlFree(buf);
    
    /*
     * children
     */
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	    
	if ((buf = xmlGetProp(child, "name")) == 0)
	{
	    parse_error("Required attribute \"name\" missing\n");
	    continue;
	}
	
	fss = 0;
	if (!strcmp(child->name, "include"))
	    fss = fileset_add_include(fs, buf);
	else if (!strcmp(child->name, "includesfile"))
	    fss = fileset_add_include_file(fs, buf);
	else if (!strcmp(child->name, "exclude"))
	    fss = fileset_add_exclude(fs, buf);
	else if (!strcmp(child->name, "excludesfile"))
	    fss = fileset_add_exclude_file(fs, buf);
	    
	xmlFree(buf);
	if (fss == 0)
	    continue;

    	if ((buf = xmlGetProp(child, "if")) != 0)
	{
	    fs_spec_set_if_condition(fss, buf);
	    xmlFree(buf);
	}	    
    	else if ((buf = xmlGetProp(child, "unless")) != 0)
	{
	    fs_spec_set_unless_condition(fss, buf);
	    xmlFree(buf);
	}	    

    }
    
    return fs;
}


static void
parse_project_fileset(project_t *proj, xmlNode *node)
{
    fileset_t *fs;
    
    if ((fs = parse_fileset(proj, node, "dir")) == 0)
    	return;
	
    if (fs->id == 0)
    {
    	parse_error("<fileset> at project scope must have \"id\" attribute\n");
    	return;
    }
    /* TODO: detect duplicate fileset ids */
    project_add_fileset(proj, fs);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
parse_path(project_t *proj, xmlNode *node)
{
#if DEBUG
    fprintf(stderr, "ignoring path\n");
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_t *
parse_mapper(project_t *proj, xmlNode *node)
{
    char *name;
    char *from;
    char *to;
    
    if ((name = xmlGetProp(node, "name")) == 0)
    {
    	parse_error("Required attribute \"%s\" missing\n", "name");
    	return 0;
    }

    if ((from = xmlGetProp(node, "from")) == 0)
    {
    	parse_error("Required attribute \"%s\" missing\n", "from");
	xmlFree(name);
    	return 0;
    }

    if ((to = xmlGetProp(node, "to")) == 0)
    {
    	parse_error("Required attribute \"%s\" missing\n", "to");
	xmlFree(name);
	xmlFree(from);
    	return 0;
    }
    
    return mapper_new(name, from, to);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
check_one_attribute(const task_attr_t *ta, void *userdata)
{
    xmlNode *node = (xmlNode *)userdata;
    char *value;

    if (!(ta->flags & TT_REQUIRED))
    	return;
	
    if ((value = xmlGetProp(node, ta->name)) == 0)
    {
	parse_error("Required attribute \"%s\" missing\n", ta->name);
	/* TODO: task_delete(task); */
	return;
    }
    xmlFree(value);
}

static gboolean
dummy_task_execute(task_t *task)
{
    logf("dummy\n");
    return TRUE;
}

static task_ops_t dummy_ops = 
{
    "dummy",
    /*init*/0,
    /*parse*/0,
    dummy_task_execute,
    /*delete*/0,
    /*attrs*/0
};

task_t *
parse_task(project_t *proj, xmlNode *node)
{
    task_t *task;
    xmlAttr *attr;
    xmlNode *child;
    task_ops_t *ops;
    task_child_t *tc;
    
#if DEBUG
    fprintf(stderr, "parse_task: parsing task \"%s\"\n", node->name);
#endif

    if ((ops = task_ops_find(node->name)) == 0)
    {
    	parse_error("Unknown task \"%s\"\n", node->name);
	
	/* TODO: return 0 */
	ops = g_memdup(&dummy_ops, sizeof(dummy_ops));
	ops->name = g_strdup(node->name);
    }

    task = task_new();
    task->project = proj;
    task->ops = ops;
    task_set_name(task, ops->name);
    
    if (ops->new != 0)
    	(*ops->new)(task);
    
    for (attr = node->properties ; attr != 0 ; attr = attr->next)
    {
    	char *value = cantXmlAttrValue(attr);
	
    	if (!strcmp(attr->name, "id"))
	    task_set_id(task, value);
    	else if (!strcmp(attr->name, "taskname"))
	    task_set_name(task, value);
    	else if (!strcmp(attr->name, "description"))
	    task_set_description(task, value);
	else if (ops->is_fileset && !strcmp(attr->name, ops->fileset_dir_name))
	    ;	/* parse_fileset will get it later */
	else
	{
	    if (!task_set_attribute(task, attr->name, value))
	    {
	    	task_delete(task);
	    	return 0;
	    }
	    /* TODO: whinge about superfluous attributes */
	}
    	xmlFree(value);
    }
    
    /* scan for required attributes and whinge about their absence */
    task_ops_attributes_apply(ops, check_one_attribute, node);
    /* TODO: whinge about absence of "dir" */

    if (ops->is_fileset &&
    	(task->fileset = parse_fileset(proj, node, ops->fileset_dir_name)) == 0)
    {
    	task_delete(task);
	return 0;
    }

    /* parse the child nodes */
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	/* TODO: handle character data */
	
	tc = (task_child_t *)g_hash_table_lookup(ops->children_hashed, child->name);
	if (tc != 0 && !(*tc->adder)(task, child))
	{
	    task_delete(task);
	    return 0;
	}
	/* TODO: whinge about superfluous children */
    }
    
    /* TODO: scan for required children */
    
    if (ops->post_parse != 0 && !(*ops->post_parse)(task))
    {
    	task_delete(task);
	return 0;
    }

    return task;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
add_depends(project_t *proj, target_t *targ, const char *str)
{
    char *buf, *buf2, *x;
    target_t *dep;
    
    buf = buf2 = g_strdup(str);
    
    while ((x = strtok(buf2, ", \t\n\r")) != 0)
    {
    	buf2 = 0;
	
#if DEBUG
    	fprintf(stderr, "add_depends: \"%s\" depends on \"%s\"\n",
	    	    targ->name, x);
#endif
    	if ((dep = project_find_target(proj, x)) == 0)
	{
	    /* handle forward references */
	    dep = target_new();
	    target_set_name(dep, x);
	    project_add_target(proj, dep);
	}
	
	dep->flags |= T_DEPENDED_ON;
	targ->depends = g_list_append(targ->depends, dep);
    }
    
    g_free(buf);
}


static void
parse_target(project_t *proj, xmlNode *node)
{
    target_t *targ;
    char *name = 0;
    xmlAttr *attr;
    xmlNode *child;
    
    
    /*
     * We need to detect the (illegal) case that the target
     * depends directly on itself, so we have to add it before
     * we've parsed all the attributes, but of course we need
     * to scan the attributes to get the name to add...
     */
    name = xmlGetProp(node, "name");
    
    if (name == 0)
    {
    	parse_error("Required attribute \"name\" missing from \"task\"\n");
    	return;
    }
    
    targ = target_new();
    target_set_name(targ, name);
    xmlFree(name);
    
    targ->flags |= T_DEFINED;
    project_add_target(proj, targ);


    /*
     * Now parse the attributes for real.
     */    
    for (attr = node->properties ; attr != 0 ; attr = attr->next)
    {
    	char *value = cantXmlAttrValue(attr);
	
    	if (!strcmp(attr->name, "name"))
	    /* not an error */;
    	else if (!strcmp(attr->name, "description"))
	    target_set_description(targ, value);
    	else if (!strcmp(attr->name, "depends"))
	    add_depends(proj, targ, value);
    	else if (!strcmp(attr->name, "if"))
	    target_set_if_condition(targ, value);
    	else if (!strcmp(attr->name, "unless"))
	    target_set_unless_condition(targ, value);
	else
	    parse_error("Unknown attribute \"%s\" on \"target\"\n", attr->name);

    	xmlFree(value);
    }

    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type == XML_ELEMENT_NODE)
	{
    	    task_t *task;
	    if ((task = parse_task(targ->project, child)) != 0)
		target_add_task(targ, task);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
check_one_target(gpointer key, gpointer value, gpointer userdata)
{
/*    project_t *proj = (project_t *)userdata; */
    target_t *targ = (target_t *)value;
    
    if ((targ->flags & (T_DEFINED|T_DEPENDED_ON)) == T_DEPENDED_ON)
    	parse_error("Target \"%s\" is depended on but never defined\n", targ->name);
}

static project_t *
parse_project(xmlNode *node)
{
    project_t *proj;
    xmlAttr *attr;
    xmlNode *child;
   
    proj = project_new();
    
    for (attr = node->properties ; attr != 0 ; attr = attr->next)
    {
    	char *value = cantXmlAttrValue(attr);
	
    	if (!strcmp(attr->name, "name"))
	    project_set_name(proj, value);
    	else if (!strcmp(attr->name, "default"))
	    project_set_default_target(proj, value);
    	else if (!strcmp(attr->name, "basedir"))
	    project_set_basedir(proj, value);
	else
	    parse_error("Unknown attribute \"%s\" on \"project\"\n", attr->name);

    	xmlFree(value);
    }
    
    if (proj->default_target == 0)
    	parse_error("Required attribute \"default\" missing from \"project\"\n");

    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	    
    	if (!strcmp(child->name, "property"))
	    parse_property(proj, child);
	else if (!strcmp(child->name, "path"))
	    parse_path(proj, child);
    	else if (!strcmp(child->name, "fileset"))
	    parse_project_fileset(proj, child);
    	else if (!strcmp(child->name, "xtaskdef"))
	    parse_xtaskdef(proj, child);
	else if (!strcmp(child->name, "target"))
	    parse_target(proj, child);
	else
	    /* TODO: fileset */
	    /* TODO: patternset */
	    parse_error("Element \"%s\" unexpected at this point.\n", child->name);
    }
    
    g_hash_table_foreach(proj->targets, check_one_target, proj);
    
    return proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

project_t *
read_buildfile(const char *filename)
{
    xmlDoc *doc;
    xmlNode *node;
    project_t *proj;

    if ((doc = xmlParseFile(filename)) == 0)
    {
    	/* TODO: print error message */
	fprintf(stderr, "Failed to load file \"%s\"\n", filename);
	return 0;
    }
        
    num_errs = 0;
    for (node = doc->root ; node != 0 ; node = node->next)
    {
    	if (node->type != XML_ELEMENT_NODE)
	    continue;
	    
	if (!strcmp(node->name, "project"))
	{
	    proj = parse_project(node);
	    if (proj != 0)
	    	project_set_filename(proj, filename);
	    break;
	}
	else
	    parse_error("Element \"%s\" unexpected at this point.\n",
		    	    node->name);
    }
    
    if (num_errs > 0)
    	fprintf(stderr, "%s: found %d errors\n", filename, num_errs);
	
    xmlFreeDoc(doc);
    
    return proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
