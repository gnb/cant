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

#include "props.H"
#include "estring.H"
#include "filename.H"
#include "hashtable.H"

CVSID("$Id: props.C,v 1.4 2002-04-07 07:45:00 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

props_t::props_t(const props_t *parent)
{
    parent_ = parent;
    values_ = new hashtable_t<char *, char>;
}

static gboolean
_props_delete_one_value(char *key, char *value, void *userdata)
{
    g_free(key);
    g_free(value);
    return TRUE;    /* so remove me already */
}

props_t::~props_t()
{
    values_->foreach_remove(_props_delete_one_value, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
_props_copy_one(char *key, char *value, void *userdata)
{
    ((props_t *)userdata)->set(key, value);
}

void
props_t::copy_contents(const props_t *orig)
{
    orig->values_->foreach(_props_copy_one, this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
props_t::get(const char *name) const
{
    const props_t *props;
    const char *value;
    
    for (props = this ; props != 0 ; props = props->parent_)
    {
    	if ((value = props->values_->lookup((char *)name)) != 0)
	    return value;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
props_t::setm(const char *name, char *value)
{
    char *okey = 0, *ovalue = 0;
    
    assert(name != 0);
    
    if (value == 0)
    {
	values_->remove((char *)name);
	return;
    }
    
    if (values_->lookup_extended((char *)name, &okey, &ovalue))
    {
    	/* remove the old value */
	values_->remove(okey);
	g_free(okey);
	g_free(ovalue);
    }
    values_->insert(g_strdup(name), value);
}

void
props_t::set(const char *name, const char *value)
{
    setm(name, (value == 0 ? 0 : g_strdup(value)));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
_props_consolidate_one(char *key, char *value, void *userdata)
{
    hashtable_t<char *, char> *consolidated =
    	    	    	    (hashtable_t<char *, char> *)userdata;
    
    if (consolidated->lookup(key) == 0)
    	consolidated->insert(key, value);
}

void
props_t::apply(
    void (*func)(const char *name, const char *value, void *userdata),
    void *userdata) const
{
    hashtable_t<char *, char> *consolidated;
    const props_t *props;
    
    /*
     * Have to build a consolidated hash table to avoid
     * the case where a property is iterated over twice
     * because it appears both in this props_t and in
     * one of its ancestors.
     */
    consolidated = new hashtable_t<char *, char>;

    for (props = this ; props != 0 ; props = props->parent_)
	props->values_->foreach(_props_consolidate_one, consolidated);
    
    consolidated->foreach(
    	    	    (void (*)(char*, char*, void*))func,
		    userdata);
    
    delete consolidated;
}

void
props_t::apply_local(
    void (*func)(const char *name, const char *value, void *userdata),
    void *userdata) const
{
    values_->foreach(
    	    	    (void (*)(char*, char*, void*))func,
		    userdata);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define LEFT_CURLY '{'
#define RIGHT_CURLY '}'
#define LEFT_ROUND '('
#define RIGHT_ROUND ')'

#define MAXDEPTH 100

void
props_t::expand_1(
    estring *rep,
    const char *str,
    int depth) const
{
    estring name;
    const char *value;
    char endname = '\0';

    estring_init(&name);

    for ( ; *str ; str++)
    {
    	if (endname)
	{
	    if (*str == endname)
	    {
	    	value = get(name.data);
		if (value != 0)
		{
		    if (depth == MAXDEPTH)
		    	fprintf(stderr, "Property loop detected while expanding \"%s\"\n",
			    	    	    name.data);
		    else
		    	expand_1(rep, value, depth+1);
		}
	    	estring_truncate(&name);
	    	endname = '\0';
		/* skip the closing character itself */
	    }
	    else
		estring_append_char(&name, *str);
	}
	else
	{
	    if (str[0] == '$' && str[1] == LEFT_CURLY)
	    {
	    	endname = RIGHT_CURLY;
	    	str++;	/* skip the dollar and left curly */
	    }
	    else if (str[0] == '$' && str[1] == LEFT_ROUND)
	    {
	    	endname = RIGHT_ROUND;
	    	str++;	/* skip the dollar and left round */
	    }
	    else
		estring_append_char(rep, *str);
	}
    }
}

char *
props_t::expand(const char *str) const
{
    estring rep;
    
    if (str == 0)
    	return 0;
	
    estring_init(&rep);
    
    expand_1(&rep, str, 0);
    
#if DEBUG
    fprintf(stderr, "props_t::expand: \"%s\" -> \"%s\"\n", str, rep.data);
#endif
        
    return rep.data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
props_t::read_shellfile(const char *filename)
{
    FILE *fp;
    estring name;
    estring value;
    gboolean ret = TRUE;
    int c;
    enum { SOL, COMMENT, NAME, VALUE } state = SOL;
    int lineno = 0;
    
    if ((fp = file_open_mode(filename, "r", 0)) == 0)
    	/* TODO: perror */
    	return FALSE;
    
#if DEBUG
    fprintf(stderr, "props_t::read_shellfile: \"%s\"\n", filename);
#endif

    estring_init(&name);
    estring_init(&value);
    
    while (ret && (c = fgetc(fp)) != EOF)
    {
    	switch (state)
	{
	case SOL:
	    lineno++;
	    if (c == '#' || isspace(c))
	    {
	    	state = COMMENT;
		break;
	    }
	    state = NAME;
	    /* fall through into NAME case */
	    
	case NAME:
	    if (c == '=' && name.length > 0)
	    	state = VALUE;
	    else if (isalnum(c) || c == '_' || c == '-')
	    	estring_append_char(&name, c);
	    else
	    {
	    	fprintf(stderr, "%s:%d: syntax error\n", filename, lineno);
	    	ret = FALSE;
	    }
	    break;

	case COMMENT:
	    if (c == '\n' || c == '\r')
	    	state = SOL;
	    break;
	
	case VALUE:
	    if (c == '\n' || c == '\r')
	    {
	    	if (name.length > 0)
		{
#if DEBUG
		    fprintf(stderr, "props_t::read_shellfile: \"%s\" -> \"%s\"\n",
		    	    	     name.data, value.data);
#endif
	    	    set(name.data, value.data);
		}
		estring_truncate(&name);
		estring_truncate(&value);
	    	state = SOL;
	    }
	    else
	    	estring_append_char(&value, c);
	    break;
	}
    }
    
    /* handle NAME=VALUE on last line without newline */
    if (ret && state == VALUE && name.length > 0)
    {
#if DEBUG
	fprintf(stderr, "props_t::read_shellfile: \"%s\" -> \"%s\"\n",
		    	 name.data, value.data);
#endif
    	set(name.data, value.data);
    }
    
    estring_free(&name);
    estring_free(&value);
    fclose(fp);

    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
