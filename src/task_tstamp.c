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
#include <time.h>

CVSID("$Id: task_tstamp.c,v 1.4 2001-11-08 04:13:35 gnb Exp $");

typedef struct
{
    char *property;
    char *cpattern;   /* parsed into strptime() format */
    unsigned long offset;
    enum
    {
    	OU_MILLI, OU_SECOND, OU_MINUTE, OU_HOUR, OU_DAY, OU_WEEK, OU_MONTH, OU_YEAR
    } offset_units;
    char *locale;
} tstamp_format_t;

static GList *tstamp_builtins;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static tstamp_format_t *
tstamp_format_new(void)
{
    tstamp_format_t *fmt;
    
    fmt = new(tstamp_format_t);
    
    return fmt;
}

static void
tstamp_format_delete(tstamp_format_t *fmt)
{
    strdelete(fmt->property);
    strdelete(fmt->cpattern);
    strdelete(fmt->locale);
    g_free(fmt);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
tstamp_format_set_property(tstamp_format_t *fmt, const char *property)
{
    strassign(fmt->property, property);
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

static gboolean
tstamp_format_set_pattern(tstamp_format_t *fmt, const char *pattern)
{
    const char *s = pattern;
    estring cpattern;
    int i, l;
    
    estring_init(&cpattern);
    
    while (*s)
    {
    	gboolean gotit = FALSE;
	
    	for (i = 0 ; pattern_map[i] ; i += 2)
	{
	    l = strlen(pattern_map[i]);
	    if (!strncmp(s, pattern_map[i], l))
	    {
	    	estring_append_string(&cpattern, pattern_map[i+1]);
		s += l;
		gotit = TRUE;
	    	break;
	    }
	}
	
	if (!gotit)
	    estring_append_char(&cpattern, *s++);
    }

#if DEBUG
    fprintf(stderr, "tstamp_pattern_parse: \"%s\" -> \"%s\"\n",
    	    	    	pattern, cpattern.data);
#endif

    if (fmt->cpattern != 0)
    	g_free(fmt->cpattern);
    fmt->cpattern = cpattern.data;

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
tstamp_format_set_offset(
    tstamp_format_t *fmt,
    const char *offset_str,
    const char *unit_str)
{
    if (offset_str != 0 || unit_str != 0)
    {
    	fprintf(stderr, "%s: sorry, <tstamp> does not support \"offset\" or \"unit\"\n", argv0);
    	return FALSE;
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
tstamp_init(void)
{
    tstamp_format_t *fmt;
    
    fmt = tstamp_format_new();
    tstamp_format_set_property(fmt, "DSTAMP");
    tstamp_format_set_pattern(fmt, "yyyyMMdd");
    tstamp_builtins = g_list_append(tstamp_builtins, fmt);

    fmt = tstamp_format_new();
    tstamp_format_set_property(fmt, "TSTAMP");
    tstamp_format_set_pattern(fmt, "hhmm");
    tstamp_builtins = g_list_append(tstamp_builtins, fmt);

    fmt = tstamp_format_new();
    tstamp_format_set_property(fmt, "TODAY");
    tstamp_format_set_pattern(fmt, "MMMM dd yyyy");
    tstamp_builtins = g_list_append(tstamp_builtins, fmt);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
tstamp_new(task_t *task)
{
    task->private = 0;	/* GList of tstamp_format_t */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static tstamp_format_t *
tstamp_format_parse(task_t *task, xmlNode *node)
{
    tstamp_format_t *fmt;
    char *property;
    char *offset_str;
    char *unit_str;
    char *pattern;
    gboolean ok;

    /*
     * property
     */
    property = xmlGetProp(node, "property");
    if (property == 0)
    {
    	parse_error("Required attribute \"property\" missing\n");
	return 0;
    }
    fmt = tstamp_format_new();
    tstamp_format_set_property(fmt, property);
    xmlFree(property);

    /*
     * pattern
     */
    pattern = xmlGetProp(node, "pattern");
    if (pattern == 0)
    {
    	parse_error("Required attribute \"pattern\" missing\n");
	tstamp_format_delete(fmt);
	return 0;
    }
    if (!tstamp_format_set_pattern(fmt, pattern))
    {
    	parse_error("Bad format in \"pattern\"\n");
	xmlFree(pattern);
	tstamp_format_delete(fmt);
	return 0;
    }
    xmlFree(pattern);
    
    /*
     * offset, unit
     */
    offset_str = xmlGetProp(node, "offset");
    unit_str = xmlGetProp(node, "unit");
    ok = TRUE;
    ok = tstamp_format_set_offset(fmt, offset_str, unit_str);
    if (offset_str != 0)
    	xmlFree(offset_str);
    if (unit_str != 0)
    	xmlFree(unit_str);
    if (!ok)
    {
    	parse_error("Bad format in \"offset\" or \"unit\"\n");
	tstamp_format_delete(fmt);
	return 0;
    }
    
    /*
     * locale
     */
    fmt->locale = xml2g(xmlGetProp(node, "locale"));
    if (fmt->locale != 0)
    {
    	fprintf(stderr, "%s: sorry, <tstamp> does not support \"locale\"\n", argv0);
	tstamp_format_delete(fmt);
    	return 0;
    }
    
    return fmt;
}

static void
tstamp_add_format(task_t *task, xmlNode *node)
{
    GList *fmt_list = (GList *)task->private;
    tstamp_format_t *fmt;
    
    if ((fmt = tstamp_format_parse(task, node)) != 0)
	fmt_list = g_list_append(fmt_list, fmt);

    task->private = fmt_list;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
tstamp_execute_format_list(project_t *proj, struct tm *tm, GList *list)
{
    char buf[256];
    
    for ( ; list != 0 ; list = list->next)
    {
    	tstamp_format_t *fmt = (tstamp_format_t*)list->data;
	
	strftime(buf, sizeof(buf), fmt->cpattern, tm);
    	logf("%s=\"%s\"\n", fmt->property, buf);
	project_set_property(proj, fmt->property, buf);
    }
}

static gboolean
tstamp_execute(task_t *task)
{
    time_t clock;
    struct tm tm;
    project_t *proj = task->target->project;

    time(&clock);
    tm = *localtime(&clock);	/* TODO: localtime_r */
    
    tstamp_execute_format_list(proj, &tm, tstamp_builtins);
    tstamp_execute_format_list(proj, &tm, (GList *)task->private);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
tstamp_delete(task_t *task)
{
    GList *fmt_list = (GList *)task->private;
    
    while (fmt_list != 0)
    {
    	tstamp_format_delete((tstamp_format_t *)fmt_list->data);
	fmt_list = g_list_remove_link(fmt_list, fmt_list);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static task_child_t tstamp_children[] = 
{
    TASK_CHILD(tstamp, format, 0),
    {0}
};

task_ops_t tstamp_ops = 
{
    "tstamp",
    tstamp_init,
    tstamp_new,
    /*post_parse*/0,
    tstamp_execute,
    tstamp_delete,
    /*attrs*/0,
    /*children*/tstamp_children,
    /*is_fileset*/FALSE,
    /*fileset_dir_name*/0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
