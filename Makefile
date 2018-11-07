CFLAGS=-O2 -pedantic -Wall -Wwrite-strings -Wconversion -Wno-unused-function -DENABLE_TRACE=0

all: keystroke-counter port-audio print-non-ascii trace-analyse renamer
phony:

keystroke-counter: phony
	gcc $(CFLAGS) -o $@ $@.c
port-audio: phony
	gcc $(CFLAGS) -o $@ $@.c
print-non-ascii: phony
	gcc $(CFLAGS) -o $@ $@.c
trace-analyse: phony
	gcc $(CFLAGS) -o $@ $@.c
renamer: phony
	gcc $(CFLAGS) -o $@ $@.c
