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
#include <parser.h>
#include <SAX.h>

CVSID("$Id: xml.C,v 1.1 2002-03-29 12:36:27 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cantXmlGetBooleanProp(xmlNode *node, const char *name, gboolean deflt)
{
    char *value;
    int res;
    
    if ((value = cantXmlGetProp(node, name)) == 0)
    	return deflt;
	
    if ((res = strbool(value, -1)) < 0)
    {
    	parse_node_error(node, "attribute \"%s\" is not a recognisable boolean\n", name);
    	res = deflt;
    }
    
    xmlFree(value);
    
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* Data structures used to keep track of node file:line information */
static GHashTable *filenames;	/* uniquified filename storage */
static GHashTable *node_infos;

static gboolean
remove_one_filename(gpointer key, gpointer value, gpointer userdata)
{
    g_free(value);
    return TRUE;    /* remove me */
}

static gboolean
remove_one_node_info(gpointer key, gpointer value, gpointer userdata)
{
    node_info_t *ni = (node_info_t *)value;
    
    g_free(ni);
    return TRUE;    /* remove me */
}


static void
node_info_init(void)
{
    if (filenames == 0)
    {
    	filenames = g_hash_table_new(g_str_hash, g_str_equal);
    	node_infos = g_hash_table_new(g_direct_hash, g_direct_equal);
    }
}

void
cantXmlNodeInfoClear(void)
{
    if (filenames !=  0)
    	g_hash_table_foreach_remove(filenames, remove_one_filename, 0);
    if (node_infos !=  0)
    	g_hash_table_foreach_remove(node_infos, remove_one_node_info, 0);
}

static node_info_t *
node_info_insert(xmlNode *node, const char *filename, int lineno)
{
    char *savedfn;
    node_info_t *ni;
    
    if ((savedfn = (char *)g_hash_table_lookup(filenames, filename)) == 0)
    {
    	savedfn = g_strdup(filename);
	g_hash_table_insert(filenames, savedfn, savedfn);
    }
    
    ni = new(node_info_t);
    ni->filename = savedfn;
    ni->lineno = lineno;
    
    g_hash_table_insert(node_infos, node, ni);
    
#if DEBUG > 10
    fprintf(stderr, "node_info_insert: 0x%08lx -> %s:%d\n",
    	    (unsigned long)node, ni->filename, ni->lineno);
#endif
    
    return ni;
}

const node_info_t *
cantXmlNodeInfoGet(const xmlNode *node)
{
    return (node == 0 ? 0 : (const node_info_t *)g_hash_table_lookup(node_infos, (gpointer)node));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sax_error(void *ctx, const char *fmt, ...)
{
    xmlParserCtxt *parser_context = (xmlParserCtxt *)ctx;
    va_list args;
    
    fprintf(stderr, "%s:%d: ",
    	    parser_context->input->filename,
	    parser_context->input->line);
    fprintf(stderr, "ERROR: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
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

    node_info_insert(
    	    parser_context->node, 
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
    	log_perror(filename);
    	return 0;
    }

    xmlDefaultSAXHandlerInit();
    sax_handler = xmlDefaultSAXHandler;
    sax_handler.startElement = sax_start_element;
    sax_handler.error = sax_error;
    sax_handler.fatalError = sax_error;

    node_info_init();    
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