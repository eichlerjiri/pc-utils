#include "common.c"
#include <linux/input.h>

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s /dev/input/<device>\n", pname);
		return 2;
	}

	char *filename = *argv++;
	if (*argv) {
		fatal("Too many arguments");
	}

	FILE *input = c_fopen(filename, "r");

	unsigned long count = 0;
	struct input_event ev;
	while (c_fread(&ev, sizeof(ev), 1, input)) {
		if (ev.type == 1 && ev.value == 1
		&& ev.code != 42 && ev.code != 29 && ev.code != 125 && ev.code != 56 // L-shift, L-ctrl, L-win, L-alt
		&& ev.code != 100 && ev.code != 54 && ev.code != 97) { // R-alt, R-shift, R-ctrl
			count++;
			c_printf("%lu\n", count);
		}
	}

	c_fclose(input);
	c_fflush(stdout);
	return 0;
}
