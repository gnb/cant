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
#include "cant.H"
#include "job.H"

CVSID("$Id: cant.C,v 1.13 2002-04-13 12:30:32 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean find_flag = FALSE;
static props_t *command_defines;   /* -Dfoo=bar on the commandline */
static list_t<const char> command_targets;     /* targets specified on the commandline */
static char *buildfile = "build.xml";
static unsigned parallelism = 1;
static char *globals_file = PKGDATADIR "/globals.xml";

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

    pat = pattern_t::create(pattern, flags);

    if (pat->match(filename) && rep != 0)
    	g_free(pat->replace(rep));

    delete pat;
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

static gboolean
find_buildfile(void)
{
    string_var dir;
    string_var bf;
    char *x;
    struct stat sb;
    
    dir = g_get_current_dir();
    
    for (;;)
    {
    	bf = g_strconcat(dir.data(), "/", buildfile, 0);
	
#if DEBUG
    	fprintf(stderr, "find_buildfile: Trying \"%s\"\n", bf.data());
#endif
	
	if (stat(bf, &sb) == 0)
	{
	    buildfile = bf.take();
#if DEBUG
    	    fprintf(stderr, "find_buildfile: Found \"%s\"\n", buildfile);
#endif
	    return TRUE;
	}
	
	if (dir.data()[1] == '\0')
	    break;  	    /* got to root, still can't find */
	
	/* chop off the last part of the directory */
	if ((x = strrchr(dir, '/')) == 0)
	    break;  	/* JIC */
	*x = '\0';
    }
    
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class cant_t
{
private:
    project_t *proj_;
    project_t *globals_;
    
public:
    cant_t();
    ~cant_t();
    
    gboolean initialise();
    gboolean build_commandline_targets();
};

cant_t::cant_t()
{
    proj_ = 0;
    globals_ = 0;
}

cant_t::~cant_t()
{
    if (proj_ != 0)
	delete proj_;
    if (globals_ != 0)
	delete globals_;
    command_targets.remove_all();
    task_scope_t::cleanup_builtins();
    file_pop_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cant_t::initialise()
{
    task_scope_t::initialise_builtins();
    mapper_t::initialise_builtins();
    if (!job_t::init(parallelism))
    	return FALSE;

    // First project read automatically becomes project_t::globals_
    // We use the pointer return here only to detect failure to load
    // and then later for cleanup.
    globals_ = read_project(globals_file, /*parent*/0, /*isglobals*/TRUE);
    if (globals_ == 0)
    {
    	log::errorf("Can't read globals file \"%s\"\n", globals_file);
	return FALSE;
    }

    if (find_flag && !find_buildfile())
    {
    	log::errorf("Can't find buildfile \"%s\" in any parent directory\n", buildfile);
	return FALSE;
    }

    if ((proj_ = read_buildfile(buildfile, /*parent*/0)) == 0)
    {
    	log::errorf("Can't read buildfile \"%s\"\n", buildfile);
	return FALSE;
    }

    if (command_defines != 0)
	proj_->override_properties(command_defines);
    	
#if DEBUG
    globals_->dump();
    proj_->dump();
#endif
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cant_t::build_commandline_targets()
{
    list_iterator_t<const char> iter;
    
    if (command_targets.length() == 0)
    	return proj_->execute_target_by_name(proj_->default_target());
	
    for (iter = command_targets.first() ; iter != 0 ; ++iter)
    {
    	if (!proj_->execute_target_by_name(*iter))
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
"--help             print this message and exit\n"
"--version          print CANT version and exit\n"
"--verbose          print more messages\n"
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
    log::messagev(log::ERROR, fmt, args);
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
    	command_defines = new props_t(0);
    command_defines->set(buf, x);
    
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
	    else if (!strcmp(argv[i], "--version"))
	    {
	    	printf("%s version %s\n", PACKAGE, VERSION);
	    	exit(0);
	    }
	    else if (!strcmp(argv[i], "--verbose"))
	    {
	    	verbose = TRUE;
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
	    command_targets.append(argv[i]);
	}
    }
    
    if (find_flag && strchr(buildfile, '/') != 0)
    	usagef(1, "When using -find, please specify only a filename for -buildfile");

#if DEBUG
    fprintf(stderr, "parse_args: find_flag = %d\n", find_flag);
    fprintf(stderr, "parse_args: buildfile = \"%s\"\n", buildfile);
    fprintf(stderr, "parse_args: globals file = \"%s\"\n", globals_file);
    {
    	list_iterator_t<const char> iter;
	fprintf(stderr, "parse_args: command_targets =");
	for (iter = command_targets.first() ; iter != 0 ; ++iter)
	    fprintf(stderr, " \"%s\"", *iter);
	fprintf(stderr, "\n");
    }
    
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


int
main(int argc, char **argv)
{
    log_simple_context_t context(file_basename_c(argv[0]));

    parse_args(argc, argv);

#if PATTERN_TEST
    hack_pattern_test();
#elif MAPPER_TEST
    hack_mapper_test();
#else
    cant_t cant;

    if (!cant.initialise())
    	return 1;

    if (!cant.build_commandline_targets())
    	return 1;
#endif

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
