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
#include "tok.H"
#include <time.h>

CVSID("$Id: task_foreach.C,v 1.9 2002-04-13 12:30:42 gnb Exp $");

class foreach_task_t : public task_t
{
private:
    string_var variable_;
    /* TODO: whitespace-safe technique */
    string_var values_;
    list_t<fileset_t> filesets_;
    /* TODO: support nested <property> tags */

    string_var variable_e_;
    gboolean failed_:1;

public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

foreach_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
}

~foreach_task_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
set_variable(const char *name, const char *value)
{
    variable_ = value;
    return TRUE;
}

gboolean
set_values(const char *name, const char *value)
{
    values_ = value;
    return TRUE;
}

gboolean
add_fileset(xml_node_t *node)
{
    fileset_t *fs;
    
    if ((fs = parse_fileset(project_, node, "dir")) == 0)
    	return FALSE;

    filesets_.append(fs);
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
do_iteration(const char *val, void *userdata)
{
    foreach_task_t *ft = (foreach_task_t *)userdata;

    /*
     * TODO: need a props stack... (per-task?) to get scope right.
     * For now, this variable goes into the project scope.
     */
    ft->project_->set_property(ft->variable_e_, val);
    if (verbose)
	log::infof("%s = %s\n", ft->variable_e_.data(), val);

    if (!ft->execute_subtasks())
    {
	ft->failed_ = TRUE;
	return FALSE;
    }
    return TRUE;
}

gboolean
exec()
{
    const char *val;
    list_iterator_t<fileset_t> iter;

    failed_ = FALSE;    
    variable_e_ = expand(variable_);

    tok_t tok(expand(values_), ",");
    while (!failed_ && (val = tok.next()) != 0)
    	do_iteration(val, this);
    
    for (iter = filesets_.first() ; !failed_ && iter != 0 ; ++iter)
	(*iter)->apply(project_->properties(), do_iteration, this);

    variable_e_ = (char*)0;

    return !failed_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

}; // end of class

static task_attr_t foreach_attrs[] = 
{
    TASK_ATTR(foreach, variable, 0),
    TASK_ATTR(foreach, values, 0),
    {0}
};

static task_child_t foreach_children[] = 
{
    TASK_CHILD(foreach, fileset, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(foreach,
			foreach_attrs,
			foreach_children,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/TRUE)
TASK_DEFINE_CLASS_END(foreach)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
