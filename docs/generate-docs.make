
ASCIIDOC = asciidoc
XSLTPROC = xsltproc
XMLTO = xmlto

all: poedit.1

clean:
	rm -f poedit.1

poedit.1: poedit_man.txt man_asciidoc.conf man_fix.xsl
	$(ASCIIDOC) -d manpage -f man_asciidoc.conf -b docbook -o - poedit_man.txt \
		| $(XSLTPROC) -o poedit.1.xml man_fix.xsl -
	$(XMLTO) man poedit.1.xml
	rm poedit.1.xml
