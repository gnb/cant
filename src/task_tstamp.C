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
#include <time.h>

CVSID("$Id: task_tstamp.C,v 1.7 2002-04-13 09:26:06 gnb Exp $");

class tstamp_format_t
{
private:
    char *property_;
    char *cpattern_;   /* parsed into strptime() format */
    unsigned long offset_;
    enum
    {
    	OU_MILLI, OU_SECOND, OU_MINUTE, OU_HOUR, OU_DAY, OU_WEEK, OU_MONTH, OU_YEAR
    } offset_units_;
    char *locale_;
        
public:
    tstamp_format_t();
    ~tstamp_format_t();
    
    void set_property(const char *property);
    gboolean set_pattern(const char *pattern);
    gboolean set_offset(const char *offset_str, const char *unit_str);

    void execute(project_t *proj, struct tm *tm);    
};


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tstamp_format_t::tstamp_format_t()
{
}

tstamp_format_t::~tstamp_format_t()
{
    strdelete(property_);
    strdelete(cpattern_);
    strdelete(locale_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
tstamp_format_t::set_property(const char *property)
{
    strassign(property_, property);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *pattern_map[] = 
{
    "%",    	"%%",
    "dd",   	"%d",
    "d",   	"%d",
    "MMMM", 	"%B",
    "MMM", 	"%b",
    "MM", 	"%m",
    "yyyy",   	"%Y",
    "yy",   	"%y",
    "hh",   	"%I",
    "mm",   	"%M",
    "ss",   	"%S",
    "aa",   	"%P",
    0
};

gboolean
tstamp_format_t::set_pattern(const char *pattern)
{
    const char *s = pattern;
    estring cpattern;
    int i, l;
    
    while (*s)
    {
    	gboolean gotit = FALSE;
	
    	for (i = 0 ; pattern_map[i] ; i += 2)
	{
	    l = strlen(pattern_map[i]);
	    if (!strncmp(s, pattern_map[i], l))
	    {
	    	cpattern.append_string(pattern_map[i+1]);
		s += l;
		gotit = TRUE;
	    	break;
	    }
	}
	
	if (!gotit)
	    cpattern.append_char(*s++);
    }

#if DEBUG
    fprintf(stderr, "tstamp_format_t::set_pattern: \"%s\" -> \"%s\"\n",
    	    	    	pattern, cpattern.data());
#endif

    if (cpattern_ != 0)
    	g_free(cpattern_);
    cpattern_ = cpattern.take();

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
tstamp_format_t::set_offset(const char *offset_str, const char *unit_str)
{
    if (offset_str != 0 || unit_str != 0)
    {
    	log::errorf("Sorry, <tstamp> does not support \"offset\" or \"unit\"\n");
    	return FALSE;
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
tstamp_format_t::execute(project_t *proj, struct tm *tm)
{
    char buf[256];
    
    strftime(buf, sizeof(buf), cpattern_, tm);
    log::infof("%s=\"%s\"\n", property_, buf);
    proj->set_property(property_, buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class tstamp_task_t : public task_t
{
private:
    list_t<tstamp_format_t> formats_;
    static list_t<tstamp_format_t> builtins_;

public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tstamp_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
}

~tstamp_task_t()
{
    formats_.delete_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
tstamp_task_t::add_format(xml_node_t *node)
{
    tstamp_format_t *fmt;
    char *property;
    char *offset_str;
    char *unit_str;
    char *pattern;
    char *locale;
    gboolean ok;

    /*
     * property
     */
    property = node->get_attribute("property");
    if (property == 0)
    {
	node->error_required_attribute("property");
	return FALSE;
    }
    fmt = new tstamp_format_t;
    fmt->set_property(property);
    g_free(property);

    /*
     * pattern
     */
    pattern = node->get_attribute("pattern");
    if (pattern == 0)
    {
	node->error_required_attribute("pattern");
	delete fmt;
	return FALSE;
    }
    if (!fmt->set_pattern(pattern))
    {
    	log::errorf("Bad format in \"pattern\"\n");
	g_free(pattern);
	delete fmt;
	return FALSE;
    }
    g_free(pattern);
    
    /*
     * offset, unit
     */
    offset_str = node->get_attribute("offset");
    unit_str = node->get_attribute("unit");
    ok = TRUE;
    ok = fmt->set_offset(offset_str, unit_str);
    if (offset_str != 0)
    	g_free(offset_str);
    if (unit_str != 0)
    	g_free(unit_str);
    if (!ok)
    {
    	log::errorf("Bad format in \"offset\" or \"unit\"\n");
	delete fmt;
	return FALSE;
    }
    
    /*
     * locale
     */
    locale = node->get_attribute("locale");
    if (locale != 0)
    {
    	log::errorf("Sorry, <tstamp> does not support \"locale\"\n");
	delete fmt;
    	return FALSE;
    }
    
    formats_.append(fmt);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
exec()
{
    time_t clock;
    struct tm tm;
    list_iterator_t<tstamp_format_t> iter;

    time(&clock);
    tm = *localtime(&clock);	/* TODO: localtime_r */

    for (iter = builtins_.first() ; iter != 0 ; ++iter)
	(*iter)->execute(project_, &tm);

    for (iter = formats_.first() ; iter != 0 ; ++iter)
	(*iter)->execute(project_, &tm);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
initialise_builtins()
{
    tstamp_format_t *fmt;
    
    fmt = new tstamp_format_t();
    fmt->set_property("DSTAMP");
    fmt->set_pattern("yyyyMMdd");
    tstamp_task_t::builtins_.append(fmt);

    fmt = new tstamp_format_t();
    fmt->set_property("TSTAMP");
    fmt->set_pattern("hhmm");
    tstamp_task_t::builtins_.append(fmt);

    fmt = new tstamp_format_t();
    fmt->set_property("TODAY");
    fmt->set_pattern("MMMM dd yyyy");
    tstamp_task_t::builtins_.append(fmt);
}

static void
cleanup_builtins()
{
    builtins_.delete_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

list_t<tstamp_format_t> tstamp_task_t::builtins_;

static task_child_t tstamp_children[] = 
{
    TASK_CHILD(tstamp, format, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(tstamp,
			/*attrs*/0,
			tstamp_children,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/FALSE)

public:

void
init()
{
    tstamp_task_t::initialise_builtins();
    task_class_t::init();
}

void
cleanup()
{
    tstamp_task_t::cleanup_builtins();
    task_class_t::cleanup();
}

TASK_DEFINE_CLASS_END(tstamp)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
