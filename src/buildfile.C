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
#include "xtask.H"

CVSID("$Id: buildfile.C,v 1.7 2002-04-07 05:28:49 gnb Exp $");

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
    	    	cantXmlAttrName(attr), cantXmlNodeGetName(attr->node));
}

void
parse_error_required_attribute(const xmlNode *node, const char *attrname)
{
    parse_node_error(node, "Required attribute \"%s\" missing from \"%s\"\n",
    	    	attrname, cantXmlNodeGetName(node));
}

void
parse_error_unexpected_element(const xmlNode *node)
{
    parse_node_error(node, "Element \"%s\" unexpected at this point\n",
    	    	cantXmlNodeGetName(node));
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
    char *if_attr = cantXmlGetProp(node, "if");
    char *unless_attr = cantXmlGetProp(node, "unless");
    char *matches_attr = cantXmlGetProp(node, "matches");
    char *matchesregex_attr = cantXmlGetProp(node, "matchesregex");
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
	    cond->set_if(if_attr);
    	else if (unless_attr != 0)
	    cond->set_unless(unless_attr);
	    
	if (matches_attr != 0)
	    cond->set_matches(matches_attr, case_sens);
	else if (matchesregex_attr != 0)
	    cond->set_matches_regex(matchesregex_attr, case_sens);

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

static gboolean
parse_property(project_t *proj, xmlNode *node)
{
    char *name = 0;
    char *name_e = 0;
    xmlAttr *attr;
    gboolean failed = FALSE;
    condition_t cond;
    gboolean doit = TRUE;
    
    if (parse_condition(&cond, node))
	doit = cond.evaluate(project_get_props(proj));

    /* a lot hinges on whether attribute "name" is present */    
    name = cantXmlGetProp(node, "name");

    if (name != 0)
    {
	name_e = project_expand(proj, name);
	strnullnorm(name_e);
    }

    if (name != 0 && name_e == 0)
    {
	parse_node_error(node, "\"name\" expanded empty\n");
	failed = TRUE;
    }
    else if (name != 0)
    {
    	/* exactly one of "value", "location" and "refid" may be present */
	char *value = cantXmlGetProp(node, "value");
	char *location = cantXmlGetProp(node, "location");
	char *refid = cantXmlGetProp(node, "refid");
	int n = 0;
	
	if (value != 0) n++;
	if (location != 0) n++;
	if (refid != 0) n++;

	if (n != 1)
	{
	    parse_node_error(node, "You must specify exactly one of \"value\", \"location\", and \"refid\"\n");
	    failed = TRUE;
	}
    	else if (value != 0)
	{
	    if (doit)
	    {
		if (cantXmlGetBooleanProp(node, "append", FALSE))
		    project_append_property(proj, name_e, value);
		else
		    project_set_property(proj, name_e, value);
	    }
	}
	else if (location != 0)
	{
	    if (doit)
	    {
		/* TODO: translate DOS format filenames */
		if (location[0] == '/')
	    	    project_set_property(proj, name_e, location);
		else
		{
	    	    /* TODO: use file_normalise() */
	    	    char *abs = g_strconcat(proj->basedir, "/", location, 0);
	    	    project_set_property(proj, name_e, abs);
		    g_free(abs);
		}
	    }
	}
	else if (refid != 0)
	{
	    /* TODO */
	    parse_node_error(node, "<property refid=> not implemented\n");
	    failed = TRUE;
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
	char *resource = cantXmlGetProp(node, "resource");
	char *file = cantXmlGetProp(node, "file");
	char *shellfile = cantXmlGetProp(node, "shellfile");
	char *environment = cantXmlGetProp(node, "environment");
	int n = 0;
	
	if (resource != 0) n++;
	if (file != 0) n++;
	if (shellfile != 0) n++;
	if (environment != 0) n++;

	if (n != 1)
	{
	    parse_node_error(node, "You must specify exactly one of \"resource\", \"file\", \"shellfile\" and \"environment\"\n");
    	    failed = TRUE;
    	}
	else if (resource != 0)
	{
	    /* TODO */
	    parse_node_error(node, "<property resource=> not implemented\n");
	    failed = TRUE;
	}
	else if (file != 0)
	{
	    /* TODO */
	    parse_node_error(node, "<property file=> not implemented\n");
	    failed = TRUE;
	}
	else if (shellfile != 0)
	{
	    if (doit)
		failed = !proj->properties->read_shellfile(shellfile);
	}
	else if (environment != 0)
	{
	    /* TODO */
	    parse_node_error(node, "<property environment=> not implemented\n");
	    failed = TRUE;
	}
	
	if (resource != 0)
    	    xmlFree(resource);
	if (file != 0)
    	    xmlFree(file);
	if (shellfile != 0)
    	    xmlFree(shellfile);
	if (environment != 0)
    	    xmlFree(environment);
    }

    /* scan the attributes anyway, to detect misspellings etc */
    for (attr = node->properties ; attr != 0 ; attr = attr->next)
    {
    	char *value = cantXmlAttrValue(attr);
	
    	if (!strcmp(cantXmlAttrName(attr), "name") ||
	    !strcmp(cantXmlAttrName(attr), "value") ||
	    !strcmp(cantXmlAttrName(attr), "location") ||
	    !strcmp(cantXmlAttrName(attr), "append") ||
	    !strcmp(cantXmlAttrName(attr), "refid") ||
	    !strcmp(cantXmlAttrName(attr), "resource") ||
	    !strcmp(cantXmlAttrName(attr), "file") ||
	    !strcmp(cantXmlAttrName(attr), "shellfile") ||
	    !strcmp(cantXmlAttrName(attr), "environment") ||
	    !strcmp(cantXmlAttrName(attr), "classpath") ||
	    !strcmp(cantXmlAttrName(attr), "classpathref") ||
	    is_condition_attribute(cantXmlAttrName(attr)))
	    ;
	else
	{
	    parse_error_unknown_attribute(attr);
	    failed = TRUE;
	}

    	xmlFree(value);
    }
    

    if (name != 0)
    	xmlFree(name);
    strdelete(name_e);
	
    return !failed;
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

    if ((buf = cantXmlGetProp(node, "namespace")) == 0)
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

    if ((buf = cantXmlGetProp(node, "default")) != 0)
    {
    	tagexp_add_default_expansion(te, buf);
    	xmlFree(buf);
    }
    
    /* TODO: check no other attributes are present */
    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;

    	if (!strcmp(cantXmlNodeGetName(child), "expand"))
	{
	    char *tag = 0, *to = 0;
	    
	    if ((tag = cantXmlGetProp(child, "tag")) == 0)
	    {
	    	parse_error_required_attribute(child, "tag");
	    	failed = TRUE;
	    }
	    	    
	    if ((to = cantXmlGetProp(child, "to")) == 0)
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
    	else if (!strcmp(cantXmlNodeGetName(child), "default"))
	{
	    char *to = 0;
	    
	    if ((to = cantXmlGetProp(child, "to")) == 0)
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
    const char *name_space = cantXmlNodeGetName(node);
    

#if DEBUG
    fprintf(stderr, "Parsing taglist \"%s\"\n", name_space);
#endif

    if ((tldef = project_find_tl_def(proj, name_space)) == 0)
    {
    	parse_node_error(node, "Unknown taglist type \"%s\"\n", name_space);
	return 0;
    }
    
    if ((buf = cantXmlGetProp(node, "refid")) != 0)
    {
    	/* Handle reference to an existing taglist */
    	if ((buf2 = cantXmlGetProp(node, "id")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"id\" and \"refid\"\n");
	    xmlFree(buf);
	    xmlFree(buf2);
	    return 0;
	}
	
	if ((tl = project_find_taglist(proj, name_space, buf)) == 0)
	{
	    /* TODO: order dependency */
	    parse_node_error(node, "Cannot find taglist \"%s\" to satisfy \"refid\"\n", buf);
	    xmlFree(buf);
	    return 0;
	}
	/* TODO: refcount to prevent problems deleting the project */
	
    	xmlFree(buf);
	taglist_ref(tl);
	return tl;
    }

    /* actual taglist definition */
    if ((buf = cantXmlGetProp(node, "id")) == 0)
    {
	parse_error_required_attribute(node, "id");
	return 0;
    }

    tl = taglist_new(name_space);
    taglist_set_id(tl, buf);
    xmlFree(buf);
    
    /* TODO: parse optional "depends" attribute */
    /* TODO: syntax check other attributes */
    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;

    	if ((tag = tl_def_find_tag(tldef, cantXmlNodeGetName(child))) != 0)
	{
	    char *name = cantXmlGetProp(child, "name");
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

	    if ((value = cantXmlGetProp(child, "value")) != 0)
		tlitem = taglist_add_item(tl, cantXmlNodeGetName(child), name, TL_VALUE, value);
	    else if ((value = cantXmlGetProp(child, "line")) != 0)
		tlitem = taglist_add_item(tl, cantXmlNodeGetName(child), name, TL_LINE, value);
	    else if ((value = cantXmlGetProp(child, "file")) != 0)
		tlitem = taglist_add_item(tl, cantXmlNodeGetName(child), name, TL_FILE, value);
	    
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
    	taglist_unref(tl);
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
	
    if (project_find_taglist(proj, tl->name_space, tl->id) != 0)
    {
    	parse_node_error(node, "Duplicate taglist %s::%s\n", tl->name_space, tl->id);
	taglist_unref(tl);
	return FALSE;
    }

    project_add_taglist(proj, tl);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
parse_xtaskdef(project_t *proj, xmlNode *node)
{
    char *buf;
    xtask_class_t *xtclass;
    xmlNode *child;
    xtask_class_t::arg_t *xa;
    fileset_t *fs;
    gboolean failed = FALSE;
    
#if DEBUG
    fprintf(stderr, "Parsing xtaskdef\n");
#endif

    if ((buf = cantXmlGetProp(node, "name")) == 0)
    {
	parse_error_required_attribute(node, "name");
	return FALSE;
    }

    parse_node_push(node);
    xtclass = new xtask_class_t(buf);
    xmlFree(buf);

    buf = cantXmlGetProp(node, "executable");
    xtclass->set_executable(buf);
    xmlFree(buf);
    
    buf = cantXmlGetProp(node, "logmessage");
    xtclass->set_logmessage(buf);
    xmlFree(buf);

    xtclass->set_is_fileset(cantXmlGetBooleanProp(node, "fileset", FALSE));
    
    xtclass->set_fileset_dir_name("dir");	/* TODO */
    
    xtclass->set_foreach(cantXmlGetBooleanProp(node, "foreach", FALSE));
    
    buf = cantXmlGetProp(node, "deptarget");
    xtclass->set_dep_target(buf);
    xmlFree(buf);

    /* TODO: syntax check other attributes */
    
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;

    	parse_node_push(child);
    	xa = 0;
    	if (!strcmp(cantXmlNodeGetName(child), "arg"))
	{
	    if ((buf = cantXmlGetProp(child, "value")) != 0)
	    {
	    	xa = xtclass->add_value(buf);
	    	xmlFree(buf);
	    }
	    else if ((buf = cantXmlGetProp(child, "line")) != 0)
	    {
	    	xa = xtclass->add_line(buf);
	    	xmlFree(buf);
	    }
	    else if ((buf = cantXmlGetProp(child, "file")) != 0)
	    {
	    	xa = xtclass->add_file(buf);
	    	xmlFree(buf);
	    }
	    else
	    {
	    	parse_node_error(node, "One of \"line\", \"value\" or \"file\" must be set\n");
    	    	failed = TRUE;
	    }
	}
	else if (!strcmp(cantXmlNodeGetName(child), "attr"))
	{
	    char *from = 0, *to = 0;
	    
	    if ((from = cantXmlGetProp(child, "attribute")) == 0)
	    {
		parse_error_required_attribute(node, "attribute");
		failed = TRUE;
	    }
	    else if ((to = cantXmlGetProp(child, "property")) == 0)
	    	to = xmlMemStrdup(from);

    	    if (from != 0)
    	    	xtclass->add_attribute(from, to,
		    	cantXmlGetBooleanProp(child, "required", FALSE));
	    /* TODO: specify default value */
	    if (from != 0)
		xmlFree(from);
	    if (to != 0)
		xmlFree(to);
	}
	else if (!strcmp(cantXmlNodeGetName(child), "taglist"))
	{
	    char *name_space = 0;
	    
	    if ((name_space = cantXmlGetProp(child, "namespace")) == 0)
	    {
		parse_error_required_attribute(node, "namespace");
		failed = TRUE;
	    }

	    xtclass->add_child(name_space);
	    
	    if (name_space != 0)
		xmlFree(name_space);
    	}	
	else if (!strcmp(cantXmlNodeGetName(child), "fileset"))
	{
	    if ((fs = parse_fileset(proj, child, "dir")) == 0)
	    	failed = TRUE;
	    else
	    	xa = xtclass->add_fileset(fs);
	}
	else if (!strcmp(cantXmlNodeGetName(child), "files"))
	{
	    xa = xtclass->add_files();
	}
	else if (!strcmp(cantXmlNodeGetName(child), "tagexpand"))
	{
	    tagexp_t *te;
	    
	    if ((te = parse_tagexpand(proj, child)) == 0)
	    	failed = TRUE;
	    else
		xa = xtclass->add_tagexpand(te);
	}
	else if (!strcmp(cantXmlNodeGetName(child), "mapper"))
	{
	    mapper_t *ma;
	    
	    if ((ma = parse_mapper(proj, child)) == 0)
	    	failed = TRUE;
	    else
		xtclass->add_mapper(ma);
	}
	else if (!strcmp(cantXmlNodeGetName(child), "depmapper"))
	{
	    mapper_t *ma;
	    
	    if ((ma = parse_mapper(proj, child)) == 0)
	    	failed = TRUE;
	    else
		xtclass->add_dep_mapper(ma);
	}
	else
	{
	    parse_error_unexpected_element(child);
	    failed = TRUE;
	}

    	parse_node_pop();
    	if (xa == 0)
	    continue;

    	if (!parse_condition(&xa->condition, child))
	{
	    failed = TRUE;
	    continue;
	}
    }    

    parse_node_pop();
    if (failed)
    {
    	delete xtclass;
	return FALSE;
    }
    
    /* TODO: handle duplicate registrations cleanly */
    proj->tscope->add(xtclass);
    return TRUE;
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

    if ((val = cantXmlGetProp(node, attrname)) == 0)
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

    if ((buf = cantXmlGetProp(node, "name")) == 0)
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
    	if (!strcmp(cantXmlNodeGetName(child), "tag"))
	{
	    char *tag = 0;
	    availability_t name_avail = AV_OPTIONAL;
	    availability_t value_avail = AV_OPTIONAL;
	    
	    if ((tag = cantXmlGetProp(child, "tag")) == 0)
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
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t *
parse_fileset(project_t *proj, xmlNode *node, const char *dirprop)
{
    fileset_t *fs;
    fileset_t::spec_t *fss;
    char *buf, *buf2, *x;
    xmlNode *child;
    gboolean appending = FALSE;
    static const char sep[] = ", \t\n\r";

#if DEBUG
    fprintf(stderr, "parsing fileset\n");
#endif

    if ((buf = cantXmlGetProp(node, "refid")) != 0)
    {
    	/* Handle reference to an existing fileset */
    	if ((buf2 = cantXmlGetProp(node, "id")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"id\" and \"refid\"\n");
	    xmlFree(buf);
	    xmlFree(buf2);
	    return 0;
	}
    	if ((buf2 = cantXmlGetProp(node, "append")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"append\" and \"refid\"\n");
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
    	xmlFree(buf);
	fs->ref();
	return fs;
    }
    else if ((buf = cantXmlGetProp(node, "append")) != 0)
    {
    	/* Handle appending to an existing fileset */
    	if ((buf2 = cantXmlGetProp(node, "id")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"id\" and \"append\"\n");
	    xmlFree(buf);
	    xmlFree(buf2);
	    return 0;
	}
    	if ((buf2 = cantXmlGetProp(node, "refid")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"refid\" and \"append\"\n");
	    xmlFree(buf);
	    xmlFree(buf2);
	    return 0;
	}

    	buf2 = project_expand(proj, buf);
	if ((fs = project_find_fileset(proj, buf2)) == 0)
	{
	    /*
    	     * TODO: order dependency.... May need to scan the project
	     *       for non-tasks first before parsing tasks.
	     */
	    parse_node_error(node, "Cannot find fileset \"%s\" to satisfy \"append\"\n", buf);
	    xmlFree(buf);
	    return 0;
	}
    	xmlFree(buf);
	strdelete(buf2);
	appending = TRUE;
    }
    else
    {
	/* Handle definition of a new fileset */

	fs = new fileset_t();

	if (dirprop != 0 && (buf = cantXmlGetProp(node, dirprop)) != 0)
	{
	    fs->set_directory(buf);
	    xmlFree(buf);
    	}

	if ((buf = cantXmlGetProp(node, "id")) != 0)
	{
    	    fs->set_id(buf);
    	    xmlFree(buf);
	}


	fs->set_case_sensitive(
    	    		cantXmlGetBooleanProp(node, "casesensitive", TRUE));
	fs->set_default_excludes(
    	    		cantXmlGetBooleanProp(node, "defaultexcludes", TRUE));
    }

    /*
     * "includes" attribute
     * TODO: use tok_t
     */
    buf = buf2 = cantXmlGetProp(node, "includes");
    if (buf != 0)
	while ((x = strtok(buf2, sep)) != 0)
	{
	    buf2 = 0;
	    fs->add_include(x);
	}
    xmlFree(buf);

    /*
     * "includesfile" attribute
     */
    buf = cantXmlGetProp(node, "includesfile");
    if (buf != 0)
	fs->add_include_file(buf);
    xmlFree(buf);
    

    /*
     * "excludes" attribute
     * TODO: use tok_t
     */
    buf = buf2 = cantXmlGetProp(node, "excludes");
    if (buf != 0)
	while ((x = strtok(buf2, sep)) != 0)
	{
	    buf2 = 0;
	    fs->add_exclude(x);
	}
    xmlFree(buf);

    /*
     * "excludesfile" attribute
     */
    buf = cantXmlGetProp(node, "excludesfile");
    if (buf != 0)
	fs->add_exclude_file(buf);
    xmlFree(buf);
    
    /*
     * children
     */
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	    
	if ((buf = cantXmlGetProp(child, "name")) == 0)
	{
	    parse_error_required_attribute(node, "name");
	    continue;
	}
	
	fss = 0;
	if (!strcmp(cantXmlNodeGetName(child), "include"))
	    fss = fs->add_include(buf);
	else if (!strcmp(cantXmlNodeGetName(child), "includesfile"))
	    fss = fs->add_include_file(buf);
	else if (!strcmp(cantXmlNodeGetName(child), "exclude"))
	    fss = fs->add_exclude(buf);
	else if (!strcmp(cantXmlNodeGetName(child), "excludesfile"))
	    fss = fs->add_exclude_file(buf);
	    
	xmlFree(buf);
	if (fss == 0)
	    continue;

    	if (!parse_condition(&fss->condition_, child))
	    continue;	    /* TODO: do something more drastic!!! */
    }
    
    return fs;
}


static gboolean
parse_project_fileset(project_t *proj, xmlNode *node)
{
    fileset_t *fs;
    
    if ((fs = parse_fileset(proj, node, "dir")) == 0)
    	return FALSE;
	
    if (fs->id() == 0)
    {
    	parse_node_error(node, "<fileset> at project scope must have \"id\" attribute\n");
    	return FALSE;
    }
    /* TODO: detect duplicate fileset ids */
    project_add_fileset(proj, fs);
    return TRUE;
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
    	    !strcmp(attrname, "recursive") ||
    	    !strcmp(attrname, "refid"));
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
parse_path(project_t *proj, xmlNode *node)
{
#if DEBUG
    fprintf(stderr, "ignoring path\n");
#endif
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_t *
parse_mapper(project_t *proj, xmlNode *node)
{
    char *name;
    char *from;
    char *to;
    mapper_t *ma;
    
    if ((name = cantXmlGetProp(node, "name")) == 0)
    {
	parse_error_required_attribute(node, "name");
    	return 0;
    }

    if ((from = cantXmlGetProp(node, "from")) == 0)
    {
	parse_error_required_attribute(node, "from");
	xmlFree(name);
    	return 0;
    }

    if ((to = cantXmlGetProp(node, "to")) == 0)
    {
	parse_error_required_attribute(node, "to");
	xmlFree(name);
	xmlFree(from);
    	return 0;
    }
    
    ma = mapper_t::create(name, from, to);
    
    xmlFree(name);
    xmlFree(from);
    xmlFree(to);
    
    return ma;
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
	
    if ((value = cantXmlGetProp(rec->node, ta->name)) == 0)
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
    task_class_t *tclass;
    const task_child_t *tc;
    char *content;
    gboolean failed = FALSE;
    
#if DEBUG
    fprintf(stderr, "parse_task: parsing task \"%s\"\n", cantXmlNodeGetName(node));
#endif

    if ((tclass = proj->tscope->find(cantXmlNodeGetName(node))) == 0)
    {
    	parse_node_error(node, "Unknown task \"%s\"\n", cantXmlNodeGetName(node));
	return 0;
    }

    parse_node_push(node);
    
    task = tclass->create_task(proj);
    
    for (attr = node->properties ; attr != 0 ; attr = attr->next)
    {
    	char *value = cantXmlAttrValue(attr);
	
    	if (!strcmp(cantXmlAttrName(attr), "id"))
	    task->set_id(value);
    	else if (!strcmp(cantXmlAttrName(attr), "taskname"))
	    task->set_name(value);
    	else if (!strcmp(cantXmlAttrName(attr), "description"))
	    task->set_description(value);
	else if (tclass->is_fileset() &&
	    	 is_fileset_attribute(cantXmlAttrName(attr), tclass->fileset_dir_name()))
	    ;	/* parse_fileset will get it later */
	else if (!task->set_attribute(cantXmlAttrName(attr), value))
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
	tclass->attributes_apply(check_one_attribute, &rec);
	failed = rec.failed;
    }

    if (tclass->is_fileset())
    {
    	fileset_t *fs = parse_fileset(proj, node, tclass->fileset_dir_name());
	if (fs != 0)
	    task->set_fileset(fs);
	else
    	    failed = TRUE;
    }

    /* parse the child nodes */
    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	/* TODO: handle character data */

	if ((tc = tclass->find_child(cantXmlNodeGetName(child))) != 0)
	{
	    parse_node_push(child);
	    if (!(task->*tc->adder)(child))
		failed = TRUE;
	    parse_node_pop();
	    continue;
	}
	
	if (tclass->is_composite())
	{
	    task_t *subtask = parse_task(proj, child);
	    if (subtask == 0)
	    	failed = TRUE;
	    else
	    	task->add_subtask(subtask);
	    continue;
	}
	
	parse_error_unexpected_element(child);
	failed = TRUE;
    }
    
    /* TODO: scan for required children */
    
    /* handle text content */
    if ((content = cantXmlNodeGetContent(node)) != 0)
    {
    	if (!task->set_content(content))
	    failed = TRUE;
    	xmlFree(content);
    }

    /* call the task's post-parse function */    
    if (!failed && !task->post_parse())
	failed = TRUE;

    parse_node_pop();

    if (failed)
    {
    	delete task;
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
	
	target_add_depend(targ, dep);
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
    name = cantXmlGetProp(node, "name");
    
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
    	if (target_is_defined(targ))
	{
	    parse_node_error(node, "Target \"%s\" already defined\n", name);
	    xmlFree(name);
	    return FALSE;
	}
    }
    xmlFree(name);
    target_set_is_defined(targ, TRUE);
    
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
	
    	if (!strcmp(cantXmlAttrName(attr), "name"))
	    /* not an error */;
    	else if (!strcmp(cantXmlAttrName(attr), "description"))
	    target_set_description(targ, value);
    	else if (!strcmp(cantXmlAttrName(attr), "depends"))
	    add_depends(proj, targ, value);
    	else if (is_condition_attribute(cantXmlAttrName(attr)))
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
check_one_target(const char *key, target_t *targ, void *userdata)
{
    gboolean *failedp = (gboolean *)userdata;
    
    if (target_is_depended_on(targ) && !target_is_defined(targ))
    {
    	parse_node_error(0, "Target \"%s\" is depended on but never defined\n", targ->name);
	*failedp = TRUE;
    }
}

static project_t *
parse_project(xmlNode *node, project_t *parent)
{
    project_t *proj;
    xmlNode *child;
    gboolean globals = (parent == 0);
    gboolean failed = FALSE;
   
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

    	    if (!strcmp(cantXmlAttrName(attr), "name"))
		project_set_name(proj, value);
    	    else if (!strcmp(cantXmlAttrName(attr), "default"))
		project_set_default_target(proj, value);
    	    else if (!strcmp(cantXmlAttrName(attr), "basedir"))
		project_set_basedir(proj, value);
	    else
	    {
		parse_error_unknown_attribute(attr);
		failed = TRUE;
	    }

    	    xmlFree(value);
	}
	if (proj->default_target == 0)
	{
	    parse_error_required_attribute(node, "default");
	    failed = TRUE;
	}
    }
        

    for (child = node->childs ; child != 0 ; child = child->next)
    {
    	if (child->type != XML_ELEMENT_NODE)
	    continue;
	    
    	if (!strcmp(cantXmlNodeGetName(child), "property"))
	    failed |= !parse_property(proj, child);
	else if (!strcmp(cantXmlNodeGetName(child), "path"))
	    failed |= !parse_path(proj, child);
    	else if (!strcmp(cantXmlNodeGetName(child), "fileset"))
	    failed |= !parse_project_fileset(proj, child);
	else if (!strcmp(cantXmlNodeGetName(child), "xtaskdef"))
	    failed |= !parse_xtaskdef(proj, child);
    	else if (!strcmp(cantXmlNodeGetName(child), "taglistdef"))
	    failed |= !parse_taglistdef(proj, child);
	else if (!globals && !strcmp(cantXmlNodeGetName(child), "target"))
	    failed |= !parse_target(proj, child);
	else if (project_find_tl_def(proj, cantXmlNodeGetName(child)) != 0)
	    failed |= !parse_project_taglist(proj, child);
	else
	{
	    /* TODO: patternset */
	    parse_error_unexpected_element(child);
	    failed = TRUE;
	}
    }

    if (!globals)    
	proj->targets->foreach(check_one_target, &failed);
    
    if (failed)
    {
    	project_delete(proj);
    	proj = 0;
    }
    
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
    
    if (strcmp(cantXmlNodeGetName(root), (parent == 0 ? "globals" : "project")))
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
	if (proj != 0)
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
