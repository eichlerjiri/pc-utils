#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <linux/input.h>

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s /dev/input/<device>\n", pname);
		return 1;
	}

	char *filename = *argv++;

	if (*argv) {
		fatal("Too many arguments");
	}

	FILE *input = fopenx(filename, "r");

	unsigned long long count = 0;

	struct input_event ev;
	while (freadx(&ev, sizeof(ev), 1, input, filename)) {
		if (ev.type == 1 && ev.value == 1 &&
		ev.code != 42 && ev.code != 29 && ev.code != 125 && // L-shift, L-ctrl, L-win
		ev.code != 56 && ev.code != 100 && ev.code != 54 && ev.code != 97) { // L-alt, R-alt, R-shift, R-ctrl
			count++;
			printf ("%llu\n", count);
		}
	}

	fclosex(input, filename);
	return 0;
}
