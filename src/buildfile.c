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
#include "xtask.h"

CVSID("$Id: buildfile.c,v 1.17 2001-11-21 07:17:31 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int num_errs;
static GList *parse_node_stack;     /* just for error reporting */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
parse_node_push(const xmlNode *node)
{
    parse_node_stack = g_list_prepend(parse_node_stack, (gpointer)node);
}

static void
parse_node_pop(void)
{
    parse_node_stack = g_list_remove_link(parse_node_stack, parse_node_stack);
}

const xmlNode *
parse_node_top(void)
{
    return (parse_node_stack == 0 ? 0 :
    	    	(const xmlNode *)parse_node_stack->data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
parse_node_errorv(const xmlNode *node, const char *fmt, va_list args)
{
    const node_info_t *ni;

    if (node != 0 && (ni = cantXmlNodeInfoGet(node)) != 0)
	fprintf(stderr, "%s:%d: ", ni->filename, ni->lineno);
    fprintf(stderr, "ERROR: ");
    
    vfprintf(stderr, fmt, args);
    
    num_errs++;
}

void
parse_node_error(const xmlNode *node, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    parse_node_errorv(node, fmt, args);
    va_end(args);
}

void
parse_error(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    parse_node_errorv(parse_node_top(), fmt, args);
    va_end(args);
}

void
parse_error_unknown_attribute(const xmlAttr *attr)
{
    parse_node_error(attr->node, "Unknown attribute \"%s\" on \"%s\"\n",
    	    	attr->name, attr->node->name);
}

void
parse_error_required_attribute(const xmlNode *node, const char *attrname)
{
    parse_node_error(node, "Required attribute \"%s\" missing from \"%s\"\n",
    	    	attrname, node->name);
}

void
parse_error_unexpected_element(const xmlNode *node)
{
    parse_node_error(node, "Element \"%s\" unexpected at this point\n",
    	    	node->name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Parse the condition-related attributes on a node
 * into the condition_t and return TRUE iff the
 * parse was successful.
 */
 
gboolean
parse_condition(condition_t *cond, xmlNode *node)
{
    char *if_attr = xmlGetProp(node, "if");
    char *unless_attr = xmlGetProp(node, "unless");
    char *matches_attr = xmlGetProp(node, "matches");
    char *matchesregex_attr = xmlGetProp(node, "matchesregex");
    gboolean case_sens = cantXmlGetBooleanProp(node, "matchsensitive", TRUE);
    gboolean res = FALSE;
    
    if (if_attr != 0 && unless_attr != 0)
    	parse_node_error(node, "Cannot specify both \"if\" and \"unless\" attributes\n");
    else if (matches_attr != 0 && matchesregex_attr != 0)
    	parse_node_error(node, "Cannot specify both \"matches\" and \"matchesregex\" attributes\n");
    else if ((matches_attr != 0 || matchesregex_attr != 0) &&
    	     (if_attr == 0 && unless_attr == 0))
    	parse_node_error(node, "Cannot specify \"matches\" or \"matchesregex\" attributes without \"if\" or \"unless\"\n");
    else
    {
    	if (if_attr != 0)
	    condition_set_if(cond, if_attr);
    	else if (unless_attr != 0)
	    condition_set_unless(cond, unless_attr);
	    
	if (matches_attr != 0)
	    condition_set_matches(cond, matches_attr, case_sens);
	else if (matchesregex_attr != 0)
	    condition_set_matches_regex(cond, matchesregex_attr, case_sens);

    	res = TRUE; 	/* success!! */
    }
    
    xmlFree(if_attr);
    xmlFree(unless_attr);
    xmlFree(matches_attr);
    xmlFree(matchesregex_attr);
    return res;
}

static gboolean
is_condition_attribute(const char *attrname)
{
    return (!strcmp(attrname, "if") ||
    	    !strcmp(attrname, "unless") ||
    	    !strcmp(attrname, "matches") ||
    	    !strcmp(attrname, "matchesregex") ||
    	    !strcmp(attrname, "matchsensitive"));
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
	    parse_node_error(node, "You must specify exactly one of \"value\", \"location\", and \"refid\"\n");
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
	    parse_node_error(node, "You must specify exactly one of \"resource\", \"file\", and \"environment\"\n");
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
	    parse_error_unknown_attribute(attr);	/* TODO: warning? */

    	xmlFree(value);
    }
    

    if (name != 0)
    	xmlFree(name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
check_childless(xmlNode *node)
{
    xmlNode *child;
    gboolean res = TRUE;

    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;

    	parse_error_unexpected_element(child);
	res = FALSE;
    }
    
    return res;
}


static tagexp_t *
parse_tagexpand(project_t *proj, xmlNode *node)
{
    char *buf;
    tagexp_t *te;
    xmlNode *child;
    gboolean failed = FALSE;

#if DEBUG
    fprintf(stderr, "Parsing tagexpand\n");
#endif

    if ((buf = xmlGetProp(node, "namespace")) == 0)
    {
	parse_error_required_attribute(node, "namespace");
	return 0;
    }
    
    te = tagexp_new(buf);
    xmlFree(buf);

#if 0 /* TODO */
    te->follow_depends = cantXmlGetBooleanProp(node, "followdepends", FALSE);
    te->reverse_order = cantXmlGetBooleanProp(node, "reverse", FALSE);
#endif

    if ((buf = xmlGetProp(node, "default")) != 0)
    {
    	tagexp_add_default_expansion(te, buf);
    	xmlFree(buf);
    }
    
    /* TODO: check no other attributes are present */
    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;

    	if (!strcmp(child->name, "expand"))
	{
	    char *tag = 0, *to = 0;
	    
	    if ((tag = xmlGetProp(child, "tag")) == 0)
	    {
	    	parse_error_required_attribute(child, "tag");
	    	failed = TRUE;
	    }
	    	    
	    if ((to = xmlGetProp(child, "to")) == 0)
	    {
	    	parse_error_required_attribute(child, "to");
	    	failed = TRUE;
	    }
	    
	    /* TODO: check no other attributes are present */
	    
    	    if (tag != 0 && to != 0)
	    	tagexp_add_expansion(te, tag, to);

	    if (!check_childless(child))
	    	failed = TRUE;
	    
	    if (tag != 0)
	    	xmlFree(tag);
	    if (to != 0)
	    	xmlFree(to);
	}
    	else if (!strcmp(child->name, "default"))
	{
	    char *to = 0;
	    
	    if ((to = xmlGetProp(child, "to")) == 0)
	    {
	    	parse_error_required_attribute(child, "to");
	    	failed = TRUE;
	    }
	    
	    /* TODO: check no other attributes are present */
	    
    	    if (to != 0)
	    	tagexp_add_default_expansion(te, to);

	    if (!check_childless(child))
	    	failed = TRUE;
	    
	    if (to != 0)
	    	xmlFree(to);
	}
	else
	{
	    parse_error_unexpected_element(child);
	    failed = TRUE;
	}
    }
    
    
    if (failed)
    {
    	tagexp_delete(te);
	te = 0;
    }
        
    return te;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

taglist_t *
parse_taglist(project_t *proj, xmlNode *node)
{
    char *buf, *buf2;
    tl_def_t *tldef;
    taglist_t *tl;
    const tl_def_tag_t *tag;
    xmlNode *child;
    gboolean failed = FALSE;
    const char *namespace = node->name;
    

#if DEBUG
    fprintf(stderr, "Parsing taglist \"%s\"\n", namespace);
#endif

    if ((tldef = project_find_tl_def(proj, namespace)) == 0)
    {
    	parse_node_error(node, "Unknown taglist type \"%s\"\n", namespace);
	return 0;
    }
    
    if ((buf = xmlGetProp(node, "refid")) != 0)
    {
    	/* Handle reference to an existing taglist */
    	if ((buf2 = xmlGetProp(node, "id")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"id\" and \"refid\"\n");
	    xmlFree(buf);
	    xmlFree(buf2);
	    return 0;
	}
	
	if ((tl = project_find_taglist(proj, namespace, buf)) == 0)
	{
	    /* TODO: order dependency */
	    parse_node_error(node, "Cannot find taglist \"%s\" to satisfy \"refid\"\n", buf);
	    xmlFree(buf);
	    return 0;
	}
	/* TODO: refcount to prevent problems deleting the project */
	
    	xmlFree(buf);
	return tl;
    }

    /* actual taglist definition */
    if ((buf = xmlGetProp(node, "id")) == 0)
    {
	parse_error_required_attribute(node, "id");
	return 0;
    }

    tl = taglist_new(namespace);
    taglist_set_id(tl, buf);
    xmlFree(buf);
    
    /* TODO: parse optional "depends" attribute */
    /* TODO: syntax check other attributes */
    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;

    	if ((tag = tl_def_find_tag(tldef, child->name)) != 0)
	{
	    char *name = xmlGetProp(child, "name");
	    char *value = 0;
	    tl_item_t *tlitem = 0;

	    if (name == 0 && tag->name_avail == AV_MANDATORY)
	    {
		parse_error_required_attribute(child, "name");
		failed = TRUE;
	    }
	    else if (name != 0 && tag->name_avail == AV_FORBIDDEN)
	    {
	    	parse_node_error(child, "Attribute \"name\" not allowed at this point\n");
	    	failed = TRUE;
	    }

	    if ((value = xmlGetProp(child, "value")) != 0)
		tlitem = taglist_add_item(tl, child->name, name, TL_VALUE, value);
	    else if ((value = xmlGetProp(child, "line")) != 0)
		tlitem = taglist_add_item(tl, child->name, name, TL_LINE, value);
	    else if ((value = xmlGetProp(child, "file")) != 0)
		tlitem = taglist_add_item(tl, child->name, name, TL_FILE, value);
	    
	    if (value == 0 && tag->value_avail == AV_MANDATORY)
	    {
		parse_node_error(child, "One of \"value\", \"line\" or \"file\" must be present\n");
		failed = TRUE;
	    }
	    else if (value != 0 && tag->value_avail == AV_FORBIDDEN)
	    {
	    	parse_node_error(child, "None of \"value\", \"line\" or \"file\" may be present\n");
	    	failed = TRUE;
	    }

    	    if (tlitem != 0 && !parse_condition(&tlitem->condition, child))
	    	failed = TRUE;

	    if (name != 0)
		xmlFree(name);
	    if (value != 0)
		xmlFree(value);
    	}
	else
	{
	    parse_error_unexpected_element(child);
	    failed = FALSE;
	}
    }
    
    
    if (failed)
    {
    	taglist_delete(tl);
	tl = 0;
    }
        
    return tl;
}

static gboolean
parse_project_taglist(project_t *proj, xmlNode *node)
{
    taglist_t *tl;
    
    if ((tl = parse_taglist(proj, node)) == 0)
    	return FALSE;
	
    if (project_find_taglist(proj, tl->namespace, tl->id) != 0)
    {
    	parse_node_error(node, "Duplicate taglist %s::%s\n", tl->namespace, tl->id);
	taglist_delete(tl);
	return FALSE;
    }

    project_add_taglist(proj, tl);
    return TRUE;
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
	parse_error_required_attribute(node, "name");
	return;     /* TODO: return failure */
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
	    else if ((buf = xmlGetProp(child, "file")) != 0)
	    {
	    	xa = xtask_ops_add_file(xops, buf);
	    	xmlFree(buf);
	    }
	    else
	    	parse_node_error(node, "One of \"line\", \"value\" or \"file\" must be set\n");
	}
	else if (!strcmp(child->name, "attr"))
	{
	    char *from = 0, *to = 0;
	    
	    if ((from = xmlGetProp(child, "attribute")) == 0)
		parse_error_required_attribute(node, "attribute");
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
	else if (!strcmp(child->name, "taglist"))
	{
	    char *namespace = 0;
	    
	    if ((namespace = xmlGetProp(child, "namespace")) == 0)
		parse_error_required_attribute(node, "namespace");

	    xtask_ops_add_child(xops, namespace);
	    
	    if (namespace != 0)
		xmlFree(namespace);
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
	else if (!strcmp(child->name, "tagexpand"))
	{
	    tagexp_t *te;
	    
	    /* TODO: propagate failure */
	    if ((te = parse_tagexpand(proj, child)) != 0)
		xa = xtask_ops_add_tagexpand(xops, te);
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
	    parse_error_unexpected_element(child);

    	if (xa == 0)
	    continue;

    	if (!parse_condition(&xa->condition, child))
	    continue;	/* TODO: do something more drastic */
    }    

    /* TODO: handle duplicate registrations cleanly */
    tscope_register(proj->tscope, (task_ops_t *)xops);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
parse_availability_attr(
    xmlNode *node,
    const char *attrname,
    availability_t *availp)
{
    char *val;
    
    /* default value */
    *availp = AV_OPTIONAL;

    if ((val = xmlGetProp(node, attrname)) == 0)
    	return TRUE;
	
    if (!strcasecmp(val, "mandatory"))
    	*availp = AV_MANDATORY;
    else if (!strcasecmp(val, "optional"))
    	*availp = AV_OPTIONAL;
    else if (!strcasecmp(val, "forbidden"))
    	*availp = AV_FORBIDDEN;
    else
    {
    	parse_node_error(node, "Attribute \"%s\" must be one of \"mandatory\", \"optional\" or \"forbidden\"\n",
	    	    	    attrname);
    	xmlFree(val);
    	return FALSE;
    }
	
    xmlFree(val);
    return TRUE;
}


static gboolean
parse_taglistdef(project_t *proj, xmlNode *node)
{
    char *buf;
    tl_def_t *tldef;
    tl_def_tag_t *tltag;
    xmlNode *child;
    gboolean failed = FALSE;
    
#if DEBUG
    fprintf(stderr, "Parsing taglistdef\n");
#endif

    if ((buf = xmlGetProp(node, "name")) == 0)
    {
	parse_error_required_attribute(node, "name");
	return FALSE;
    }

    tldef = tl_def_new(buf);
    xmlFree(buf);

    /* TODO: syntax check other attributes */
    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;

    	tltag = 0;
    	if (!strcmp(child->name, "tag"))
	{
	    char *tag = 0;
	    availability_t name_avail = AV_OPTIONAL;
	    availability_t value_avail = AV_OPTIONAL;
	    
	    if ((tag = xmlGetProp(child, "tag")) == 0)
	    {
	    	parse_error_required_attribute(child, "tag");
	    	failed = TRUE;
	    }
	    
	    if (!parse_availability_attr(child, "name", &name_avail))
	    	failed = TRUE;
	    if (!parse_availability_attr(child, "value", &value_avail))
	    	failed = TRUE;

    	    if (tag != 0)	    
	    {
		tltag = tl_def_add_tag(tldef, tag, name_avail, value_avail);
		xmlFree(tag);
	    }
	}
	else
	    parse_error_unexpected_element(child);
    }    

    /* TODO: handle duplicate registrations cleanly */
    project_add_tl_def(proj, tldef);
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
	    parse_node_error(node, "Cannot specify both \"id\" and \"refid\"\n");
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
	    parse_node_error(node, "Cannot find fileset \"%s\" to satisfy \"refid\"\n", buf);
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
	parse_error_required_attribute(node, dirprop);
    	return 0;
    }

    fs = fileset_new();
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
	    parse_error_required_attribute(node, "name");
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

    	if (!parse_condition(&fss->condition, child))
	    continue;	    /* TODO: do something more drastic!!! */
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
    	parse_node_error(node, "<fileset> at project scope must have \"id\" attribute\n");
    	return;
    }
    /* TODO: detect duplicate fileset ids */
    project_add_fileset(proj, fs);
}


static gboolean
is_fileset_attribute(const char *attrname, const char *dirprop)
{
    return ((dirprop != 0 && !strcmp(attrname, dirprop)) ||
	    !strcmp(attrname, "includes") ||
    	    !strcmp(attrname, "includesfile") ||
    	    !strcmp(attrname, "excludes") ||
    	    !strcmp(attrname, "excludesfile") ||
    	    !strcmp(attrname, "casesensitive") ||
    	    !strcmp(attrname, "defaultexcludes") ||
    	    !strcmp(attrname, "refid"));
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
	parse_error_required_attribute(node, "name");
    	return 0;
    }

    if ((from = xmlGetProp(node, "from")) == 0)
    {
	parse_error_required_attribute(node, "from");
	xmlFree(name);
    	return 0;
    }

    if ((to = xmlGetProp(node, "to")) == 0)
    {
	parse_error_required_attribute(node, "to");
	xmlFree(name);
	xmlFree(from);
    	return 0;
    }
    
    return mapper_new(name, from, to);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    xmlNode *node;
    gboolean failed;
} attr_check_rec_t;

static void
check_one_attribute(const task_attr_t *ta, void *userdata)
{
    attr_check_rec_t *rec = (attr_check_rec_t *)userdata;
    char *value;

    if (!(ta->flags & TT_REQUIRED))
    	return;
	
    if ((value = xmlGetProp(rec->node, ta->name)) == 0)
    {
	parse_error_required_attribute(rec->node, ta->name);
	rec->failed = TRUE;
    }
    else
	xmlFree(value);
}

task_t *
parse_task(project_t *proj, xmlNode *node)
{
    task_t *task;
    xmlAttr *attr;
    xmlNode *child;
    task_ops_t *ops;
    const task_child_t *tc;
    char *content;
    gboolean failed = FALSE;
    
#if DEBUG
    fprintf(stderr, "parse_task: parsing task \"%s\"\n", node->name);
#endif

    if ((ops = tscope_find(proj->tscope, node->name)) == 0)
    {
    	parse_node_error(node, "Unknown task \"%s\"\n", node->name);
	return 0;
    }

    parse_node_push(node);
    
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
	else if (ops->is_fileset &&
	    	 is_fileset_attribute(attr->name, ops->fileset_dir_name))
	    ;	/* parse_fileset will get it later */
	else if (!task_set_attribute(task, attr->name, value))
	{
	    parse_error_unknown_attribute(attr);
	    failed = TRUE;
	}
    	xmlFree(value);
    }
    
    /* scan for required attributes and whinge about their absence */
    {
    	attr_check_rec_t rec;
	
	rec.node = node;
	rec.failed = failed;
	task_ops_attributes_apply(ops, check_one_attribute, &rec);
	failed = rec.failed;
    }

    if (ops->is_fileset &&
    	(task->fileset = parse_fileset(proj, node, ops->fileset_dir_name)) == 0)
    	failed = TRUE;

    /* parse the child nodes */
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	/* TODO: handle character data */
	
	if ((tc = task_ops_find_child(ops, child->name)) == 0)
	{
	    parse_error_unexpected_element(child);
	    failed = TRUE;
	    continue;
    	}
	parse_node_push(child);
	if (!(*tc->adder)(task, child))
	    failed = TRUE;
	parse_node_pop();
    }
    
    /* TODO: scan for required children */
    
    /* handle text content */
    if ((content = xmlNodeGetContent(node)) != 0)
    {
    	if (ops->set_content != 0 && !(*ops->set_content)(task, content))
	    failed = TRUE;
    	xmlFree(content);
    }

    /* call the task's post-parse function */    
    if (!failed && ops->post_parse != 0)
    {
	if (!(*ops->post_parse)(task))
	    failed = TRUE;
    }

    parse_node_pop();

    if (failed)
    {
    	task_delete(task);
	task = 0;
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


static gboolean
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
	parse_error_required_attribute(node, "name");
    	return FALSE;
    }
    
    if ((targ = project_find_target(proj, name)) == 0)
    {
	targ = target_new();
	target_set_name(targ, name);
	project_add_target(proj, targ);
    }
    else
    {
    	if (targ->flags & T_DEFINED)
	{
	    parse_node_error(node, "Target \"%s\" already defined\n", name);
	    xmlFree(name);
	    return FALSE;
	}
    }
    xmlFree(name);
    targ->flags |= T_DEFINED;
    
    if (!parse_condition(&targ->condition, node))
    {
    	project_remove_target(proj, targ);
    	target_delete(targ);
	return FALSE;
    }


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
    	else if (is_condition_attribute(attr->name))
	    ;
	else
	    parse_error_unknown_attribute(attr);

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
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
check_one_target(gpointer key, gpointer value, gpointer userdata)
{
    target_t *targ = (target_t *)value;
    
    if ((targ->flags & (T_DEFINED|T_DEPENDED_ON)) == T_DEPENDED_ON)
    	parse_node_error(0, "Target \"%s\" is depended on but never defined\n", targ->name);
}

static project_t *
parse_project(xmlNode *node, project_t *parent)
{
    project_t *proj;
    xmlNode *child;
    gboolean globals = (parent == 0);
   
    proj = project_new(parent);
    
    if (globals)
    {
    	project_set_name(proj, "globals");
	project_set_basedir(proj, ".");
    }
    else
    {
	xmlAttr *attr;
	
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
		parse_error_unknown_attribute(attr);

    	    xmlFree(value);
	}
	if (proj->default_target == 0)
	    parse_error_required_attribute(node, "default");
    }
        

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
    	else if (!strcmp(child->name, "taglistdef"))
	    parse_taglistdef(proj, child);
	else if (!globals && !strcmp(child->name, "target"))
	    /* TODO: propagate failure */
	    parse_target(proj, child);
	else if (project_find_tl_def(proj, child->name) != 0)
	    parse_project_taglist(proj, child);
	else
	    /* TODO: fileset */
	    /* TODO: patternset */
	    parse_error_unexpected_element(child);
    }

    if (!globals)    
	g_hash_table_foreach(proj->targets, check_one_target, proj);
    
    return proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


project_t *
read_buildfile(const char *filename, project_t *parent)
{
    xmlDoc *doc;
    xmlNode *root;
    project_t *proj;
    
#if DEBUG
    fprintf(stderr, "Reading file \"%s\"\n", filename);
#endif

    if ((doc = cantXmlParseFile(filename)) == 0)
    {
    	/* TODO: print error message */
	logf("Failed to load file \"%s\"\n", filename);
	return 0;
    }
        
    num_errs = 0;
    root = xmlDocGetRootElement(doc);

    if (root == 0)
    {
    	parse_node_error(0, "No elements in buildfile\n");
	xmlFreeDoc(doc);
	cantXmlNodeInfoClear();
	return 0;
    }
    
    if (strcmp(root->name, (parent == 0 ? "globals" : "project")))
    {
    	parse_error_unexpected_element(root);
	xmlFreeDoc(doc);
	cantXmlNodeInfoClear();
	return 0;
    }

    proj = parse_project(root, parent);
    if (proj != 0)
	project_set_filename(proj, filename);
    
    if (num_errs > 0)
    {
    	logf("%s: found %d errors\n", filename, num_errs);
	project_delete(proj);
	xmlFreeDoc(doc);
	cantXmlNodeInfoClear();
	return 0;
    }
	
    xmlFreeDoc(doc);
    cantXmlNodeInfoClear();
    
    return proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
