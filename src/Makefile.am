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
# $Id: Makefile.am,v 1.33 2002-04-13 02:30:18 gnb Exp $
#

bin_PROGRAMS=	cant

TASK_SOURCES=	\
		task_mkdir.C \
		task_tstamp.C \
		task_delete.C \
		task_enumerate.C \
		task_copy.C \
		task_echo.C \
		task_cant.C \
		task_foreach.C \
		task_redirect.C \
		task_changedir.C

MAPPER_SOURCES=	\
		mapper_identity.C \
		mapper_flatten.C \
		mapper_merge.C \
		mapper_glob.C \
		mapper_regexp.C \
		mapper_null.C

cant_SOURCES=	common.H thread.H cant.H \
		cant.C buildfile.C \
		project.H project.C \
		target.H target.C \
		task.H task.C \
		taglist.H taglist.C \
		xtask.H xtask.C \
		props.H props.C \
		estring.H estring.C \
		tok.H tok.C \
		log.H log.C \
		filename.H filename.C \
		globber.H globber.C \
		fileset.H fileset.C \
		mapper.H mapper.C \
		condition.H condition.C \
		pattern.H pattern.C \
		xml.H xml.C \
		common.C process.C \
		strarray.H strarray.C \
		hashtable.H hashtable.C \
		list.H \
		queue.H queue.C \
		job.H job.C \
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

task.o: builtin-tasks.H

builtin-tasks.H: $(TASK_SOURCES)
	$(RM) $@
	for file in $(TASK_SOURCES) ; do \
	    sed -n -e 's|^[ 	]*TASK_DEFINE_CLASS_BEGIN[ 	]*([ 	]*\([a-z_][a-z_]*\).*$$|TASK_CLASS(\1)|p' < $$file >> $@ ;\
	done

mapper.o: builtin-mappers.H
builtin-mappers.H: $(MAPPER_SOURCES)
	$(RM) $@
	for file in $(MAPPER_SOURCES) ; do \
	    sed -n -e 's|^[ 	]*MAPPER_DEFINE_CLASS[ 	]*([ 	]*\([a-z_][a-z_]*\).*$$|MAPPER_CLASS(\1)|p' < $$file >> $@ ;\
	done

CLEANFILES=	builtin-tasks.H builtin-mappers.H \
		*.bb *.bbg *.da *.gcov

pkgdata_DATA=	globals.xml
EXTRA_DIST=	$(pkgdata_DATA)


####
GCOV=		gcov
GCOVFLAGS=	-b -f

post-gcov:
	for file in $(SOURCES) ; do \
	    case "$$file" in \
	    *.C) echo "==> $$file" ; $(GCOV) $(GCOVFLAGS) $$file ;; \
	    esac ;\
	done

pre-gcov:
	$(RM) *.da *.gcov



####
# Test programs

noinst_PROGRAMS=		normalise_test

normalise_test_SOURCES=		normalise_test.C \
				filename.H filename.C \
				estring.H estring.C \
				tok.H tok.C \
				log.H log.C \
				common.H common.C
normalise_test_LDADD=		$(GLIB_LIBS)
