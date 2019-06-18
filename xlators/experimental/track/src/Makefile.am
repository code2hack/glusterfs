
xlator_LTLIBRARIES = track.la
xlatordir = $(libdir)/glusterfs/$(PACKAGE_VERSION)/xlator/experimental

track_la_LDFLAGS = -module $(GF_XLATOR_DEFAULT_LDFLAGS)

track_la_SOURCES = track.c ring-buff.c req2vec.c
track_la_LIBADD = $(top_builddir)/libglusterfs/src/libglusterfs.la

noinst_HEADERS = track.h ring-buff.h req2vec.h #track-mem-types.h
AM_CPPFLAGS = $(GF_CPPFLAGS) -I$(top_srcdir)/libglusterfs/src \
	-I$(top_srcdir)/rpc/xdr/src -I$(top_builddir)/rpc/xdr/src

AM_CFLAGS = -Wall $(GF_CFLAGS)

CLEANFILES = 

