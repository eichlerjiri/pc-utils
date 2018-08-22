keystroke-counter: keystroke-counter.c
	gcc -O2 -Wall -pedantic -o $@ $<

clean:
	rm keystroke-counter
