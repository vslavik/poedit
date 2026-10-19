
ASCIIDOCTOR = asciidoctor

all: poedit.1

clean:
	rm -f poedit.1

poedit.1: poedit_man.txt generate-docs.make
	$(ASCIIDOCTOR) -d manpage -b manpage -o $@ $<
