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

#ifndef _globber_h_
#define _globber_h_ 1

#include "common.H"
#include "filename.H"
#include "pattern.H"
#include "list.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
//
// Class globber_t handles all the grunt work of expanding
// a set of filenames and wildcard expressions, including
// handling recursion for the ** metacharacter.
//
class globber_t
{
private:
    char *basedir_;
    list_t<char> filenames_;
    list_t<char> pending_;
    gboolean case_sensitive_;
    gboolean check_exists_;
    
    void glob_part(const char *globpart, list_iterator_t<char> baselink);
    void recurse(const pattern_t *pat, const char *base);
    void glob_path_m(char *globpath);
    void apply_file(const char *pattfile, gboolean include);

public:
    globber_t(const char *basedir, gboolean case_sensitive);
    ~globber_t();
    
    void include(const char *glob);
    void exclude(const char *glob);
    void exclude(const pattern_t *pat);

    void include_file(const char *pattfile);
    void exclude_file(const char *pattfile);
    
    list_iterator_t<char> first_filename() const { return filenames_.first(); }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _globber_h_ */
