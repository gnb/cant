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

#ifndef _runner_h_
#define _runner_h_ 1

#include "common.H"
#include "hashtable.H"
#include "strarray.H"
#include "string_var.H"
#include "props.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
class runner_t;
typedef runner_t * (runner_creator_t)(void);

class runner_t
{
private:
    static hashtable_t<char*, runner_creator_t> *creators;

protected:    
    strarray_t *command_;
    strarray_t *environment_;
    string_var directory_;

    runner_t();

    // separate from ctor to allow it to fail gracefully
    virtual gboolean init();

    int vulture(pid_t pid);
    gboolean interpret_status(int status);

public:
    virtual ~runner_t();

    static runner_t *create(const char *name);

    virtual void set_command(strarray_t *cmd);
    virtual void set_environment(strarray_t *env);

    virtual void setup_properties(props_t *props) const;
    virtual char *describe() const;
    virtual gboolean run() = 0;
    virtual strarray_t *extracted_dependencies() const;

    static gboolean add_creator(const char *name, runner_creator_t *);
    // TODO: remove_creator

    static void initialise_builtins();
};

#define RUNNER_DEFINE_CLASS(nm) \
\
runner_t *runner_##nm##_create(void) \
{ \
    return new runner_##nm##_t(); \
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _runner_h_ */
