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

CVSID("$Id: taglist.C,v 1.10 2002-04-13 03:18:40 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tl_def_t::tag_t::tag_t(
    const char *name,
    availability_t name_avail,
    availability_t value_avail)
{
    strassign(name_, name);
    name_avail_ = name_avail;
    value_avail_ = value_avail;
}

tl_def_t::tag_t::~tag_t()
{
    strdelete(name_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tl_def_t::tl_def_t(const char *name_space)
{
    strassign(name_space_, name_space);
    tags_ = new hashtable_t<const char *, tag_t>;
}

static gboolean
remove_one_tag(const char *key, tl_def_t::tag_t *value, void *userdata)
{
    delete value;
    return TRUE;    /* remove me please */
}

tl_def_t::~tl_def_t()
{
    tags_->foreach_remove(remove_one_tag, 0);
    delete tags_;
    strdelete(name_space_);
}

tl_def_t::tag_t *
tl_def_t::add_tag(
    const char *name,
    availability_t name_avail,
    availability_t value_avail)
{
    tag_t *tltag;
    
    tltag = new tag_t(name, name_avail, value_avail);
    tags_->insert(tltag->name_, tltag);
    
    return tltag;
}

const tl_def_t::tag_t *
tl_def_t::find_tag(const char *name) const
{
    return tags_->lookup(name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tagexp_t::tagexp_t(const char *name_space)
{
    strassign(name_space_, name_space);
    expansions_ = new hashtable_t<char *, strarray_t>;
}

static gboolean
remove_one_expansion(char *key, strarray_t *value, void *userdata)
{
    g_free(key);
    delete value;
    return TRUE;    /* remove me please */
}

tagexp_t::~tagexp_t()
{
    strdelete(name_space_);
    if (default_expansions_ != 0)
	delete default_expansions_;
    expansions_->foreach_remove(remove_one_expansion, 0);
    delete expansions_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
tagexp_t::add_default_expansion(const char *s)
{
    if (default_expansions_ == 0)
    	default_expansions_ = new strarray_t;
    default_expansions_->append(s);
}

void
tagexp_t::add_expansion(const char *tag, const char *exp)
{
    strarray_t *sa;
    
    if ((sa = expansions_->lookup((char *)tag)) == 0)
    {
    	sa = new strarray_t;
	expansions_->insert(g_strdup(tag), sa);
    }
    sa->append(exp);
}

const strarray_t *
tagexp_t::find_expansions(const char *tag) const
{
    const strarray_t *sa;
    
    if ((sa = expansions_->lookup((char *)tag)) == 0)
    	sa = default_expansions_;
    return sa;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

taglist_t::item_t::item_t(
    const char *tag,
    const char *name,
    taglist_t::item_type_t type,
    const char *value)
{
    strassign(tag_, tag);
    strassign(name_, name);
    type_ = type;
    strassign(value_, value);
}

taglist_t::item_t::~item_t()
{
    strdelete(tag_);
    strdelete(name_);
    strdelete(value_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

taglist_t::taglist_t(const char *name_space)
{
    refcount_ = 1;
    strassign(name_space_, name_space);
}

taglist_t::~taglist_t()
{
    items_.delete_all();
    strdelete(name_space_);
    strdelete(id_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
taglist_t::ref()
{
    refcount_++;
}

void
taglist_t::unref()
{
    if (--refcount_ == 0)
    	delete this;
}

void
unref(taglist_t *tl)
{
    tl->unref();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
taglist_t::set_id(const char *s)
{
    strassign(id_, s);
}

taglist_t::item_t *
taglist_t::add_item(
    const char *tag,
    const char *name,
    item_type_t type,
    const char *value)
{
    item_t *tlitem;
    
    tlitem = new item_t(tag, name, type, value);
    items_.append(tlitem);
    
    return tlitem;
}

taglist_t::item_t *
taglist_t::add_value(
    const char *tag,
    const char *name,
    const char *value)
{
    return add_item(tag, name, TL_VALUE, value);
}
    
taglist_t::item_t *
taglist_t::add_line(
    const char *tag,
    const char *name,
    const char *value)
{
    return add_item(tag, name, TL_LINE, value);
}

taglist_t::item_t *
taglist_t::add_file(
    const char *tag,
    const char *name,
    const char *value)
{
    return add_item(tag, name, TL_FILE, value);
}
    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
gather_exps(
    const props_t *localprops,
    const strarray_t *exps,
    strarray_t *sa)
{
    char *expexp;
    int i;

    for (i = 0 ; i < exps->len ; i++)
    {    
	expexp = localprops->expand(exps->nth(i));
	strnullnorm(expexp);
	if (expexp != 0)
	{
	    sa->appendm(expexp);
#if DEBUG
	    fprintf(stderr, "taglist_t::gather:     -> \"%s\"\n", expexp);
#endif
    	}
    }
}

void
taglist_t::gather(
    const tagexp_t *te, /* definition of expansions */
    const props_t *props,/* for evaluating conditions & expansions */
    strarray_t *sa) 	/* results appended to this */
    const
{
    list_iterator_t<item_t> iter;
    const strarray_t *exps;
    char *expvalue;
    props_t *localprops;

    if (strcmp(name_space_, te->name_space()))
    {
    	log::errorf("namespace mismatch in taglist_t::gather, taglist=%s::%s tagexpand=%s::\n",
	    	name_space_, id_, te->name_space());
	return;
    }

#if DEBUG
    fprintf(stderr, "taglist_t::gather: gathering %s::%s {\n",
    	    	name_space_, id_);
#endif

    /*
     * TODO: deal with the `reverse_order' and `follow_depends' flags,
     *       by building a temporary list from this and optionally
     *       all it's dependencies, and optionally reversing it, then
     *       deleting it from the head instead of just iterating.
     */
    
    localprops = new props_t(/*parent*/props);
    
    for (iter = items_.first() ; iter != 0 ; ++iter)
    {
    	item_t *tlitem = *iter;
	
	if (!tlitem->condition_.evaluate(props))
	    continue;
	
	if ((exps = te->find_expansions(tlitem->tag_)) == 0)
	    continue;

#if DEBUG
	fprintf(stderr, "taglist_t::gather:     tag=\"%s\" name=\"%s\" value=\"%s\"\n",
	    	    	    tlitem->tag_, tlitem->name_, tlitem->value_);
#endif

	localprops->set("name", tlitem->name_);
	localprops->set("tag", tlitem->tag_);

	switch (tlitem->type_)
	{
	case TL_VALUE:
	    localprops->set("value", tlitem->value_);
    	    gather_exps(localprops, exps, sa);
	    break;

	case TL_LINE:
	    /* to get whitespace semantics right, need to pre-expand */
	    expvalue = props->expand(tlitem->value_);
	    if (expvalue != 0)
	    {
		/* tokenise value on whitespace */
		const char *x;
		tok_t tok(expvalue);

		while ((x = tok.next()) != 0)
		{
		    localprops->set("value", x);
    		    gather_exps(localprops, exps, sa);
		}
	    }
	    break;

	case TL_FILE:
	    /* to canonicalise the right filename, need to pre-expand */
	    expvalue = props->expand(tlitem->value_);
	    strnullnorm(expvalue);
	    if (expvalue != 0)
	    {
	    	localprops->setm("value", expvalue);
    		gather_exps(localprops, exps, sa);
	    }
	    break;
	}
    }
    
    delete localprops;

#if DEBUG
    fprintf(stderr, "taglist_t::gather: }\n");
#endif

}

void
taglist_t::list_gather(
    const list_t<taglist_t> *list,
    const tagexp_t *te,
    const props_t *props,
    strarray_t *sa)
{
    list_iterator_t<taglist_t> iter;
    
    for (iter = list->first() ; iter != 0 ; ++iter)
    {
    	taglist_t *tl = *iter;
	
	if (!strcmp(tl->name_space_, te->name_space()))
	    tl->gather(te, props, sa);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if DEBUG

void
taglist_t::dump() const
{
    list_iterator_t<item_t> iter;
    
    fprintf(stderr, "    TAGLIST {\n");
    fprintf(stderr, "        NAMESPACE=\"%s\"\n", name_space_);
    fprintf(stderr, "        ID=\"%s\"\n", id_);

    for (iter = first_item() ; iter != 0 ; ++iter)
    {
    	item_t *tlitem = *iter;
	
	fprintf(stderr, "        TL_SPEC {\n");
	fprintf(stderr, "            TAG=\"%s\"\n", tlitem->tag_);
	fprintf(stderr, "            NAME=\"%s\"\n", tlitem->name_);
	fprintf(stderr, "            TYPE=%d\n", tlitem->type_);
	fprintf(stderr, "            VALUE=\"%s\"\n", tlitem->value_);
	fprintf(stderr, "        }\n");
    }
        
    fprintf(stderr, "    }\n");
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
