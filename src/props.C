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

struct props_s
{
    props_t *parent;	    /* inherits values from here */
    GHashTable *values;
};

CVSID("$Id: props.C,v 1.1 2002-03-29 12:36:26 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

props_t *
props_new(props_t *parent)
{
    props_t *props;
    
    props = new(props_t);

    props->parent = parent;
    props->values = g_hash_table_new(g_str_hash, g_str_equal);
    
    return props;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
_props_copy_one(gpointer key, gpointer value, gpointer userdata)
{
    props_set((props_t *)userdata, (const char *)key, (const char *)value);
}

void
props_copy_contents(props_t *props, const props_t *orig)
{
    g_hash_table_foreach(orig->values, _props_copy_one, props);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
_props_delete_one_value(gpointer key, gpointer value, gpointer userdata)
{
    g_free(key);
    g_free(value);
    return TRUE;    /* so remove me already */
}

void
props_delete(props_t *props)
{
    g_hash_table_foreach_remove(props->values, _props_delete_one_value, 0);
    g_free(props);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
props_get(const props_t *props, const char *name)
{
    const char *value;
    
    for ( ; props != 0 ; props = props->parent)
    {
    	if ((value = (const char *)g_hash_table_lookup(props->values, name)) != 0)
	    return value;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
props_setm(props_t *props, const char *name, char *value)
{
    gpointer okey = 0, ovalue = 0;
    
    assert(name != 0);
    
    if (value == 0)
    {
	g_hash_table_remove(props->values, name);
	return;
    }
    
    if (g_hash_table_lookup_extended(props->values, name, &okey, &ovalue))
    {
    	/* remove the old value */
	g_hash_table_remove(props->values, okey);
	g_free(okey);
	g_free(ovalue);
    }
    g_hash_table_insert(props->values, g_strdup(name), value);
}

void
props_set(props_t *props, const char *name, const char *value)
{
    props_setm(props, name, (value == 0 ? 0 : g_strdup(value)));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
_props_consolidate_one(gpointer key, gpointer value, gpointer userdata)
{
    GHashTable *consolidated = (GHashTable *)userdata;
    
    if (g_hash_table_lookup(consolidated, key) == 0)
    	g_hash_table_insert(consolidated, key, value);
}

void
props_apply(
    const props_t *props,
    void (*func)(const char *name, const char *value, void *userdata),
    void *userdata)
{
    GHashTable *consolidated;
    
    /*
     * Have to build a consolidated hash table to avoid
     * the case where a property is iterated over twice
     * because it appears both in this props_t and in
     * one of its ancestors.
     */
    consolidated = g_hash_table_new(g_str_hash, g_str_equal);

    for ( ; props != 0 ; props = props->parent)
	g_hash_table_foreach(props->values, _props_consolidate_one, consolidated);
    
    g_hash_table_foreach(consolidated, (GHFunc)func, userdata);
    
    g_hash_table_destroy(consolidated);
}

void
props_apply_local(
    const props_t *props,
    void (*func)(const char *name, const char *value, void *userdata),
    void *userdata)
{
    g_hash_table_foreach(props->values, (GHFunc)func, userdata);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define LEFT_CURLY '{'
#define RIGHT_CURLY '}'
#define LEFT_ROUND '('
#define RIGHT_ROUND ')'

#define MAXDEPTH 100

static void
_props_expand_1(
    const props_t *props,
    estring *rep,
    const char *str,
    int depth)
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
	    	value = props_get(props, name.data);
		if (value != 0)
		{
		    if (depth == MAXDEPTH)
		    	fprintf(stderr, "Property loop detected while expanding \"%s\"\n",
			    	    	    name.data);
		    else
		    	_props_expand_1(props, rep, value, depth+1);
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
props_expand(const props_t *props, const char *str)
{
    estring rep;
    
    if (str == 0)
    	return 0;
	
    estring_init(&rep);
    
    _props_expand_1(props, &rep, str, 0);
    
#if DEBUG
    fprintf(stderr, "props_expand: \"%s\" -> \"%s\"\n", str, rep.data);
#endif
        
    return rep.data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
props_read_shellfile(props_t *props, const char *filename)
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
    fprintf(stderr, "props_read_shellfile: \"%s\"\n", filename);
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
		    fprintf(stderr, "props_read_shellfile: \"%s\" -> \"%s\"\n",
		    	    	     name.data, value.data);
#endif
	    	    props_set(props, name.data, value.data);
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
	fprintf(stderr, "props_read_shellfile: \"%s\" -> \"%s\"\n",
		    	 name.data, value.data);
#endif
    	props_set(props, name.data, value.data);
    }
    
    estring_free(&name);
    estring_free(&value);
    fclose(fp);

    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/