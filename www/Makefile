
# Pages which provide backwards compatibility for old URLs
PAGES=		index.html download.html changelog.html
IMAGES=		cant_banner.gif fm.mini.jpg gimp.gif \
		valid-html40.gif borderl.gif borderr.gif
BINARIES:=	$(shell m4 list-binaries.m4)	

DELIVERABLES=	$(PAGES) $(IMAGES) $(BINARIES)

ENABLE_COUNT=	1

############################################################

all:: $(PAGES)

changelog.html: _changelist.html changes2html
$(PAGES): _styles.html _common.m4 _copyright.html toc.html.in
download.html: version.m4 _download.m4

_changelist.html: ../ChangeLog
	./changes2html < $< > $@

%.html: %.html.in
	m4 -DHTMLFILE=$@ -DENABLE_COUNT=$(ENABLE_COUNT) $< > $@
	
clean::
	$(RM) $(patsubst %.html.in,%.html,$(wildcard $(patsubst %.html,%.html.in,$(PAGES))))
	$(RM) _changelist.html
			
############################################################

INSTALL=	install -c
htmldir=	$(HOME)/public_html/alphalink/cant
uploaddir=	shell.alphalink.com.au:public_html

install:: installdirs $(addprefix $(htmldir)/,$(notdir $(DELIVERABLES)))

installdirs:
	test -d $(htmldir) || $(INSTALL) -d $(htmldir)
	
$(htmldir)/%: %
	$(INSTALL) -m 644 $< $@

############################################################

upload:
	rsync -v -r --delete -e ssh --rsync-path=/home/g/gnb/inst/bin/rsync $(htmldir) $(uploaddir)
