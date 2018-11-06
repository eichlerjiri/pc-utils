ENABLE_TRACE=0

ifneq ($(ENABLE_TRACE), 0)
TRACE_FILE=trace.o
TRACE_FLAG=-DENABLE_TRACE=1
else
TRACE_FILE=
TRACE_FLAG=
endif

CFLAGS=-O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -pedantic -Wall -Wwrite-strings -Wconversion

all: keystroke-counter port-audio print-non-ascii trace-analyse

keystroke-counter: common.o $(TRACE_FILE) keystroke-counter.o
	gcc $(CFLAGS) -o $@ $^
port-audio: common.o $(TRACE_FILE) port-audio.o
	gcc $(CFLAGS) -o $@ $^
print-non-ascii: common.o $(TRACE_FILE) print-non-ascii.o
	gcc $(CFLAGS) -o $@ $^
trace-analyse: common.o $(TRACE_FILE) hmap.o trace-analyse.o
	gcc $(CFLAGS) -o $@ $^

%.o : %.c
	gcc $(CFLAGS) $(TRACE_FLAG) -c -o $@ $<

clean:
	rm *.o keystroke-counter port-audio print-non-ascii trace-analyse
