#
# CANT - A C implementation of the Apache/Tomcat ANT build system
# Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
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
# $Id: Makefile.am,v 1.19 2002-02-04 04:57:55 gnb Exp $
#

bin_PROGRAMS=	cant

TASK_SOURCES=	\
		task_mkdir.c \
		task_tstamp.c \
		task_delete.c \
		task_enumerate.c \
		task_copy.c \
		task_echo.c \
		task_cant.c

MAPPER_SOURCES=	\
		mapper_identity.c \
		mapper_flatten.c \
		mapper_merge.c \
		mapper_glob.c \
		mapper_regexp.c \
		mapper_null.c

cant_SOURCES=	common.h thread.h cant.h \
		cant.c buildfile.c project.c target.c task.c \
		taglist.c \
		xtask.h xtask.c \
		props.h props.c \
		estring.h estring.c \
		log.h log.c \
		filename.h filename.c \
		fileset.h fileset.c \
		mapper.h mapper.c \
		condition.h condition.c \
		pattern.h pattern.c \
		xml.h xml.c \
		common.c process.c \
		strarray.h strarray.c \
		queue.h queue.c \
		job.h job.c \
		$(TASK_SOURCES) \
		$(MAPPER_SOURCES)
INCLUDES=	$(GLIB_CFLAGS) $(LIBXML_CFLAGS) $(THREADS_CFLAGS)
cant_LDADD=	$(GLIB_LIBS) $(LIBXML_LIBS) $(THREADS_LIBS)
CPPFLAGS=	-DPKGDATADIR="\"$(pkgdatadir)\"" -DDEBUG=$(DEBUG)

# Note this is an automake conditional *not* a gmake conditional
if DEBUG
DEBUG=1
else
DEBUG=0
endif

task.o: builtin-tasks.h

builtin-tasks.h: $(TASK_SOURCES)
	$(RM) $@
	for file in $(TASK_SOURCES) ; do \
	    sed -n -e 's|^task_ops_t[ 	][ 	]*\([a-z_][a-z_]*\).*$$|TASKOPS(\1)|p' < $$file >> $@ ;\
	done

mapper.o: builtin-mappers.h
builtin-mappers.h: $(MAPPER_SOURCES)
	$(RM) $@
	for file in $(MAPPER_SOURCES) ; do \
	    sed -n -e 's|^mapper_ops_t[ 	][ 	]*\([a-z_][a-z_]*\).*$$|MAPPEROPS(\1)|p' < $$file >> $@ ;\
	done

CLEANFILES=	builtin-tasks.h builtin-mappers.h \
		*.bb *.bbg *.da *.gcov

pkgdata_DATA=	globals.xml
EXTRA_DIST=	$(pkgdata_DATA)


####
GCOV=		gcov
GCOVFLAGS=	-b -f

post-gcov:
	for file in $(SOURCES) ; do \
	    case "$$file" in \
	    *.c) echo "==> $$file" ; $(GCOV) $(GCOVFLAGS) $$file ;; \
	    esac ;\
	done

pre-gcov:
	$(RM) *.da *.gcov
