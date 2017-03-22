
POEDIT_LINGUAS = an ar az be be@latin bg bs ca ckb co cs da de el en_GB es et eu fa fi fr ga gl he hr hu hy id is it ja ka kab kk ko lt lv ms nb nl oc pa pl pt_BR pt_PT ro ru sk sl sq sr sv tg th tr uk uz vi zh_CN zh_TW

localedir = $(datadir)/locale

install-data-local: install-poedit-catalogs

install-poedit-catalogs:
	for i in $(POEDIT_LINGUAS) ; do \
	    $(mkinstalldirs) $(DESTDIR)$(localedir)/$$i/LC_MESSAGES ; \
	    $(INSTALL_DATA) $(srcdir)/$$i.mo $(DESTDIR)$(localedir)/$$i/LC_MESSAGES/poedit.mo ; \
	done

uninstall-local:
	rm -rf $(DESTDIR)$(localedir)/*/LC_MESSAGES/poedit.mo

# ----------------------------------------------------------------------------
# Logic for catalogs updating follows
# (shamelessly stolen from wxWidgets makefile):
# ----------------------------------------------------------------------------


# the programs we use (TODO: use configure to detect them)
MSGFMT=msgfmt --verbose --check
MSGMERGE=msgmerge
XGETTEXT=xgettext
XARGS=xargs

# common xgettext args: C++ syntax, use the specified macro names as markers
XGETTEXT_ARGS=-C -k_ -kwxGetTranslation -kwxTRANSLATE -kwxPLURAL:1,2 -F -j \
              --add-comments=TRANSLATORS \
              --from-code=UTF-8 \
              --package-name=Poedit --package-version=$(PACKAGE_SHORT_VERSION) \
              --msgid-bugs-address=help@poedit.net

# implicit rules
%.mo: %.po
	$(MSGFMT) -o $@ $<

# a PO file must be updated from  poedit.pot include new translations
%.po: $(srcdir)/poedit.pot
	if [ -f $@ ]; then $(MSGMERGE) --previous $@ $(srcdir)/poedit.pot > $@.new && mv $@.new $@; else cp $(srcdir)/poedit.pot $@; fi

$(srcdir)/sr_RS@latin.po: $(srcdir)/sr.po
	recode-sr-latin <$< >$@

$(srcdir)/poedit.pot:
	touch $@
	(cd $(srcdir) ; find ../src -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o poedit.pot)
	(cd $(srcdir) ; find ../src -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o poedit.pot)
	(cd $(srcdir) ; $(WXRC) --gettext ../src/resources/*.xrc | $(XGETTEXT) $(XGETTEXT_ARGS) -o poedit.pot -)

allpo: force-update
	@-for t in $(POEDIT_LINGUAS); do $(MAKE) $(srcdir)/$$t.po; done

allmo:
	@for t in $(POEDIT_LINGUAS); do $(MAKE) $(srcdir)/$$t.mo; done

force-update:
	$(RM) $(srcdir)/poedit.pot

# print out the percentage of the translated strings
stats:
	@for i in $(POEDIT_LINGUAS); do \
		x=`$(MSGFMT) -o /dev/null "$(srcdir)/$$i.po" 2>&1 | sed -e 's/[,\.]//g' \
			-e 's/\([0-9]\+\) translated messages\?/TR=\1/' \
			-e 's/\([0-9]\+\) fuzzy translations\?/FZ=\1/' \
			-e 's/\([0-9]\+\) untranslated messages\?/UT=\1/'`; \
		TR=0 FZ=0 UT=0; \
		eval $$x; \
		TOTAL=`expr $$TR + $$FZ + $$UT`; \
		echo "\"$$i\" => \"`expr 100 "*" $$TR / $$TOTAL`\", /* $$TOTAL strings */"; \
	done

#echo "$$i.po `expr 100 "*" $$TR / $$TOTAL`% of $$TOTAL strings"; 

dist-hook: allmo
	cp -a $(srcdir)/*.pot $(srcdir)/*.po $(srcdir)/*.mo $(distdir)

.PHONY: allpo allmo force-update stats FORCE
