ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
 
AM_CPPFLAGS = -D_FILE_OFFSET_BITS=64

bin_PROGRAMS = mkfs.nanofs nanofs.dump nanofuse

# Shared library
noinst_LIBRARIES = libnanofs.a
libnanofs_a_SOURCES = log.h log.c\
	nanofs_filedir.h nanofs_filedir.c\
	nanofs_io.h nanofs_io.c nanofs.h

# Utilities for manage file system
mkfs_nanofs_SOURCES = mknanofs.c
nanofs_dump_SOURCES = nanofsdump.c
mkfs_nanofs_LDADD = libnanofs.a
nanofs_dump_LDADD = libnanofs.a

# Nanofuse
nanofuse_LDADD = libnanofs.a $(FUSE_LIBS) 
nanofuse_SOURCES = nanofuse.c nanofuse.h
