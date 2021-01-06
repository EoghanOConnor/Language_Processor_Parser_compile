#
# Makefile for compiler project files. For use under Linux, Cygwin or
# Mac OS/X (i.e., Unix-style environments).
#
# Targets:
#	make comp		generate Compiler from Compiler.c
#
#
#	make clean		delete all object files (but NOT the library
#					file) created by this Makefile
#
#	make veryclean		delete all object and library files created
#						by this Makefile
#

#
# Name of your local gcc (or other ANSI C compiler).
CC=gcc
# Flags to pass to compiler.
CFLAGS=-ansi -pedantic -Wall -Iheaders

# The version of MAKE being used is set by MAKE
MAKE=make

# Name of your local delete program. Usually "/bin/rm -rf".
RM=/bin/rm -rf

# Name of library of object files.
CODELIB=libcomp.a

# Build rules follow.

$(CODELIB) :
	$(MAKE) -C libsrc $(CODELIB)
	mv libsrc/$(CODELIB) .
	$(MAKE) -C libsrc veryclean

comp: Compiler.o $(CODELIB)
	$(CC) -o $@ Compiler.o $(CODELIB)


clean:
	$(RM) *.o

veryclean:
	$(RM) $(CODELIB) *.o compiler
