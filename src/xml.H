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

/*
 * A large part of the job of the libxml C++ wrapper
 * is to do the silly casting which is required to sanely
 * work around libxml's stupid typedef of xmlChar to unsigned
 * char.  Another part is fixing libxml's stupidly inconsistent
 * naming, e.g. the same concept is sometimes "property" and
 * sometimes "attribute".  And finally, const-correctness.
 */

class xml_attribute_t : public xmlAttr
{
private:
    xml_attribute_t();   	// can't instantiate
    ~xml_attribute_t();  	// can't destroy

    friend class xml_node_t;
    
public:
    const char *get_name() const
    {
    	return (const char *)name;
    }
    
    char *get_value() const
    {
    	return (char *)xmlNodeListGetString(node->doc, val, /*inLine*/TRUE);
    }
    
    xml_node_t *get_node() const
    {
    	return (xml_node_t *)node;
    }

    void error_unknown_attribute() const;
};


template<class T> class xml_iterator_t
{
private:
    T *ptr_;
    
    xml_iterator_t(T *ptr)
     :  ptr_(ptr)
    {
    }
    
    friend class xml_node_t;
    
public:
    xml_iterator_t()
     :  ptr_(0)
    {
    }
    
    T *operator++()
    {
    	T *p = ptr_;
	if (ptr_ != 0)
	    ptr_ = (T *)ptr_->next;
	return p;
    }
    
    T *operator--()
    {
    	T *p = 0;
	if (ptr_ != 0)
	    ptr_ = (T *)ptr_->prev;
	return p;
    }
    
    T *operator*()
    {
    	return ptr_;
    }
    
    xml_iterator_t<T> peek_next() const
    {
    	return xml_iterator_t<T>(ptr_ == 0 ? 0 : ptr_->next);
    }
    xml_iterator_t<T> peek_prev() const
    {
    	return xml_iterator_t<T>(ptr_ == 0 ? 0 : ptr_->prev);
    }

    int operator==(T *item)
    {
    	return (item == ptr_);
    }
    int operator!=(T *item)
    {
    	return (item != ptr_);
    }
};


class xml_node_t : public xmlNode
{
private:
    xml_node_t();   	// can't instantiate
    ~xml_node_t();  	// can't destroy

    friend class shut_up_silly_compiler;
        
public:
    const char *get_name() const
    {
    	return (const char *)name;
    }
    
    xmlElementType get_type() const
    {
    	return type;
    }
    
    char *get_content() const
    {
    	return (char *)xmlNodeGetContent((xmlNode *)this);
    }
    
    char *get_attribute(const char *attrname) const
    {
    	return (char *)xmlGetProp((xmlNode *)this, (const xmlChar *)attrname);
    }

    gboolean get_boolean_attribute(const char *attrname, gboolean deflt) const;
    
    void error_required_attribute(const char *attrname) const;
    void error_unexpected_element() const;
    
    static void info_init();
    static void info_clear();
    typedef struct
    {
	const char *filename;   	/* points into `filenames' hashtable */
	int lineno;
    } info_t;
    const info_t *info_get() const;
    info_t *info_insert(const char *filename, int lineno);
    
    xml_iterator_t<xml_node_t> first_child() const
    {
    	return xml_iterator_t<xml_node_t>((xml_node_t *)childs);
    }
    xml_iterator_t<xml_attribute_t> first_attribute() const
    {
    	return xml_iterator_t<xml_attribute_t>((xml_attribute_t *)properties);
    }
};


xmlDoc *cantXmlParseFile(const char *filename);


class log_node_context_t : public log_context_t
{
private:
    const xml_node_t *node_;

    char *format();
    void format(FILE *);

public:
    log_node_context_t(const xml_node_t *node)
     :  node_(node)
    {
    }
};

extern void log_error_unknown_attribute(const xmlAttr *attr);


#endif /* _cant_xml_h_ */
