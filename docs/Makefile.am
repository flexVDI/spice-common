NULL =
ASCIIDOC_FLAGS = -a icons -a toc

EXTRA_DIST =					\
	spice_protocol.txt			\
	$(NULL)

if BUILD_HTML_MANUAL
all-local: spice_protocol.html

spice_protocol.html: spice_protocol.txt
	$(AM_V_GEN) $(ASCIIDOC) -n $(ASCIIDOC_FLAGS) -o $@ $<
endif

CLEANFILES = spice_protocol.html

-include $(top_srcdir)/git.mk
