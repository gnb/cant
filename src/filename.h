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

#ifndef _cant_filename_h_
#define _cant_filename_h_ 1

#include "common.h"
#include "estring.h"
#include <dirent.h>

typedef gboolean (*file_apply_proc_t)(const char *filename, void *userdata);

/*
 * Manipulate the internal stack of pseudo-working-directories
 * which are used to interpret filenames passed to file_mode() etc.
 * This trick allows projects from different directories to
 * co-exist in the same process, as long as we never call system
 * calls like stat() directly.
 */
void file_push_dir(const char *dirname);
void file_pop_dir(void);
const char *file_top_dir(void);

/*
 * In the next 2 functions, if `basedir' is 0, the topmost directory
 * in the directory stack is used; this is usually what you want.
 */
char *file_normalise(const char *dir, const char *basedir/*may be 0*/);
char *file_normalise_m(char *dir, const char *basedir/*may be 0*/);

/*
 * These work directly on the given filename textually, and
 * do not internally normalise.
 */
char *file_dirname(const char *filename);
const char *file_basename_c(const char *filename);
mode_t file_mode_from_string(const char *str, mode_t base, mode_t deflt);
/*
 * These will internally normalise the filename because they
 * need to pass it to system calls.
 */
FILE *file_open_mode(const char *filename, const char *rw, mode_t mode);
DIR *file_opendir(const char *dirname);
mode_t file_mode(const char *filename);
int file_exists(const char *filename);
int file_build_tree(const char *dirname, mode_t mode);	/* make sequence of directories */
int file_apply_children(const char *filename, file_apply_proc_t, void *userdata);
int file_is_directory(const char *filename);
int file_rmdir(const char *filename);
int file_unlink(const char *filename);


#endif /* _cant_filename_h_ */
