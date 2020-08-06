CFLAGS=-O2 -pedantic -Wall -Wwrite-strings -Wconversion -Wno-unused-function

all: keystroke-counter port-audio print-non-ascii trace-analyse renamer srt-edit sub-to-srt
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
srt-edit: phony
	gcc $(CFLAGS) -o $@ $@.c
sub-to-srt: phony
	gcc $(CFLAGS) -o $@ $@.c
