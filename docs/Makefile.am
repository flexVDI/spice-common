NULL =
ASCIIDOC_FLAGS = -a icons -a toc

EXTRA_DIST =					\
	spice_protocol.html			\
	spice_protocol.txt			\
	$(NULL)

if BUILD_HTML_MANUAL
all-local: spice_protocol.html

spice_protocol.html: spice_protocol.txt
	$(AM_V_GEN) $(ASCIIDOC) -n $(ASCIIDOC_FLAGS) -o $@ $<
endif

clean-local:
	rm -f spice_protocol.html
