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

#ifndef _target_h_
#define _target_h_ 1

#include "common.H"
#include "condition.H"
#include "list.H"
#include "task.H"
#include "string_var.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class project_t;

class target_t
{
private:

    enum _flags
    {
    	/* values for `flags_' */
    	T_DEFINED   	    =(1<<0),  	/* defined with <target> element */
    	T_DEPENDED_ON	    =(1<<1)  	/* referenced in at least one `depends' attribute */
    };
    
    string_var name_;
    string_var description_;
    project_t *project_;
    unsigned flags_;
    list_t<target_t> depends_;
public: // AAAAARGH HACK TODO
    condition_t condition_;
private:
    list_t<task_t> tasks_;

public:
    target_t();
    ~target_t();
    
    const char *name() const { return name_; }
    void set_name(const char *name);
    
    const char *description() const { return description_; }
    void set_description(const char *description);
    
    project_t *project() const { return project_; }
    void set_project(project_t *);

    gboolean is_defined() const { return (flags_ & T_DEFINED); }    
    void set_is_defined(gboolean b);

    void add_task(task_t *task);

    gboolean is_depended_on() const { return (flags_ & T_DEPENDED_ON); }
    void add_depend(target_t *dep);

    gboolean execute();
    
#if DEBUG
    void dump() const;
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _target_h_ */
