CC=gcc
CFLAGS=
FMT=indent

# Default values if not provided on command line
framesize ?= 3
varmemsize ?= 10

mysh: *.c
	$(CC) -DFRAMESIZE=$(framesize) -DVARMEMSIZE=$(varmemsize) -o mysh *.c

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h pcb.c pcb.h queue.c queue.h scheduler.c scheduler.h
	$(FMT) $?

clean: 
	$(RM) mysh; $(RM) *.o; $(RM) *~

.PHONY: clean

