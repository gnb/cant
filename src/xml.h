/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * Copyright (c) 2001 Greg Banks
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

#include "common.h"
#include <tree.h>
#include <xmlmemory.h>

/* TODO: This macro would be non-trivial if glib and xml used different free()s */
#define xml2g(x)    (x)

#define cantXmlAttrValue(at) \
    	    xmlNodeListGetString((at)->node->doc, (at)->val, /*inLine*/TRUE)

gboolean cantXmlGetBooleanProp(xmlNode *node, const char *name, gboolean deflt);
gboolean cantXmlStringToBoolean(const char *val, gboolean deflt);

#define boolassign(bv, s) \
    (bv) = cantXmlStringToBoolean((s), (bv))


#endif /* _cant_xml_h_ */
