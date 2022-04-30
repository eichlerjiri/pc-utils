CC=gcc
CFLAGS=-O2 -pedantic -Wall -Wwrite-strings -Wconversion -Wno-unused-function
ALL=keystroke-counter port-audio print-non-ascii trace-analyse renamer srt-edit sub-to-srt dup-files password-gen log-browser

all: $(ALL)

keystroke-counter:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
port-audio:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
print-non-ascii:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
trace-analyse:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
renamer:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
srt-edit:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
sub-to-srt:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
dup-files:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
password-gen:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c
log-browser:
	$(CC) $(CFLAGS) -MMD -o $@ $@.c -l:libncursesw.a -l:libtinfo.a

-include $(ALL:=.d)

clean:
	rm -f *.d $(ALL)
