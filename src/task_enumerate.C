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

CVSID("$Id: task_enumerate.C,v 1.7 2002-04-13 02:30:18 gnb Exp $");


class enumerate_task_t : public task_t
{
private:
    list_t<fileset_t> filesets_;
    
public:

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

enumerate_task_t(task_class_t *tclass, project_t *proj)
 :  task_t(tclass, proj)
{
}

~enumerate_task_t()
{
    filesets_.apply_remove(unref);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

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

gboolean
exec()
{
    list_iterator_t<fileset_t> iter;
    
    for (iter = filesets_.first() ; iter != 0 ; ++iter)
    {
    	fileset_t *fs = *iter;
	strarray_t *sa = new strarray_t;
	int i;
	
	fs->gather_mapped(project_->properties(), sa, 0);
    	sa->sort(0);
	
	log::infof("{\n");
	for (i = 0 ; i < sa->len ; i++)
	    log::infof("%s\n", sa->nth(i));
	log::infof("}\n");
	
	delete sa;
    }
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

};  // end of class

static task_child_t enumerate_children[] = 
{
    TASK_CHILD(enumerate, fileset, 0),
    {0}
};

TASK_DEFINE_CLASS_BEGIN(enumerate,
			/*attrs*/0,
			enumerate_children,
			/*is_fileset*/FALSE,
			/*fileset_dir_name*/0,
			/*is_composite*/FALSE)
TASK_DEFINE_CLASS_END(enumerate)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
