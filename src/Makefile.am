#
# CANT - A C implementation of the Apache/Tomcat ANT build system
# Copyright (c) 2001 Greg Banks
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# 
# $Id: Makefile.am,v 1.2 2001-11-06 09:03:23 gnb Exp $
#

bin_PROGRAMS=	cant

TASK_SOURCES=	\
		task_mkdir.c \
		task_tstamp.c \
		task_delete.c \
		task_enumerate.c \
		task_copy.c

cant_SOURCES=	common.h cant.h \
		cant.c buildfile.c \
		project.c target.c task.c \
		props.h props.c \
		estring.h estring.c \
		log.c filename.c \
		fileset.c pattern.c \
		xml.h xml.c \
		$(TASK_SOURCES)
INCLUDES=	$(GLIB_CFLAGS) $(LIBXML_CFLAGS)
cant_LDADD=	$(GLIB_LIBS) $(LIBXML_LIBS)

task.o: builtins.h

builtins.h: $(TASK_SOURCES)
	$(RM) $@
	for file in $(TASK_SOURCES) ; do \
	    sed -n -e 's|^task_ops_t[ 	][ 	]*\([a-z_][a-z_]*\).*$$|TASKOPS(\1)|p' < $$file >> $@ ;\
	done

CLEANFILES=	builtins.h
