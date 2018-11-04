CFLAGS=-O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -pedantic -Wall -Wwrite-strings -Wconversion

all: keystroke-counter port-audio print-non-ascii

keystroke-counter: common.o keystroke-counter.o
	gcc $(CFLAGS) -o $@ $^
port-audio: common.o port-audio.o
	gcc $(CFLAGS) -o $@ $^
print-non-ascii: common.o print-non-ascii.o
	gcc $(CFLAGS) -o $@ $^

%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm *.o keystroke-counter port-audio print-non-ascii
