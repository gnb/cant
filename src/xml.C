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
#include "hashtable.H"
#include <parser.h>
#include <SAX.h>

CVSID("$Id: xml.C,v 1.4 2002-04-12 14:28:21 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
log_node_context_t::format()
{
    const xml_node_t::info_t *ni;

    if (node_ != 0 && (ni = node_->info_get()) != 0)
    	return g_strdup_printf("%s:%d: ", ni->filename, ni->lineno);
    return 0;
}

void
log_node_context_t::format(FILE *fp)
{
    const xml_node_t::info_t *ni;

    if (node_ != 0 && (ni = node_->info_get()) != 0)
	fprintf(fp, "%s:%d: ", ni->filename, ni->lineno);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
xml_attribute_t::error_unknown_attribute() const
{
    log_node_context_t context(get_node());
    
    log::errorf("Unknown attribute \"%s\" on \"%s\"\n",
    	    	get_name(), get_node()->get_name());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
xml_node_t::error_required_attribute(const char *attrname) const
{
    log_node_context_t context(this);
    
    log::errorf("Required attribute \"%s\" missing from \"%s\"\n",
    	    	attrname, get_name());
}

void
xml_node_t::error_unexpected_element() const
{
    log_node_context_t context(this);
    
    log::errorf("Element \"%s\" unexpected at this point\n", get_name());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
xml_node_t::get_boolean_attribute(const char *attrname, gboolean deflt) const
{
    char *value;
    int res;
    
    if ((value = get_attribute(attrname)) == 0)
    	return deflt;
	
    if ((res = strbool(value, -1)) < 0)
    {
	log_node_context_t context(this);
    	log::errorf("Attribute \"%s\" is not a recognisable boolean\n", attrname);
    	res = deflt;
    }
    
    g_free(value);
    
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GHashFunc hashtable_ops_t<const xml_node_t*>::hash = g_direct_hash;
GCompareFunc hashtable_ops_t<const xml_node_t*>::compare = g_direct_equal;

/* Data structures used to keep track of node file:line information */
static hashtable_t<const char*, char> *filenames;	/* uniquified filename storage */
static hashtable_t<const xml_node_t*, xml_node_t::info_t> *node_infos;

static gboolean
remove_one_filename(const char *key, char *value, void *closure)
{
    g_free(value);
    return TRUE;    /* remove me */
}

static gboolean
remove_one_node_info(const xml_node_t *key, xml_node_t::info_t *value, void *closure)
{
    g_free(value);
    return TRUE;    /* remove me */
}


void
xml_node_t::info_init(void)
{
    if (filenames == 0)
    {
    	filenames = new hashtable_t<const char*, char>;
    	node_infos = new hashtable_t<const xml_node_t*, xml_node_t::info_t>;
    }
}

void
xml_node_t::info_clear(void)
{
    if (filenames !=  0)
    	filenames->foreach_remove(remove_one_filename, 0);
    if (node_infos !=  0)
    	node_infos->foreach_remove(remove_one_node_info, 0);
}

xml_node_t::info_t *
xml_node_t::info_insert(const char *filename, int lineno)
{
    char *savedfn;
    info_t *ni;
    
    if ((savedfn = filenames->lookup(filename)) == 0)
    {
    	savedfn = g_strdup(filename);
	filenames->insert(savedfn, savedfn);
    }
    
    ni = new(info_t);
    ni->filename = savedfn;
    ni->lineno = lineno;
    
    node_infos->insert(this, ni);
    
#if DEBUG > 10
    fprintf(stderr, "xml_node_t::info_insert: 0x%08lx -> %s:%d\n",
    	    (unsigned long)this, ni->filename, ni->lineno);
#endif
    
    return ni;
}

const xml_node_t::info_t *
xml_node_t::info_get() const
{
    return node_infos->lookup(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sax_error(void *ctx, const char *fmt, ...)
{
    xmlParserCtxt *parser_context = (xmlParserCtxt *)ctx;
    va_list args;
    
    log_file_context_t context(
    	    parser_context->input->filename,
	    parser_context->input->line);

    va_start(args, fmt);
    log::messagev(log::ERROR, fmt, args);
    va_end(args);
}

static void
sax_start_element(void *ctx, const xmlChar *name, const xmlChar **atts)
{
    xmlParserCtxt *parser_context = (xmlParserCtxt *)ctx;
    
#if DEBUG > 20
    printf("start_element: ctx=0x%08lx, file=\"%s\" line=%d name=\"%s\"\n",
    	    (unsigned long)ctx,
    	    parser_context->input->filename,
	    parser_context->input->line,
	    name);
#endif

    startElement(ctx, name, atts);

    ((xml_node_t *)parser_context->node)->info_insert(
    	    parser_context->input->filename,
	    parser_context->input->line);
}


xmlDoc *
cantXmlParseFile(const char *filename)
{
    xmlDoc *doc;
    xmlSAXHandler sax_handler;
    xmlParserCtxt *parser_context;
    FILE *fp;
    int n;
    char buf[1024];

    if ((fp = fopen(filename, "r")) == 0)
    {
    	log::perror(filename);
    	return 0;
    }

    xmlDefaultSAXHandlerInit();
    sax_handler = xmlDefaultSAXHandler;
    sax_handler.startElement = sax_start_element;
    sax_handler.error = sax_error;
    sax_handler.fatalError = sax_error;

    xml_node_t::info_init();    
    parser_context = xmlCreatePushParserCtxt(&sax_handler,
    	    	    	/*user_data*/0, /*chunk*/0, /*size*/0, filename);
    
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
    	xmlParseChunk(parser_context, buf, n, /*terminate*/FALSE);

    xmlParseChunk(parser_context, 0, 0, /*terminate*/TRUE);

    fclose(fp);
    doc = parser_context->myDoc;
    xmlFreeParserCtxt(parser_context);
 
    return doc;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
