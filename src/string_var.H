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

#ifndef _string_var_h_
#define _string_var_h_

#include "common.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class string_var
{
private:
    char *data_;

public:
    // default ctor
    string_var()
     :  data_(0)
    {
    }
    // copy assign c'tor
    string_var(const char *s)
    {
    	data_ = (s == 0 ? 0 : g_strdup(s));
    }
    // non-copy assign c'tor
    string_var(char *s)
    {
    	data_ = (s == 0 ? 0 : s);
    }
    // copy c'tor
    string_var(const string_var &o)
    {
    	data_ = (o.data_ == 0 ? 0 : g_strdup(o.data_));
    }
    // dtor    
    ~string_var()
    {
	if (data_ != 0)
	{
	    g_free(data_);
	    data_ = 0;
	}
    }

    const char *data() const
    {
    	return data_;
    }
    
    operator const char *() const
    {
    	return data_;
    }
    
    // assignment operators
    string_var &operator=(const char *s)
    {
    	if (data_ != 0)
	    g_free(data_);
	data_ = (s == 0 ? 0 : g_strdup(s));
	return *this;
    }
    string_var &operator=(char *s)
    {
    	if (data_ != 0)
	    g_free(data_);
	data_ = (s == 0 ? 0 : s);
	return *this;
    }
    
    gboolean is_null() const
    {
    	return (data_ == 0 || *data_ == '\0');
    }
    
    char *take()
    {
    	char *d = data_;
	data_ = 0;
	return d;
    }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _string_var_h_ */
