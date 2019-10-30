#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <linux/input.h>
#include "utils/stdio_e.h"

static int process(FILE *input, char *filename) {
	unsigned long count = 0;
	struct input_event ev;
	while (fread(&ev, sizeof(ev), 1, input)) {
		if (ev.type == 1 && ev.value == 1
		&& ev.code != 42 && ev.code != 29 && ev.code != 125 && ev.code != 56 // L-shift, L-ctrl, L-win, L-alt
		&& ev.code != 100 && ev.code != 54 && ev.code != 97) { // R-alt, R-shift, R-ctrl
			count++;
			printf_e("%lu\n", count);
		}
	}
	if (ferror(input)) {
		fprintf(stderr, "Error reading file %s\n", filename);
		return 2;
	}
	return 0;
}

static int run(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_e("Usage: %s /dev/input/<device>\n", pname);
		return 1;
	}

	char *filename = *argv++;
	if (*argv) {
		fprintf(stderr, "Too many arguments\n");
		return 2;
	}

	FILE *input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = process(input, filename);

	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	return ret;
}

int main(int argc, char **argv) {
	int ret = run(argv);
	fflush_e(stdout);
	return ret;
}
