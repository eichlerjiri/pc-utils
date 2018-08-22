#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <linux/input.h>

void fatal(char *msg, ...) {
	va_list valist;
	va_start(valist, msg);

	fprintf(stderr, "\033[91m");
	vfprintf(stderr, msg, valist);
	fprintf(stderr, "\033[0m\n");

	va_end(valist);
	exit(1);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fatal("Usage: %s /dev/input/<device>", argv[0]);
	}

	FILE *input = fopen(argv[1], "r");
	if (!input) {
		fatal("Cannot open %s", argv[1]);
	}

	unsigned long long count = 0;

	struct input_event ev;
	while (fread(&ev, sizeof(ev), 1, input)) {
		if (ev.type == 1 && ev.value == 1 &&
		ev.code != 42 && ev.code != 29 && ev.code != 125 && // L-shift, L-ctrl, L-win
		ev.code != 56 && ev.code != 100 && ev.code != 54 && ev.code != 97) { // L-alt, R-alt, R-shift, R-ctrl
			count++;
			printf ("%llu\n", count);
		}
	}

	fclose(input);
	return 0;
}
