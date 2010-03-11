
# For dist target.
noinst_HEADERS =  \
        fat.h ntfs.h filesys.h freebsd.h fs.h   \
	     mb_header.h mb_info.h  \
	  pc_slice.h   shared.h   term.h

EXTRA_DIST = setjmp.S apm.S $(noinst_SCRIPTS)

# For <stage1.h>.
INCLUDES = -I$(top_srcdir)/stage1



pkglib_DATA = stage2
noinst_DATA = pre_stage2
noinst_PROGRAMS = pre_stage2.exec


MOSTLYCLEANFILES = $(noinst_PROGRAMS)

PRE_STAGE2_LINK = -nostdlib -Wl,-N -Wl,-Ttext -Wl,8200


STAGE2_COMPILE = $(STAGE2_CFLAGS) -fno-builtin
	



# For stage2 target.
pre_stage2_exec_SOURCES = asm.S bios.c builtins.c char_io.c \
	cmdline.c common.c console.c disk_io.c fsys_ext2fs.c \
	fsys_fat.c fsys_ntfs.c \
	fsys_reiserfs.c stage2.c
pre_stage2_exec_CFLAGS = $(STAGE2_COMPILE) $(FSYS_CFLAGS)
pre_stage2_exec_CCASFLAGS = $(STAGE2_COMPILE) $(FSYS_CFLAGS)
pre_stage2_exec_LDFLAGS = $(PRE_STAGE2_LINK)



BUILT_SOURCES = stage2_size.h


CLEANFILES = $(pkglib_DATA) $(noinst_DATA) $(BUILT_SOURCES)

stage2_size.h: pre_stage2
	-rm -f stage2_size.h
	set dummy `ls -l pre_stage2`; \
	echo "#define STAGE2_SIZE $$6" > stage2_size.h



# XXX: automake doesn't provide a way to specify dependencies for object
# files explicitly, so we must write this by a general Makefile scheme.
# If automake change the naming scheme for per-executable objects, this
# will be broken.

stage2: pre_stage2 63s_grub_cfg
	-rm -f stage2
	cat 63s_grub_cfg pre_stage2 > stage2

# General rule for making a raw binary.
SUFFIXES = .exec
.exec:
	$(OBJCOPY) -O binary $< $@
