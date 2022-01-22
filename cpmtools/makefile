#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# $BeginLicense$
#
# $EndLicense$
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Makefile for cpmtools, using MingW 
#

CC=	        gcc
CFLAGS=         -DMINGW -DNEED_NCURSES -DHAVE_NCURSES_NCURSES_H -O -c -DFORMAT=\"ibm-3740\"
LDFLAGS=
LIBCURSES=	-lncurses 

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ALL=	bin/cpmls.exe 		\
	bin/cpmrm.exe 		\
	bin/cpmcp.exe 		\
	bin/cpmchmod.exe	\
	bin/cpmchattr.exe 	\
        bin/mkfs.cpm.exe 	\
	bin/fsck.cpm.exe 	\
	bin/fsed.cpm.exe

all: $(ALL)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

build/%.o: src/%.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@
	@echo "CC $<"

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bin/cpmls.exe:			        \
	build/cpmls.o 		        \
	build/cpmfs.o 		        \
	build/device_win32.o
	@$(CC) $(LDFLAGS) -o $@ 	\
		build/cpmls.o 		\
		build/cpmfs.o 		\
		build/device_win32.o
	@echo "CC -o $@"

bin/cpmrm.exe:				\
	build/cpmrm.o 			\
	build/cpmfs.o 			\
	build/device_win32.o
	@$(CC) $(LDFLAGS) -o $@ 	\
		build/cpmrm.o 		\
		build/cpmfs.o 		\
		build/device_win32.o
	@echo "CC -o $@"
		

bin/cpmcp.exe:				\
	build/cpmcp.o 			\
	build/cpmfs.o 			\
	build/device_win32.o
	@$(CC) $(LDFLAGS) -o $@	 	\
		build/cpmcp.o 		\
		build/cpmfs.o 		\
		build/device_win32.o
	@echo "CC -o $@"

bin/cpmchattr.exe:			\
	build/cpmchattr.o 		\
	build/cpmfs.o 			\
	build/device_win32.o
	@$(CC) $(LDFLAGS) -o $@ 	\
		build/cpmchattr.o 	\
		build/cpmfs.o 		\
		build/device_win32.o
	@echo "CC -o $@"

bin/cpmchmod.exe:			\
	build/cpmchmod.o 		\
	build/cpmfs.o 			\
	build/device_win32.o
	@$(CC) $(LDFLAGS) -o $@ 	\
		build/cpmchmod.o 	\
		build/cpmfs.o 		\
		build/device_win32.o
	@echo "CC -o $@"

bin/mkfs.cpm.exe:			\
	build/mkfs.cpm.o 		\
	build/cpmfs.o 			\
	build/device_win32.o
	@$(CC) $(LDFLAGS) -o $@ 	\
		build/mkfs.cpm.o 	\
		build/cpmfs.o 		\
		build/device_win32.o
	@echo "CC -o $@"

bin/fsck.cpm.exe:			\
	build/fsck.cpm.o 		\
	build/cpmfs.o 			\
	build/device_win32.o
	@$(CC) $(LDFLAGS) -o $@ 	\
		build/fsck.cpm.o 	\
		build/cpmfs.o 		\
		build/device_win32.o
	@echo "CC -o $@"

bin/fsed.cpm.exe:			\
	build/fsed.cpm.o 		\
	build/getopt.o 			\
	build/device_win32.o
	@$(CC) $(LDFLAGS) -o $@ 	\
		build/fsed.cpm.o 	\
		build/cpmfs.o 		\
		build/device_win32.o    \
                $(LIBCURSES)
	@echo "CC -o $@"

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

clean:
	rm -f build/*.o bin/*.exe

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## vim: syntax=make ts=8 columns=100 lines=45 fileencoding=utf-8
