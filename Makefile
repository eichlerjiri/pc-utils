CFLAGS=-O2 -pedantic -Wall -Wwrite-strings -Wconversion -Wno-unused-function

all: keystroke-counter port-audio print-non-ascii trace-analyse renamer srt-edit sub-to-srt dup-files password-gen log-browser
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
dup-files: phony
	gcc $(CFLAGS) -o $@ $@.c
password-gen: phony
	gcc $(CFLAGS) -o $@ $@.c
log-browser: phony
	gcc $(CFLAGS) -o $@ $@.c -lncursesw
