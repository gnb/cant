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

#include "cant.h"

CVSID("$Id: xml.c,v 1.5 2001-11-13 04:08:06 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cantXmlStringToBoolean(const char *value, gboolean deflt)
{
    gboolean res = deflt;
    
    if (!strcasecmp(value, "true") ||
    	!strcasecmp(value, "yes") ||
    	!strcasecmp(value, "off"))
    	res = TRUE;
    else if (!strcasecmp(value, "false") ||
    	     !strcasecmp(value, "no") ||
    	     !strcasecmp(value, "off"))
    	res = FALSE;
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cantXmlGetBooleanProp(xmlNode *node, const char *name, gboolean deflt)
{
    char *value;
    gboolean res;
    
    if ((value = xmlGetProp(node, name)) == 0)
    	return deflt;
	
    res = deflt;
    if (!strcasecmp(value, "true") ||
    	!strcasecmp(value, "yes") ||
    	!strcasecmp(value, "off"))
    	res = TRUE;
    else if (!strcasecmp(value, "false") ||
    	     !strcasecmp(value, "no") ||
    	     !strcasecmp(value, "off"))
    	res = FALSE;
    else
    	parse_error("attribute \"%s\" should be one of \"true\" or \"false\"\n", name);
	
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
