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

#define _DEFINE_GLOBALS 1
#include "cant.h"
#include "job.h"

CVSID("$Id: cant.c,v 1.11 2001-11-14 10:59:03 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean find_flag = FALSE;
static props_t *command_defines;   /* -Dfoo=bar on the commandline */
static GList *command_targets;     /* targets specified on the commandline */
static char *buildfile = "build.xml";
static unsigned parallelism = 1;
static char *globals_file = PKGDATADIR "/globals.xml";

static project_t *project_globals;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#define PATTERN_TEST 0
#if PATTERN_TEST

static void
test1(
    const char *pattern,
    unsigned flags,
    const char *filename,
    const char *rep)
{
    pattern_t *pat;

    pat = pattern_new(pattern, flags);

    if (pattern_match(pat, filename) && rep != 0)
    	g_free(pattern_replace(pat, rep));

    pattern_delete(pat);
}

static void
hack_pattern_test(void)
{
    test1("**/*.c", PAT_GROUPS, "foo/bar/baz.c", "X\\1Y\\2Z");
    test1("^([a-f][n-q]+)/.*/([^/]*\\.c)$", PAT_GROUPS|PAT_REGEXP, "foo/bar/baz.c", "X\\1Y\\2Z");
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#define MAPPER_TEST 0
#if MAPPER_TEST

static void
test1(
    const char *name,
    const char *from,
    const char *to)
{
    mapper_t *ma;
    const char **p;
    static const char *samples[] =
    {
    	"A.java",
	"foo/bar/B.java", 
	"C.properties",
	"Classes/dir/dir2/A.properties",
	0
    };

    fprintf(stderr, "================\n");
    ma = mapper_new(name, from, to);

    for (p = samples ; *p ; p++)
    	g_free(mapper_map(ma, *p));
    
    mapper_delete(ma);
}

static void
hack_mapper_test(void)
{
    test1("identity", 0, 0);
    test1("flatten", 0, 0);
    test1("merge", 0, "archive.tar");
    test1("glob", "*.java", "*.java.bak");
    test1("glob", "C*ies", "Q*y");
    test1("regexp", "^(.*)\\.java$", "\\1.java.bak");
    test1("regexp", "^(.*)/([^/]+)/([^/]*)$", "\\1/\\2/\\2-\\3");
    test1("regexp", "^(.*)\\.(.*)$", "\\2.\\1");
    test1("null", 0, 0);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fatal(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    fprintf(stderr, "%s: ", argv0);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    exit(1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
find_buildfile(void)
{
    char *dir;
    char *bf;
    char *x;
    struct stat sb;
    
    dir = g_get_current_dir();
    
    for (;;)
    {
    	bf = g_strconcat(dir, "/", buildfile, 0);
	
#if DEBUG
    	fprintf(stderr, "find_buildfile: Trying \"%s\"\n", bf);
#endif
	
	if (stat(bf, &sb) == 0)
	{
	    g_free(dir);
	    buildfile = bf;
#if DEBUG
    	    fprintf(stderr, "find_buildfile: Found \"%s\"\n", buildfile);
#endif
	    return TRUE;
	}
	g_free(bf);
	
	if (dir[1] == '\0')
	    break;  	    /* got to root, still can't find */
	
	/* chop off the last part of the directory */
	if ((x = strrchr(dir, '/')) == 0)
	    break;  	/* JIC */
	*x = '\0';
    }
    
    g_free(dir);
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
build_target_by_name(project_t *proj, const char *name)
{
    target_t *targ;
    
    if ((targ = project_find_target(proj, name)) == 0)
    {
    	fprintf(stderr, "%s: no such target \"%s\"\n", argv0, name);
    	return FALSE;
    }
    return target_execute(targ);
}    
    
static gboolean
build_commandline_targets(project_t *proj)
{
    GList *iter;
    
    if (command_targets == 0)
    	return build_target_by_name(proj, proj->default_target);
	
    for (iter = command_targets ; iter != 0 ; iter = iter->next)
    {
    	if (!build_target_by_name(proj, (const char *)iter->data))
	    return FALSE;
    }
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char usage_str[] = 
"Usage: cant [options] [target [target...]]\n"
" options are:\n"
"-find              search in parent directories for build.xml\n"
"-buildfile FILE    specify build file (default \"build.xml\")\n"
"-Dname=value       override property \"name\"\n"
"-jN                set N-way parallelism for compilation (default 1)\n"
;

static void
usage(int ec)
{
    fputs(usage_str, stderr);
    fflush(stderr); /* JIC */
    
    exit(ec);
}

static void
usagef(int ec, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    fprintf(stderr, "%s: ", argv0);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    usage(ec);
}


static void
add_commandline_define(const char *pair)
{
    char *buf, *x;
    
    if (pair == 0 || *pair == '*')
    	usagef(1, "No argument to -D\n");
	
    buf = g_strdup(pair);

    x = strchr(buf, '=');
    if (x == 0)
    	usagef(1, "Argument to -D should contain an \"=\"\n");    
    *x++ = '\0';
    
    if (command_defines == 0)
    	command_defines = props_new(0);
    props_set(command_defines, buf, x);
    
    g_free(buf);
}

static void
set_build_file(char *arg)
{
    if (arg == 0 || *arg == '\0')
	usagef(1, "Expecting filename argument for --build-file");
    buildfile = arg;
}

static void
set_globals_file(char *arg)
{
    if (arg == 0 || *arg == '\0')
	usagef(1, "Expecting filename argument for --globals-file");
    globals_file = arg;
}

static void
set_parallelism(const char *arg)
{
    if (arg == 0 || *arg == '\0' || (parallelism = atoi(arg)) < 1)
	parallelism = 1;
}

static void
parse_args(int argc, char **argv)
{
    int i;
    
    argv0 = argv[0];
    
    for (i = 1 ; i < argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    if (!strcmp(argv[i], "-find"))
	    {
	    	find_flag = TRUE;
	    }
	    else if (!strcmp(argv[i], "-buildfile") ||
	    	     !strcmp(argv[i], "--build-file"))
	    {
	    	set_build_file(argv[++i]);
	    }
	    else if (!strncmp(argv[i], "--build-file=", 13))
	    {
	    	set_build_file(argv[i]+13);
	    }
	    else if (!strcmp(argv[i], "-f"))
	    {
	    	set_build_file(argv[++i]);
	    }
	    else if (!strncmp(argv[i], "-D", 2))
	    {
	    	add_commandline_define(argv[i]+2);
	    }
	    else if (!strncmp(argv[i], "-j", 2))
	    {
	    	set_parallelism(argv[i]+2);
	    }
	    else if (!strncmp(argv[i], "--jobs=", 7))
	    {
	    	set_parallelism(argv[i]+7);
	    }
	    else if (!strncmp(argv[i], "--globals-file=", 15))
	    {
    	    	set_globals_file(argv[i]+15);
	    }
	    else if (!strcmp(argv[i], "--globals-file"))
	    {
	    	set_globals_file(argv[++i]);
	    }
	    else if (!strcmp(argv[i], "--help"))
	    {
	    	usage(0);
	    }
	    else
	    {
	    	usagef(1, "unknown option \"%s\"\n", argv[i]);
	    }
	}
	else
	{
	    command_targets = g_list_append(command_targets,
	    	    	    	    	    g_strdup(argv[i]));
	}
    }
    
    if (find_flag && strchr(buildfile, '/') != 0)
    	usagef(1, "When using -find, please specify only a filename for -buildfile");

#if DEBUG
    fprintf(stderr, "parse_args: find_flag = %d\n", find_flag);
    fprintf(stderr, "parse_args: buildfile = \"%s\"\n", buildfile);
    fprintf(stderr, "parse_args: globals file = \"%s\"\n", globals_file);
    {
    	GList *iter;
	fprintf(stderr, "parse_args: command_targets =");
	for (iter = command_targets ; iter != 0 ; iter = iter->next)
	    fprintf(stderr, " \"%s\"", (const char *)iter->data);
	fprintf(stderr, "\n");
    }
    
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if DEBUG

static void
dump_one_property(const char *name, const char *value, void *userdata)
{
    const char *spaces = (const char *)userdata;
    
    fprintf(stderr, "%s\"%s\"=\"%s\"\n", spaces, name, value);
}

static void
dump_one_target(gpointer key, gpointer value, gpointer userdata)
{
    target_t *targ = (target_t *)value;
    GList *iter;
    char *cond_desc;
    
    fprintf(stderr, "    TARGET {\n");
    fprintf(stderr, "        NAME=\"%s\"\n", targ->name);
    fprintf(stderr, "        DESCRIPTION=\"%s\"\n", targ->description);
    fprintf(stderr, "        FLAGS=\"%08x\"\n", targ->flags);
    cond_desc = condition_describe(&targ->condition);
    fprintf(stderr, "        CONDITION=%s\n", cond_desc);
    g_free(cond_desc);
    fprintf(stderr, "        DEPENDS {\n");
    for (iter = targ->depends ; iter != 0 ; iter = iter->next)
    {
    	target_t *dep = (target_t *)iter->data;
	fprintf(stderr, "            \"%s\"\n", dep->name);
    }
    fprintf(stderr, "        }\n");
    for (iter = targ->tasks ; iter != 0 ; iter = iter->next)
    {
    	task_t *task = (task_t *)iter->data;
	fprintf(stderr, "        TASK {\n");
	fprintf(stderr, "            NAME = \"%s\"\n", task->name);
	fprintf(stderr, "            ID = \"%s\"\n", task->id);
	fprintf(stderr, "        }\n");
    }
    fprintf(stderr, "    }\n");
}

static void
dump_project_properties(project_t *proj)
{
    fprintf(stderr, "    FIXED_PROPERTIES {\n");
    props_apply_local(proj->fixed_properties, dump_one_property, "        ");
    fprintf(stderr, "    }\n");
    fprintf(stderr, "    PROPERTIES {\n");
    props_apply_local(proj->properties, dump_one_property, "        ");
    fprintf(stderr, "    }\n");
}

static void
dump_project(project_t *proj)
{
    fprintf(stderr, "PROJECT {\n");
    fprintf(stderr, "    NAME=\"%s\"\n", proj->name);
    fprintf(stderr, "    DESCRIPTION=\"%s\"\n", proj->description);
    fprintf(stderr, "    DEFAULT=\"%s\"\n", proj->default_target);
    fprintf(stderr, "    BASEDIR=\"%s\"\n", proj->basedir);
    g_hash_table_foreach(proj->targets, dump_one_target, 0);
    dump_project_properties(proj);
    fprintf(stderr, "}\n");
}

#endif /* DEBUG */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    project_t *proj;
    int ret;
    
    parse_args(argc, argv);
    
    task_initialise_builtins();
    mapper_initialise_builtins();
    if (!job_init(parallelism))
    	return 1;

    project_globals = read_buildfile(globals_file, /*parent*/0);
    if (project_globals == 0)
    	fatal("Can't read globals file \"%s\"\n", globals_file);

    if (find_flag && !find_buildfile())
    	fatal("Can't find buildfile \"%s\" in any parent directory\n", buildfile);

    if ((proj = read_buildfile(buildfile, project_globals)) == 0)
    	fatal("Can't read buildfile \"%s\"\n", buildfile);

    if (command_defines != 0)
	project_override_properties(proj, command_defines);
    	
#if DEBUG
    dump_project(project_globals);
    dump_project(proj);
#endif
    
    ret = !build_commandline_targets(proj);
    
#if 0 /*DEBUG*/
    dump_project_properties(proj);
#endif

#if PATTERN_TEST
    hack_pattern_test();
#endif
#if MAPPER_TEST
    hack_mapper_test();
#endif

    project_delete(proj);
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
