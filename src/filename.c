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
#include "tok.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

CVSID("$Id: filename.c,v 1.6 2002-02-08 07:18:35 gnb Exp $");

#ifndef __set_errno
#define __set_errno(v)	 errno = (v)
#endif

/*
 * Each element in the stack is a fully normalised
 * relative directory from the process' current working
 * directory (i.e. the root in a dir-tree build) to
 * the directory from which filenames need to be
 * interpreted.
 */
static GList *dir_stack;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
file_push_dir(const char *dirname)
{
    dir_stack = g_list_prepend(dir_stack, file_normalise(dirname, 0));
}

void
file_pop_dir(void)
{
    if (dir_stack != 0)
    {
    	g_free(dir_stack->data);
	dir_stack = g_list_remove_link(dir_stack, dir_stack);
    }
}

const char *
file_top_dir(void)
{
    return (dir_stack == 0 ? "." : (const char *)dir_stack->data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
file_normalise_m(char *filename, const char *basedir)
{
    const char *fp;
    estring abs;
    tok_t tok;
        
#if DEBUG
    fprintf(stderr, "file_normalise_m(\"%s\", \"%s\")",
    	filename, (basedir == 0 ? file_top_dir() : basedir));
#endif
    estring_init(&abs);

    /* seed `abs' with the base directory to which `filename' is relative */
    if (filename[0] == '/')
    	estring_append_char(&abs, '/');
    else if (basedir == 0)
    	estring_append_string(&abs, file_top_dir());
    else
    	estring_append_string(&abs, basedir);
    
    /* iterate over parts of `filename', appending to `abs' */
    tok_init_m(&tok, filename, "/");
    while ((fp = tok_next(&tok)) != 0)
    {
    	if (!strcmp(fp, "."))
	{
	    /* drop redundant `.' */
	}
	else if (!strcmp(fp, ".."))
	{
	    /* back up one dir level */
	    char *p = strrchr(abs.data, '/');
	    if (p == 0)
	    	estring_append_string(&abs, "/..");
	    else if (p != abs.data)
	    	estring_truncate_to(&abs, (p - abs.data));
	}
	else
	{
	    /* some other component -- just append it */
	    if (abs.length > 1 || abs.data[0] != '/')
		estring_append_char(&abs, '/');
	    estring_append_string(&abs, fp);
	}
    }
    
    if (abs.data[0] == '.' && abs.data[1] == '/')
    	estring_remove(&abs, 0, 2);
    
    tok_free(&tok);
#if DEBUG
    fprintf(stderr, " -> \"%s\"\n", abs.data);
#endif
    return abs.data;
}

char *
file_normalise(const char *filename, const char *basedir)
{
    return file_normalise_m(g_strdup(filename), basedir);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
file_dirname(const char *filename)
{
    const char *base;
    
    if ((base = file_basename_c(filename)) == filename)
    	return g_strdup(".");
    return g_strndup(filename, (base - filename - 1));
}

const char *
file_basename_c(const char *filename)
{
    const char *base;
    
    return ((base = strrchr(filename, '/')) == 0 ? filename : ++base);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mode_t
file_mode_from_string(const char *str, mode_t base, mode_t deflt)
{
    if (str == 0 || *str == '\0')
    	return deflt;
	
    if (str[0] >= '0' && str[0] <= '7')
    	return strtol(str, 0, 8);

    fprintf(stderr, "TODO: can't handle mode strings properly\n");
    return base;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

FILE *
file_open_mode(const char *filename, const char *rw, mode_t mode)
{
    int fd;
    FILE *fp;
    int flags;
    char *norm_filename;
    
    if (rw[0] == 'r')
    	flags = O_RDONLY;
    else
    	flags = O_WRONLY|O_CREAT;
    
    norm_filename = file_normalise(filename, 0);
    
    if ((fd = open(norm_filename, flags, mode)) < 0)
    {
    	int e = errno;
	g_free(norm_filename);
	__set_errno(e);
	return 0;
    }
    g_free(norm_filename);
    
    if ((fp = fdopen(fd, rw)) == 0)
    {
    	int e = errno;
	close(fd);
	__set_errno(e);
	return 0;
    }
    
    return fp;
}

DIR *
file_opendir(const char *dirname)
{
    char *norm_dirname;
    DIR *dir;
    
    norm_dirname = file_normalise(dirname, 0);
    dir = opendir(norm_dirname);
    g_free(norm_dirname);
    
    return dir;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
file_stat(const char *filename, struct stat *sb)
{
    char *norm_filename;
    
    norm_filename = file_normalise(filename, 0);
    if (stat(norm_filename, sb) < 0)
    {
	int e = errno;
	g_free(norm_filename);
	__set_errno(e);
	return -1;
    }
    g_free(norm_filename);
    return 0;
}

mode_t
file_mode(const char *filename)
{
    struct stat sb;
    
    if (file_stat(filename, &sb) < 0)
    	return -1;
	
    return (sb.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO));
}


int
file_exists(const char *filename)
{
    struct stat sb;
    
    if (file_stat(filename, &sb) < 0)
    	return (errno == ENOENT ? -1 : 0);
    return 0;
}

int
file_is_directory(const char *filename)
{
    struct stat sb;
    
    if (file_stat(filename, &sb) < 0)
    	return -1;
	
    if (!S_ISDIR(sb.st_mode))
    {
    	__set_errno(ENOTDIR);
    	return -1;
    }
    
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
file_build_tree(const char *dirname, mode_t mode)
{
    char *p, *dir = file_normalise(dirname, 0);
    int ret = 0;
    char oldc;
    
    /* skip leading /s */
    for (p = dir ; *p && *p == '/' ; p++)
    	;
	
    /* check and make each directory part in turn */
    for (;;)
    {
    	/* skip p to next / */
	for ( ; *p && *p != '/' ; p++)
	    ;
	    
	oldc = *p;
	*p = '\0';
	
	if (file_exists(dir) < 0)
	{
	    if (mkdir(dir, mode) < 0)
	    {
	    	ret = -1;
	    	break;
	    }
	}
	
	if (file_is_directory(dir) < 0)
	{
	    ret = -1;
	    break;
	}
	
	if (!oldc)
	    break;
	*p = '/';
	
	/* skip possible multiple / */
	for ( ; *p && *p == '/' ; p++)
	    ;
	if (!*p)
	    break;
    }
    
    g_free(dir);
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
file_apply_children(
    const char *filename,
    file_apply_proc_t function,
    void *userdata)
{
    DIR *dir;
    struct dirent *de;
    estring child;
    int ret = 1;
    
    if ((dir = file_opendir(filename)) == 0)
    	return -1;
	
    estring_init(&child);
    while ((de = readdir(dir)) != 0)
    {
    	if (!strcmp(de->d_name, ".") ||
	    !strcmp(de->d_name, ".."))
	    continue;
	    
	/* TODO: estring_truncate_to() */
	estring_truncate(&child);
	if (strcmp(filename, "."))
	{
	    estring_append_string(&child, filename);
	    estring_append_char(&child, '/');
	}
	estring_append_string(&child, de->d_name);
	
	if (!(*function)(child.data, userdata))
	{
	    ret = 0;
	    break;
	}
    }
    
    estring_free(&child);
    closedir(dir);
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
