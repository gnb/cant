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

#include "savedep.H"
#include "estring.H"
#include "filename.H"
#include "depfile.H"
#include "log.H"

CVSID("$Id: savedep.C,v 1.1 2002-04-21 04:01:40 gnb Exp $");

savedep_t *savedep_t::instance_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

savedep_t::savedep_t(const char *filename)
 :  filename_(filename)
{
    assert(instance_ == 0);
    instance_ = this;
    
    deps_ = new hashtable_t<char*, hashtable_t<char*, quality_t> >;
    load();
#if DEBUG
    dump();
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
remove_one_dep2(char *key, savedep_t::quality_t *value, void *closure)
{
    g_free(key);
    delete value;
    return TRUE;    // remove me please
}

static gboolean
remove_one_dep(char *key, hashtable_t<char*, savedep_t::quality_t> *value, void *closure)
{
    g_free(key);
    value->foreach_remove(remove_one_dep2, 0);
    delete value;
    return TRUE;    // remove me please
}

savedep_t::~savedep_t()
{
    assert(instance_ == this);
    instance_ = 0;

#if DEBUG
    dump();
#endif
    save();
        
    deps_->foreach_remove(remove_one_dep, 0);
    delete deps_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
savedep_t::add(const char *from, const char *to, savedep_t::quality_t q)
{
    hashtable_t<char*, quality_t> *ht;
    quality_t *qp;
    
    if ((ht = deps_->lookup((char *)from)) == 0)
    {
    	ht = new hashtable_t<char*, quality_t>;
	deps_->insert(g_strdup(from), ht);
    }
    
    if ((qp = ht->lookup((char *)to)) == 0 && q > NONE)
    {
    	qp = new quality_t;
	*qp = q;
    	ht->insert(g_strdup(to), qp);
    }
    else if (qp != 0)
    {
    	*qp = MAX(*qp, q);
    }
}

void
savedep_t::add(const char *from, strarray_t *to, savedep_t::quality_t q)
{
    hashtable_t<char*, quality_t> *ht;
    quality_t *qp;
    unsigned int i;
    
    if ((ht = deps_->lookup((char *)from)) == 0)
    {
    	ht = new hashtable_t<char*, quality_t>;
	deps_->insert(g_strdup(from), ht);
    }
    
    for (i = 0 ; i < to->len ; i++)
    {
	if ((qp = ht->lookup((char *)to->nth(i))) == 0 && q > NONE)
	{
    	    qp = new quality_t;
	    *qp = q;
	    // TODO: char *strarray_t::take_nth(unsigned)
    	    ht->insert(g_strdup(to->nth(i)), qp);
	}
	else if (qp != 0)
	{
    	    *qp = MAX(*qp, q);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

savedep_t::quality_t
savedep_t::get_quality(const char *from, const char *to)
{
    hashtable_t<char*, quality_t> *ht;
    quality_t *qp;
    
    if ((ht = deps_->lookup((char *)from)) == 0)
    	return NONE;
    if ((qp = ht->lookup((char *)to)) == 0)
    	return NONE;
    return *qp;
}
    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct savedep_apply_rec_t
{
    const char *from;
    savedep_t::apply_func_t function;
    void *closure;
};

static void
from_apply_one_2(char *key, savedep_t::quality_t *qp, void *closure)
{
    savedep_apply_rec_t *ar = (savedep_apply_rec_t *)closure;
    
    (*ar->function)(ar->from, key, *qp, ar->closure);
}

static void
from_apply_one(
    char *key,
    hashtable_t<char*, savedep_t::quality_t> *ht,
    void *closure)
{
    savedep_apply_rec_t *ar = (savedep_apply_rec_t *)closure;

    ar->from = key;    
    ht->foreach(from_apply_one_2, ar);
}

void
savedep_t::apply(savedep_t::apply_func_t function, void *closure) const
{
    savedep_apply_rec_t ar;
    
    ar.function = function;
    ar.closure = closure;
    deps_->foreach(from_apply_one, &ar);
}

void
savedep_t::from_apply(
    const char *from,
    savedep_t::apply_func_t function,
    void *closure) const
{
    hashtable_t<char*, quality_t> *ht;
    savedep_apply_rec_t ar;
    
    if ((ht = deps_->lookup((char *)from)) == 0)
    	return;
    
    ar.from = from;
    ar.function = function;
    ar.closure = closure;
    ht->foreach(from_apply_one_2, &ar);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if DEBUG

static void
dump_one_dep(
    const char *from,
    const char *to,
    savedep_t::quality_t q,
    void *closure)
{
    fprintf(stderr, "    from=\"%s\" to=\"%s\" quality=%d\n", from, to, q);
}

void
savedep_t::dump() const
{
    fprintf(stderr, "savedeps:\n");
    apply(dump_one_dep, 0);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class savedep_depfile_reader_t : public depfile_reader_t
{
public:
    
    savedep_depfile_reader_t(const char *filename)
     :  depfile_reader_t(filename)
    {
    }
    ~savedep_depfile_reader_t()
    {
    }

    void open_error()
    {
    	if (errno != ENOENT)
	    log::perror(filename_);
    }
    
    void add_dep(const char *from, const char *to)
    {
	savedep_t::instance()->add(from, to, savedep_t::LOADED);
    }
};

gboolean
savedep_t::load()
{
    savedep_depfile_reader_t reader(filename_);
    return reader.read();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
save_one_dep(
    const char *from,
    const char *to,
    savedep_t::quality_t q,
    void *closure)
{
    FILE *fp = (FILE *)closure;
    
    switch (q)
    {
    case savedep_t::LOADED:
    case savedep_t::EXTRACTED:
    	fprintf(fp, "%s: %s\n", from, to);
	break;
    default:
    	break;
    }
}

gboolean
savedep_t::save() const
{
    FILE *fp;

    if (file_exists(filename_) == 0)
    {
	string_var bakfile = g_strconcat(filename_.data(), ".bak", 0);
	unlink(bakfile);    
	rename(filename_, bakfile);
    }

    if ((fp = fopen(filename_, "w")) == 0)
    {
    	log::perror(filename_);
	return FALSE;
    }

    apply(save_one_dep, fp);

    fclose(fp);    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
