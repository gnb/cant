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

CVSID("$Id: taglist.C,v 1.5 2002-04-06 04:16:38 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static tl_def_tag_t *
tl_def_tag_new(
    const char *name,
    availability_t name_avail,
    availability_t value_avail)
{
    tl_def_tag_t *tltag;
    
    tltag = new(tl_def_tag_t);
    
    strassign(tltag->name, name);
    tltag->name_avail = name_avail;
    tltag->value_avail = value_avail;
    
    return tltag;
}

static void
tl_def_tag_delete(tl_def_tag_t *tltag)
{
    strdelete(tltag->name);
    g_free(tltag);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tl_def_t *
tl_def_new(const char *name)
{
    tl_def_t *tldef;
    
    tldef = new(tl_def_t);
    
    strassign(tldef->name, name);
    tldef->tags = new hashtable_t<const char *, tl_def_tag_t>;
    
    return tldef;
}

static gboolean
remove_one_tag(const char *key, tl_def_tag_t *value, void *userdata)
{
    tl_def_tag_delete(value);
    return TRUE;    /* remove me please */
}

void
tl_def_delete(tl_def_t *tldef)
{
    tldef->tags->foreach_remove(remove_one_tag, 0);
    delete tldef->tags;
    strdelete(tldef->name);
    g_free(tldef);
}

tl_def_tag_t *
tl_def_add_tag(
    tl_def_t *tldef,
    const char *name,
    availability_t name_avail,
    availability_t value_avail)
{
    tl_def_tag_t *tltag;
    
    tltag = tl_def_tag_new(name, name_avail, value_avail);
    tldef->tags->insert(tltag->name, tltag);
    
    return tltag;
}

const tl_def_tag_t *
tl_def_find_tag(
    const tl_def_t *tldef,
    const char *name)
{
    return tldef->tags->lookup(name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

tagexp_t *
tagexp_new(const char *name_space)
{
    tagexp_t *te;
    
    te = new(tagexp_t);
    
    strassign(te->name_space, name_space);
    te->exps = new hashtable_t<char *, strarray_t>;
    
    return te;
}

static gboolean
remove_one_expansion(char *key, strarray_t *value, void *userdata)
{
    g_free(key);
    delete value;
    return TRUE;    /* remove me please */
}

void
tagexp_delete(tagexp_t *te)
{
    strdelete(te->name_space);
    if (te->default_exps != 0)
	delete te->default_exps;
    te->exps->foreach_remove(remove_one_expansion, 0);
    delete te->exps;
    g_free(te);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
tagexp_add_default_expansion(tagexp_t *te, const char *s)
{
    if (te->default_exps == 0)
    	te->default_exps = new strarray_t;
    te->default_exps->append(s);
}

void
tagexp_add_expansion(tagexp_t *te, const char *tag, const char *exp)
{
    strarray_t *sa;
    
    if ((sa = te->exps->lookup((char *)tag)) == 0)
    {
    	sa = new strarray_t;
	te->exps->insert(g_strdup(tag), sa);
    }
    sa->append(exp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static tl_item_t *
tl_item_new(
    const char *tag,
    const char *name,
    tl_item_type_t type,
    const char *value)
{
    tl_item_t *tlitem;
    
    tlitem = new(tl_item_t);
    
    strassign(tlitem->tag, tag);
    strassign(tlitem->name, name);
    tlitem->type = type;
    strassign(tlitem->value, value);
    
    return tlitem;
}

static void
tl_item_delete(tl_item_t *tlitem)
{
    strdelete(tlitem->tag);
    strdelete(tlitem->name);
    strdelete(tlitem->value);
    g_free(tlitem);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

taglist_t *
taglist_new(const char *name_space)
{
    taglist_t *tl;
    
    tl = new(taglist_t);
    
    tl->refcount = 1;
    strassign(tl->name_space, name_space);
    
    return tl;
}

static void
taglist_delete(taglist_t *tl)
{
    tl->items.apply_remove(tl_item_delete);
    strdelete(tl->name_space);
    strdelete(tl->id);
    g_free(tl);
}

void
taglist_ref(taglist_t *tl)
{
    tl->refcount++;
}

void
taglist_unref(taglist_t *tl)
{
    if (--tl->refcount == 0)
    	taglist_delete(tl);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
taglist_set_id(taglist_t *tl, const char *s)
{
    strassign(tl->id, s);
}

tl_item_t *
taglist_add_item(
    taglist_t *tl,
    const char *tag,
    const char *name,
    tl_item_type_t type,
    const char *value)
{
    tl_item_t *tlitem;
    
    tlitem = tl_item_new(tag, name, type, value);
    tl->items.append(tlitem);
    
    return tlitem;
}
    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
gather_exps(
    props_t *localprops,
    strarray_t *exps,
    strarray_t *sa)
{
    char *expexp;
    int i;

    for (i = 0 ; i < exps->len ; i++)
    {    
	expexp = props_expand(localprops, exps->nth(i));
	strnullnorm(expexp);
	if (expexp != 0)
	{
	    sa->appendm(expexp);
#if DEBUG
	    fprintf(stderr, "taglist_gather:     -> \"%s\"\n", expexp);
#endif
    	}
    }
}

void
taglist_gather(
    taglist_t *tl,  	/* taglist to expand */
    tagexp_t *te,   	/* definition of expansions */
    props_t *props, 	/* for evaluating conditions & expansions */
    strarray_t *sa) 	/* results appended to this */
{
    list_iterator_t<tl_item_t> iter;
    strarray_t *exps;
    char *expvalue;
    props_t *localprops;

    if (strcmp(tl->name_space, te->name_space))
    {
    	logf("ERROR: namespace mismatch in taglist_gather, taglist=%s::%s tagexpand=%s::\n",
	    	tl->name_space, tl->id, te->name_space);
	return;
    }

#if DEBUG
    fprintf(stderr, "taglist_gather: gathering %s::%s {\n",
    	    	tl->name_space, tl->id);
#endif

    /*
     * TODO: deal with the `reverse_order' and `follow_depends' flags,
     *       by building a temporary list from this and optionally
     *       all it's dependencies, and optionally reversing it, then
     *       deleting it from the head instead of just iterating.
     */
    
    localprops = props_new(/*parent*/props);
    
    for (iter = tl->items.first() ; iter != 0 ; ++iter)
    {
    	tl_item_t *tlitem = *iter;
	
	if (!condition_evaluate(&tlitem->condition, props))
	    continue;
	
	exps = te->exps->lookup(tlitem->tag);
	if (exps == 0)
	    exps = te->default_exps;
	if (exps == 0)
	    continue;

#if DEBUG
	fprintf(stderr, "taglist_gather:     tag=\"%s\" name=\"%s\" value=\"%s\"\n",
	    	    	    tlitem->tag, tlitem->name, tlitem->value);
#endif

	props_set(localprops, "name", tlitem->name);
	props_set(localprops, "tag", tlitem->tag);

	switch (tlitem->type)
	{
	case TL_VALUE:
	    props_set(localprops, "value", tlitem->value);
    	    gather_exps(localprops, exps, sa);
	    break;

	case TL_LINE:
	    /* to get whitespace semantics right, need to pre-expand */
	    expvalue = props_expand(props, tlitem->value);
	    if (expvalue != 0)
	    {
		/* tokenise value on whitespace */
		const char *x;
		tok_t tok(expvalue);

		while ((x = tok.next()) != 0)
		{
		    props_set(localprops, "value", x);
    		    gather_exps(localprops, exps, sa);
		}
	    }
	    break;

	case TL_FILE:
	    /* to canonicalise the right filename, need to pre-expand */
	    expvalue = props_expand(props, tlitem->value);
	    strnullnorm(expvalue);
	    if (expvalue != 0)
	    {
	    	props_setm(localprops, "value", expvalue);
    		gather_exps(localprops, exps, sa);
	    }
	    break;
	}
    }
    
    props_delete(localprops);

#if DEBUG
    fprintf(stderr, "taglist_gather: }\n");
#endif

}

void
taglist_list_gather(
    const list_t<taglist_t> *list,
    tagexp_t *te,
    props_t *props,
    strarray_t *sa)
{
    list_iterator_t<taglist_t> iter;
    
    for (iter = list->first() ; iter != 0 ; ++iter)
    {
    	taglist_t *tl = *iter;
	
	if (!strcmp(tl->name_space, te->name_space))
	    taglist_gather(tl, te, props, sa);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
