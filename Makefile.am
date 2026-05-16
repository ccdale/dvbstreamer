## DVBStreamer main Makefile

SUBDIRS = include include/dvbpsi src/dvbpsi src src/plugins

dvbstreamerdocdir = ${prefix}/doc/DVBStreamer
dvbstreamerdoc_DATA = \
	README.md\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO

nodist_pkginclude_HEADERS = \
	config.h

EXTRA_DIST = $(dvbstreamerdoc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

