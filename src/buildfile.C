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

CVSID("$Id: buildfile.C,v 1.10 2002-04-12 14:28:21 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
parse_node_error(const xml_node_t *node, const char *fmt, ...)
{
    va_list args;
    log_node_context_t context(node);
    
    va_start(args, fmt);
    log::messagev(log::ERROR, fmt, args);
    va_end(args);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Parse the condition-related attributes on a node
 * into the condition_t and return TRUE iff the
 * parse was successful.
 */
 
gboolean
parse_condition(condition_t *cond, xml_node_t *node)
{
    char *if_attr = node->get_attribute("if");
    char *unless_attr = node->get_attribute("unless");
    char *matches_attr = node->get_attribute("matches");
    char *matchesregex_attr = node->get_attribute("matchesregex");
    gboolean case_sens = node->get_boolean_attribute("matchsensitive", TRUE);
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
    
    g_free(if_attr);
    g_free(unless_attr);
    g_free(matches_attr);
    g_free(matchesregex_attr);
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
parse_property(project_t *proj, xml_node_t *node)
{
    char *name = 0;
    char *name_e = 0;
    xml_iterator_t<xml_attribute_t> aiter;
    gboolean failed = FALSE;
    condition_t cond;
    gboolean doit = TRUE;
    
    if (parse_condition(&cond, node))
	doit = cond.evaluate(project_get_props(proj));

    /* a lot hinges on whether attribute "name" is present */    
    name = node->get_attribute("name");

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
	char *value = node->get_attribute("value");
	char *location = node->get_attribute("location");
	char *refid = node->get_attribute("refid");
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
		if (node->get_boolean_attribute("append", FALSE))
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
    	    g_free(value);
	if (location != 0)
    	    g_free(location);
	if (refid != 0)
    	    g_free(refid);
    }
    else
    {
    	/* exactly one of "resource", "file" and "environment" may be present */
	char *resource = node->get_attribute("resource");
	char *file = node->get_attribute("file");
	char *shellfile = node->get_attribute("shellfile");
	char *environment = node->get_attribute("environment");
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
    	    g_free(resource);
	if (file != 0)
    	    g_free(file);
	if (shellfile != 0)
    	    g_free(shellfile);
	if (environment != 0)
    	    g_free(environment);
    }

    /* scan the attributes anyway, to detect misspellings etc */
    for (aiter = node->first_attribute() ; aiter != 0 ; ++aiter)
    {
	xml_attribute_t *attr = *aiter;
    	char *value = attr->get_value();
	
    	if (!strcmp(attr->get_name(), "name") ||
	    !strcmp(attr->get_name(), "value") ||
	    !strcmp(attr->get_name(), "location") ||
	    !strcmp(attr->get_name(), "append") ||
	    !strcmp(attr->get_name(), "refid") ||
	    !strcmp(attr->get_name(), "resource") ||
	    !strcmp(attr->get_name(), "file") ||
	    !strcmp(attr->get_name(), "shellfile") ||
	    !strcmp(attr->get_name(), "environment") ||
	    !strcmp(attr->get_name(), "classpath") ||
	    !strcmp(attr->get_name(), "classpathref") ||
	    is_condition_attribute(attr->get_name()))
	    ;
	else
	{
	    attr->error_unknown_attribute();
	    failed = TRUE;
	}

    	g_free(value);
    }
    

    if (name != 0)
    	g_free(name);
    strdelete(name_e);
	
    return !failed;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
check_childless(xml_node_t *node)
{
    xml_iterator_t<xml_node_t> iter;
    gboolean res = TRUE;

    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() != XML_ELEMENT_NODE)
	    continue;

    	child->error_unexpected_element();
	res = FALSE;
    }
    
    return res;
}


static tagexp_t *
parse_tagexpand(project_t *proj, xml_node_t *node)
{
    char *buf;
    tagexp_t *te;
    xml_iterator_t<xml_node_t> iter;
    gboolean failed = FALSE;

#if DEBUG
    fprintf(stderr, "Parsing tagexpand\n");
#endif

    if ((buf = node->get_attribute("namespace")) == 0)
    {
	node->error_required_attribute("namespace");
	return 0;
    }
    
    te = new tagexp_t(buf);
    g_free(buf);

#if 0 /* TODO */
    te->set_follow_depends(node->get_boolean_attribute("followdepends", FALSE));
    te->set_reverse_order(node->get_boolean_attribute("reverse", FALSE));
#endif

    if ((buf = node->get_attribute("default")) != 0)
    {
    	te->add_default_expansion(buf);
    	g_free(buf);
    }
    
    /* TODO: check no other attributes are present */
    
    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() != XML_ELEMENT_NODE)
	    continue;

    	if (!strcmp(child->get_name(), "expand"))
	{
	    char *tag = 0, *to = 0;
	    
	    if ((tag = child->get_attribute("tag")) == 0)
	    {
	    	child->error_required_attribute("tag");
	    	failed = TRUE;
	    }
	    	    
	    if ((to = child->get_attribute("to")) == 0)
	    {
	    	child->error_required_attribute("to");
	    	failed = TRUE;
	    }
	    
	    /* TODO: check no other attributes are present */
	    
    	    if (tag != 0 && to != 0)
	    	te->add_expansion(tag, to);

	    if (!check_childless(child))
	    	failed = TRUE;
	    
	    if (tag != 0)
	    	g_free(tag);
	    if (to != 0)
	    	g_free(to);
	}
    	else if (!strcmp(child->get_name(), "default"))
	{
	    char *to = 0;
	    
	    if ((to = child->get_attribute("to")) == 0)
	    {
	    	child->error_required_attribute("to");
	    	failed = TRUE;
	    }
	    
	    /* TODO: check no other attributes are present */
	    
    	    if (to != 0)
	    	te->add_default_expansion(to);

	    if (!check_childless(child))
	    	failed = TRUE;
	    
	    if (to != 0)
	    	g_free(to);
	}
	else
	{
	    child->error_unexpected_element();
	    failed = TRUE;
	}
    }
    
    
    if (failed)
    {
    	delete te;
	te = 0;
    }
        
    return te;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

taglist_t *
parse_taglist(project_t *proj, xml_node_t *node)
{
    char *buf, *buf2;
    tl_def_t *tldef;
    taglist_t *tl;
    const tl_def_t::tag_t *tag;
    xml_iterator_t<xml_node_t> iter;
    gboolean failed = FALSE;
    const char *name_space = node->get_name();
    

#if DEBUG
    fprintf(stderr, "Parsing taglist \"%s\"\n", name_space);
#endif

    if ((tldef = project_find_tl_def(proj, name_space)) == 0)
    {
    	parse_node_error(node, "Unknown taglist type \"%s\"\n", name_space);
	return 0;
    }
    
    if ((buf = node->get_attribute("refid")) != 0)
    {
    	/* Handle reference to an existing taglist */
    	if ((buf2 = node->get_attribute("id")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"id\" and \"refid\"\n");
	    g_free(buf);
	    g_free(buf2);
	    return 0;
	}
	
	if ((tl = project_find_taglist(proj, name_space, buf)) == 0)
	{
	    /* TODO: order dependency */
	    parse_node_error(node, "Cannot find taglist \"%s\" to satisfy \"refid\"\n", buf);
	    g_free(buf);
	    return 0;
	}
	/* TODO: refcount to prevent problems deleting the project */
	
    	g_free(buf);
	tl->ref();
	return tl;
    }

    /* actual taglist definition */
    if ((buf = node->get_attribute("id")) == 0)
    {
	node->error_required_attribute("id");
	return 0;
    }

    tl = new taglist_t(name_space);
    tl->set_id(buf);
    g_free(buf);
    
    /* TODO: parse optional "depends" attribute */
    /* TODO: syntax check other attributes */
    
    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() != XML_ELEMENT_NODE)
	    continue;

    	if ((tag = tldef->find_tag(child->get_name())) != 0)
	{
	    char *name = child->get_attribute("name");
	    char *value = 0;
	    taglist_t::item_t *tlitem = 0;

	    if (name == 0 && tag->name_avail_ == AV_MANDATORY)
	    {
		child->error_required_attribute("name");
		failed = TRUE;
	    }
	    else if (name != 0 && tag->name_avail_ == AV_FORBIDDEN)
	    {
	    	parse_node_error(child, "Attribute \"name\" not allowed at this point\n");
	    	failed = TRUE;
	    }

	    if ((value = child->get_attribute("value")) != 0)
		tlitem = tl->add_value(child->get_name(), name, value);
	    else if ((value = child->get_attribute("line")) != 0)
		tlitem = tl->add_line(child->get_name(), name, value);
	    else if ((value = child->get_attribute("file")) != 0)
		tlitem = tl->add_file(child->get_name(), name, value);
	    
	    if (value == 0 && tag->value_avail_ == AV_MANDATORY)
	    {
		parse_node_error(child, "One of \"value\", \"line\" or \"file\" must be present\n");
		failed = TRUE;
	    }
	    else if (value != 0 && tag->value_avail_ == AV_FORBIDDEN)
	    {
	    	parse_node_error(child, "None of \"value\", \"line\" or \"file\" may be present\n");
	    	failed = TRUE;
	    }

    	    if (tlitem != 0 && !parse_condition(&tlitem->condition_, child))
	    	failed = TRUE;

	    if (name != 0)
		g_free(name);
	    if (value != 0)
		g_free(value);
    	}
	else
	{
	    child->error_unexpected_element();
	    failed = FALSE;
	}
    }
    
    
    if (failed)
    {
    	tl->unref();	// should delete
	tl = 0;
    }
        
    return tl;
}

static gboolean
parse_project_taglist(project_t *proj, xml_node_t *node)
{
    taglist_t *tl;
    
    if ((tl = parse_taglist(proj, node)) == 0)
    	return FALSE;
	
    if (project_find_taglist(proj, tl->name_space(), tl->id()) != 0)
    {
    	parse_node_error(node, "Duplicate taglist %s::%s\n", tl->name_space(), tl->id());
	tl->unref();	    // should delete
	return FALSE;
    }

    project_add_taglist(proj, tl);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
parse_xtaskdef(project_t *proj, xml_node_t *node)
{
    char *buf;
    xtask_class_t *xtclass;
    xml_iterator_t<xml_node_t> iter;
    xtask_class_t::arg_t *xa;
    fileset_t *fs;
    gboolean failed = FALSE;
    
#if DEBUG
    fprintf(stderr, "Parsing xtaskdef\n");
#endif

    if ((buf = node->get_attribute("name")) == 0)
    {
	node->error_required_attribute("name");
	return FALSE;
    }

    log_node_context_t context(node);
    xtclass = new xtask_class_t(buf);
    g_free(buf);

    buf = node->get_attribute("executable");
    xtclass->set_executable(buf);
    g_free(buf);
    
    buf = node->get_attribute("logmessage");
    xtclass->set_logmessage(buf);
    g_free(buf);

    xtclass->set_is_fileset(node->get_boolean_attribute("fileset", FALSE));
    
    xtclass->set_fileset_dir_name("dir");	/* TODO */
    
    xtclass->set_foreach(node->get_boolean_attribute("foreach", FALSE));
    
    buf = node->get_attribute("deptarget");
    xtclass->set_dep_target(buf);
    g_free(buf);

    /* TODO: syntax check other attributes */
    
    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() != XML_ELEMENT_NODE)
	    continue;

	log_node_context_t context(node);
    	xa = 0;
    	if (!strcmp(child->get_name(), "arg"))
	{
	    if ((buf = child->get_attribute("value")) != 0)
	    {
	    	xa = xtclass->add_value(buf);
	    	g_free(buf);
	    }
	    else if ((buf = child->get_attribute("line")) != 0)
	    {
	    	xa = xtclass->add_line(buf);
	    	g_free(buf);
	    }
	    else if ((buf = child->get_attribute("file")) != 0)
	    {
	    	xa = xtclass->add_file(buf);
	    	g_free(buf);
	    }
	    else
	    {
	    	parse_node_error(node, "One of \"line\", \"value\" or \"file\" must be set\n");
    	    	failed = TRUE;
	    }
	}
	else if (!strcmp(child->get_name(), "attr"))
	{
	    char *from = 0, *to = 0;
	    
	    if ((from = child->get_attribute("attribute")) == 0)
	    {
		node->error_required_attribute("attribute");
		failed = TRUE;
	    }
	    else if ((to = child->get_attribute("property")) == 0)
	    	to = xmlMemStrdup(from);

    	    if (from != 0)
    	    	xtclass->add_attribute(from, to,
		    	child->get_boolean_attribute("required", FALSE));
	    /* TODO: specify default value */
	    if (from != 0)
		g_free(from);
	    if (to != 0)
		g_free(to);
	}
	else if (!strcmp(child->get_name(), "taglist"))
	{
	    char *name_space = 0;
	    
	    if ((name_space = child->get_attribute("namespace")) == 0)
	    {
		node->error_required_attribute("namespace");
		failed = TRUE;
	    }

	    xtclass->add_child(name_space);
	    
	    if (name_space != 0)
		g_free(name_space);
    	}	
	else if (!strcmp(child->get_name(), "fileset"))
	{
	    if ((fs = parse_fileset(proj, child, "dir")) == 0)
	    	failed = TRUE;
	    else
	    	xa = xtclass->add_fileset(fs);
	}
	else if (!strcmp(child->get_name(), "files"))
	{
	    xa = xtclass->add_files();
	}
	else if (!strcmp(child->get_name(), "tagexpand"))
	{
	    tagexp_t *te;
	    
	    if ((te = parse_tagexpand(proj, child)) == 0)
	    	failed = TRUE;
	    else
		xa = xtclass->add_tagexpand(te);
	}
	else if (!strcmp(child->get_name(), "mapper"))
	{
	    mapper_t *ma;
	    
	    if ((ma = parse_mapper(proj, child)) == 0)
	    	failed = TRUE;
	    else
		xtclass->add_mapper(ma);
	}
	else if (!strcmp(child->get_name(), "depmapper"))
	{
	    mapper_t *ma;
	    
	    if ((ma = parse_mapper(proj, child)) == 0)
	    	failed = TRUE;
	    else
		xtclass->add_dep_mapper(ma);
	}
	else
	{
	    child->error_unexpected_element();
	    failed = TRUE;
	}

    	if (xa == 0)
	    continue;

    	if (!parse_condition(&xa->condition, child))
	{
	    failed = TRUE;
	    continue;
	}
    }    

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
    xml_node_t *node,
    const char *attrname,
    availability_t *availp)
{
    char *val;
    
    /* default value */
    *availp = AV_OPTIONAL;

    if ((val = node->get_attribute(attrname)) == 0)
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
    	g_free(val);
    	return FALSE;
    }
	
    g_free(val);
    return TRUE;
}


static gboolean
parse_taglistdef(project_t *proj, xml_node_t *node)
{
    char *buf;
    tl_def_t *tldef;
    xml_iterator_t<xml_node_t> iter;
    gboolean failed = FALSE;
    
#if DEBUG
    fprintf(stderr, "Parsing taglistdef\n");
#endif

    if ((buf = node->get_attribute("name")) == 0)
    {
	node->error_required_attribute("name");
	return FALSE;
    }

    tldef = new tl_def_t(buf);
    g_free(buf);

    /* TODO: syntax check other attributes */
    
    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() != XML_ELEMENT_NODE)
	    continue;

    	if (!strcmp(child->get_name(), "tag"))
	{
	    char *tag = 0;
	    availability_t name_avail = AV_OPTIONAL;
	    availability_t value_avail = AV_OPTIONAL;
	    
	    if ((tag = child->get_attribute("tag")) == 0)
	    {
	    	child->error_required_attribute("tag");
	    	failed = TRUE;
	    }
	    
	    if (!parse_availability_attr(child, "name", &name_avail))
	    	failed = TRUE;
	    if (!parse_availability_attr(child, "value", &value_avail))
	    	failed = TRUE;

    	    if (tag != 0)	    
	    {
		tldef->add_tag(tag, name_avail, value_avail);
		g_free(tag);
	    }
	}
	else
	    child->error_unexpected_element();
    }    

    /* TODO: handle duplicate registrations cleanly */
    project_add_tl_def(proj, tldef);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileset_t *
parse_fileset(project_t *proj, xml_node_t *node, const char *dirprop)
{
    fileset_t *fs;
    fileset_t::spec_t *fss;
    char *buf, *buf2, *x;
    xml_iterator_t<xml_node_t> iter;
    gboolean appending = FALSE;
    static const char sep[] = ", \t\n\r";

#if DEBUG
    fprintf(stderr, "parsing fileset\n");
#endif

    if ((buf = node->get_attribute("refid")) != 0)
    {
    	/* Handle reference to an existing fileset */
    	if ((buf2 = node->get_attribute("id")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"id\" and \"refid\"\n");
	    g_free(buf);
	    g_free(buf2);
	    return 0;
	}
    	if ((buf2 = node->get_attribute("append")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"append\" and \"refid\"\n");
	    g_free(buf);
	    g_free(buf2);
	    return 0;
	}
	
	if ((fs = project_find_fileset(proj, buf)) == 0)
	{
	    /*
    	     * TODO: order dependency.... May need to scan the project
	     *       for non-tasks first before parsing tasks.
	     */
	    parse_node_error(node, "Cannot find fileset \"%s\" to satisfy \"refid\"\n", buf);
	    g_free(buf);
	    return 0;
	}
    	g_free(buf);
	fs->ref();
	return fs;
    }
    else if ((buf = node->get_attribute("append")) != 0)
    {
    	/* Handle appending to an existing fileset */
    	if ((buf2 = node->get_attribute("id")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"id\" and \"append\"\n");
	    g_free(buf);
	    g_free(buf2);
	    return 0;
	}
    	if ((buf2 = node->get_attribute("refid")) != 0)
	{
	    parse_node_error(node, "Cannot specify both \"refid\" and \"append\"\n");
	    g_free(buf);
	    g_free(buf2);
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
	    g_free(buf);
	    return 0;
	}
    	g_free(buf);
	strdelete(buf2);
	appending = TRUE;
    }
    else
    {
	/* Handle definition of a new fileset */

	fs = new fileset_t();

	if (dirprop != 0 && (buf = node->get_attribute(dirprop)) != 0)
	{
	    fs->set_directory(buf);
	    g_free(buf);
    	}

	if ((buf = node->get_attribute("id")) != 0)
	{
    	    fs->set_id(buf);
    	    g_free(buf);
	}


	fs->set_case_sensitive(
    	    		node->get_boolean_attribute("casesensitive", TRUE));
	fs->set_default_excludes(
    	    		node->get_boolean_attribute("defaultexcludes", TRUE));
    }

    /*
     * "includes" attribute
     * TODO: use tok_t
     */
    buf = buf2 = node->get_attribute("includes");
    if (buf != 0)
	while ((x = strtok(buf2, sep)) != 0)
	{
	    buf2 = 0;
	    fs->add_include(x);
	}
    g_free(buf);

    /*
     * "includesfile" attribute
     */
    buf = node->get_attribute("includesfile");
    if (buf != 0)
	fs->add_include_file(buf);
    g_free(buf);
    

    /*
     * "excludes" attribute
     * TODO: use tok_t
     */
    buf = buf2 = node->get_attribute("excludes");
    if (buf != 0)
	while ((x = strtok(buf2, sep)) != 0)
	{
	    buf2 = 0;
	    fs->add_exclude(x);
	}
    g_free(buf);

    /*
     * "excludesfile" attribute
     */
    buf = node->get_attribute("excludesfile");
    if (buf != 0)
	fs->add_exclude_file(buf);
    g_free(buf);
    
    /*
     * children
     */
    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() != XML_ELEMENT_NODE)
	    continue;
	    
	if ((buf = child->get_attribute("name")) == 0)
	{
	    node->error_required_attribute("name");
	    continue;
	}
	
	fss = 0;
	if (!strcmp(child->get_name(), "include"))
	    fss = fs->add_include(buf);
	else if (!strcmp(child->get_name(), "includesfile"))
	    fss = fs->add_include_file(buf);
	else if (!strcmp(child->get_name(), "exclude"))
	    fss = fs->add_exclude(buf);
	else if (!strcmp(child->get_name(), "excludesfile"))
	    fss = fs->add_exclude_file(buf);
	    
	g_free(buf);
	if (fss == 0)
	    continue;

    	if (!parse_condition(&fss->condition_, child))
	    continue;	    /* TODO: do something more drastic!!! */
    }
    
    return fs;
}


static gboolean
parse_project_fileset(project_t *proj, xml_node_t *node)
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
parse_path(project_t *proj, xml_node_t *node)
{
#if DEBUG
    fprintf(stderr, "ignoring path\n");
#endif
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mapper_t *
parse_mapper(project_t *proj, xml_node_t *node)
{
    char *name;
    char *from;
    char *to;
    mapper_t *ma;
    
    if ((name = node->get_attribute("name")) == 0)
    {
	node->error_required_attribute("name");
    	return 0;
    }

    if ((from = node->get_attribute("from")) == 0)
    {
	node->error_required_attribute("from");
	g_free(name);
    	return 0;
    }

    if ((to = node->get_attribute("to")) == 0)
    {
	node->error_required_attribute("to");
	g_free(name);
	g_free(from);
    	return 0;
    }
    
    ma = mapper_t::create(name, from, to);
    
    g_free(name);
    g_free(from);
    g_free(to);
    
    return ma;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    xml_node_t *node;
    gboolean failed;
} attr_check_rec_t;

static void
check_one_attribute(const task_attr_t *ta, void *userdata)
{
    attr_check_rec_t *rec = (attr_check_rec_t *)userdata;
    char *value;

    if (!(ta->flags & TT_REQUIRED))
    	return;
	
    if ((value = rec->node->get_attribute(ta->name)) == 0)
    {
	rec->node->error_required_attribute(ta->name);
	rec->failed = TRUE;
    }
    else
	g_free(value);
}

task_t *
parse_task(project_t *proj, xml_node_t *node)
{
    task_t *task;
    xml_iterator_t<xml_attribute_t> aiter;
    xml_iterator_t<xml_node_t> iter;
    task_class_t *tclass;
    const task_child_t *tc;
    char *content;
    gboolean failed = FALSE;
    
#if DEBUG
    fprintf(stderr, "parse_task: parsing task \"%s\"\n", node->get_name());
#endif

    if ((tclass = proj->tscope->find(node->get_name())) == 0)
    {
    	parse_node_error(node, "Unknown task \"%s\"\n", node->get_name());
	return 0;
    }

    log_node_context_t context(node);
    
    task = tclass->create_task(proj);
    
    for (aiter = node->first_attribute() ; aiter != 0 ; ++aiter)
    {
	xml_attribute_t *attr = *aiter;
    	char *value = attr->get_value();
	
    	if (!strcmp(attr->get_name(), "id"))
	    task->set_id(value);
    	else if (!strcmp(attr->get_name(), "taskname"))
	    task->set_name(value);
    	else if (!strcmp(attr->get_name(), "description"))
	    task->set_description(value);
	else if (tclass->is_fileset() &&
	    	 is_fileset_attribute(attr->get_name(), tclass->fileset_dir_name()))
	    ;	/* parse_fileset will get it later */
	else if (!task->set_attribute(attr->get_name(), value))
	{
	    attr->error_unknown_attribute();
	    failed = TRUE;
	}
    	g_free(value);
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
    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() != XML_ELEMENT_NODE)
	    continue;
	/* TODO: handle character data */

	if ((tc = tclass->find_child(child->get_name())) != 0)
	{
	    log_node_context_t context(node);
	    if (!(task->*tc->adder)(child))
		failed = TRUE;
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
	
	child->error_unexpected_element();
	failed = TRUE;
    }
    
    /* TODO: scan for required children */
    
    /* handle text content */
    if ((content = node->get_content()) != 0)
    {
    	if (!task->set_content(content))
	    failed = TRUE;
    	g_free(content);
    }

    /* call the task's post-parse function */    
    if (!failed && !task->post_parse())
	failed = TRUE;

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
parse_target(project_t *proj, xml_node_t *node)
{
    target_t *targ;
    char *name = 0;
    xml_iterator_t<xml_attribute_t> aiter;
    xml_iterator_t<xml_node_t> iter;
    
    
    /*
     * We need to detect the (illegal) case that the target
     * depends directly on itself, so we have to add it before
     * we've parsed all the attributes, but of course we need
     * to scan the attributes to get the name to add...
     */
    name = node->get_attribute("name");
    
    if (name == 0)
    {
	node->error_required_attribute("name");
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
	    g_free(name);
	    return FALSE;
	}
    }
    g_free(name);
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
    for (aiter = node->first_attribute() ; aiter != 0 ; ++aiter)
    {
	xml_attribute_t *attr = *aiter;
    	char *value = attr->get_value();
	
    	if (!strcmp(attr->get_name(), "name"))
	    /* not an error */;
    	else if (!strcmp(attr->get_name(), "description"))
	    target_set_description(targ, value);
    	else if (!strcmp(attr->get_name(), "depends"))
	    add_depends(proj, targ, value);
    	else if (is_condition_attribute(attr->get_name()))
	    ;
	else
	    attr->error_unknown_attribute();

    	g_free(value);
    }

    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() == XML_ELEMENT_NODE)
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
parse_project(xml_node_t *node, project_t *parent)
{
    project_t *proj;
    xml_iterator_t<xml_node_t> iter;
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
	xml_iterator_t<xml_attribute_t> aiter;
	
	for (aiter = node->first_attribute() ; aiter != 0 ; ++aiter)
	{
	    xml_attribute_t *attr = *aiter;
    	    char *value = attr->get_value();

    	    if (!strcmp(attr->get_name(), "name"))
		project_set_name(proj, value);
    	    else if (!strcmp(attr->get_name(), "default"))
		project_set_default_target(proj, value);
    	    else if (!strcmp(attr->get_name(), "basedir"))
		project_set_basedir(proj, value);
	    else
	    {
		attr->error_unknown_attribute();
		failed = TRUE;
	    }

    	    g_free(value);
	}
	if (proj->default_target == 0)
	{
	    node->error_required_attribute("default");
	    failed = TRUE;
	}
    }
        

    for (iter = node->first_child() ; iter != 0 ; ++iter)
    {
    	xml_node_t *child = *iter;
    	if (child->get_type() != XML_ELEMENT_NODE)
	    continue;
	    
    	if (!strcmp(child->get_name(), "property"))
	    failed |= !parse_property(proj, child);
	else if (!strcmp(child->get_name(), "path"))
	    failed |= !parse_path(proj, child);
    	else if (!strcmp(child->get_name(), "fileset"))
	    failed |= !parse_project_fileset(proj, child);
	else if (!strcmp(child->get_name(), "xtaskdef"))
	    failed |= !parse_xtaskdef(proj, child);
    	else if (!strcmp(child->get_name(), "taglistdef"))
	    failed |= !parse_taglistdef(proj, child);
	else if (!globals && !strcmp(child->get_name(), "target"))
	    failed |= !parse_target(proj, child);
	else if (project_find_tl_def(proj, child->get_name()) != 0)
	    failed |= !parse_project_taglist(proj, child);
	else
	{
	    /* TODO: patternset */
	    child->error_unexpected_element();
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
    xml_node_t *root;
    project_t *proj;
    
#if DEBUG
    fprintf(stderr, "Reading file \"%s\"\n", filename);
#endif

    if ((doc = cantXmlParseFile(filename)) == 0)
    {
    	/* TODO: print error message */
	log::errorf("Failed to load file \"%s\"\n", filename);
	return 0;
    }
        
    log::zero_message_counts();
    root = (xml_node_t *)xmlDocGetRootElement(doc);

    if (root == 0)
    {
    	parse_node_error(0, "No elements in buildfile\n");
	xmlFreeDoc(doc);
	xml_node_t::info_clear();
	return 0;
    }
    
    if (strcmp(root->get_name(), (parent == 0 ? "globals" : "project")))
    {
    	root->error_unexpected_element();
	xmlFreeDoc(doc);
	xml_node_t::info_clear();
	return 0;
    }

    proj = parse_project(root, parent);
    if (proj != 0)
	project_set_filename(proj, filename);
    
    if (log::message_count(log::ERROR) > 0)
    {
    	log::errorf("%s: found %d errors\n", filename,
	    	    	log::message_count(log::ERROR));
	if (proj != 0)
	    project_delete(proj);
	xmlFreeDoc(doc);
	xml_node_t::info_clear();
	return 0;
    }
	
    xmlFreeDoc(doc);
    xml_node_t::info_clear();
    
    return proj;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
