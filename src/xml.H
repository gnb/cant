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

#ifndef _cant_xml_h_
#define _cant_xml_h_ 1

#include "common.H"
#include <tree.h>
#include <xmlmemory.h>
#include "log.H"

/* TODO: This macro would be non-trivial if glib and xml used different free()s */
#define xml2g(x)    (x)

/*
 * Much silly casting is required to sanely work around
 * libxml's stupid typedef of xmlChar to unsigned char.
 */

#define cantXmlAttrName(at) \
    	    ((char *)(at)->name)
#define cantXmlAttrValue(at) \
    	    (char *)xmlNodeListGetString((at)->node->doc, (at)->val, /*inLine*/TRUE)

gboolean cantXmlGetBooleanProp(xmlNode *node, const char *name, gboolean deflt);

#define cantXmlGetProp(node, name) \
    	    (char *)xmlGetProp((node), (const xmlChar *)(name))
#define cantXmlNodeGetName(node) \
    	    ((char *)(node)->name)
#define cantXmlNodeGetContent(node) \
    	    (char *)xmlNodeGetContent(node)

typedef struct
{
    const char *filename;   	/* points into `filenames' hashtable */
    int lineno;
} node_info_t;

xmlDoc *cantXmlParseFile(const char *filename);
const node_info_t *cantXmlNodeInfoGet(const xmlNode *node);
void cantXmlNodeInfoClear(void);


class log_node_context_t : public log_context_t
{
private:
    const xmlNode *node_;

    char *format();
    void format(FILE *);

public:
    log_node_context_t(const xmlNode *node)
     :  node_(node)
    {
    }
};

extern void log_error_unknown_attribute(const xmlAttr *attr);
extern void log_error_required_attribute(const xmlNode *node, const char *attrname);
extern void log_error_unexpected_element(const xmlNode *node);


#endif /* _cant_xml_h_ */
