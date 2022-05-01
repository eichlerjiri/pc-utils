#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <linux/input.h>

#include "utils/stdio_utils.h"

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_safe("Usage: %s /dev/input/<device>\n", pname);
		return 1;
	}

	char *filename = *argv++;
	if (*argv) {
		fprintf(stderr, "Too many arguments\n");
		return 2;
	}

	FILE *input = fopen_trace(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = 0;

	unsigned long count = 0;
	struct input_event ev;
	while (fread(&ev, sizeof(ev), 1, input)) {
		// L-shift, L-ctrl, L-win, L-alt, R-alt, R-shift, R-ctrl
		if (ev.type == 1 && ev.value == 1 && ev.code != 42 && ev.code != 29 && ev.code != 125 && ev.code != 56 && ev.code != 100 && ev.code != 54 && ev.code != 97) {
			count++;
			printf_safe("%lu\n", count);
		}
	}
	if (!feof(input)) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}
	if (fclose_trace(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
