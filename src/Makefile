# $Id: Makefile,v 1.1.1.1 2001-05-23 11:22:08 masamic Exp $

#
# $Log: not supported by cvs2svn $
# Revision 1.3  2000/02/02  13:00:20  yfujii
# Source files (line1.c, line3.c) are no more needed. Removed.
#
# Revision 1.2  1999/12/07  12:33:14  yfujii
# *** empty log message ***
#
# Revision 1.2  1999/11/15  03:33:03  yfujii
# Some source files are added.
#
# Revision 1.1  1999/10/22  03:33:36  yfujii
# Initial revision

#

SRCS =	calc.c doscall.c exec.c getini.c iocscall.c key.c \
	line0.c line2.c line4.c line5.c \
	line6.c line7.c line8.c line9.c lineb.c linec.c \
	lined.c linee.c linef.c \
	load.c mem.c run68.c debugger.c eaaccess.c disassemble.c \
	conditions.c ansicolor-w32.c

OBJS =	calc.o doscall.o exec.o getini.o iocscall.o key.o \
	line0.o line2.o line4.o line5.o \
	line6.o line7.o line8.o line9.o lineb.o linec.o \
	lined.o linee.o linef.o \
	load.o mem.o run68.o debugger.o eaaccess.o disassemble.o \
	conditions.o ansicolor-w32.o

CC = gcc

#LDFLAGS = -lc -lcygwin -lm -lg -lmsvcrt40
# for MinGW
#LDFLAGS = -lmsvcrt40
# for MSYS
#LDFLAGS =
LDFLAGS = -lcygwin -lmsvcrt40
DEFS = -DFNC_TRACE -DENV_FROM_INI
CFLAGS = -g $(DEFS)
RELEASE_DIR = ../Release
ARCHIVE_DIR = ../archives
.c.o: $(CC) $(CFLAGS) $<

all: run68.exe
clean:
	rm -f *.o run68.exe
archive:
	tar cvfz $(ARCHIVE_DIR)/run68src`date +%G%m%d-%0k`.tar.gz *.c *.h Makefile *.txt *.ini

release:
	tar cvfz $(RELEASE_DIR)/run68src`date +%G%m%d`.tar.gz *.c *.h Makefile *.txt *.ini
	cp *.ini $(RELEASE_DIR); cp *.txt $(RELEASE_DIR)

run68.exe: $(OBJS)
	gcc -o run68.exe $(OBJS) $(LDFLAGS)

$(OBJS): run68.h
