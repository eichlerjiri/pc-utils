#include "common.c"

static void process(char *filename) {
	FILE *input = fopen(filename, "r");
	if (!input) {
		error("Cannot open %s: %s", filename, c_strerror(errno));
		return;
	}

	unsigned long long linenum = 0;
	char *lineptr = NULL;
	size_t n = 0;
	while (c_getline(&lineptr, &n, input) >= 0) {
		linenum++;

		int nonascii = 0;
		for (char *c = lineptr; *c; c++) {
			if (*c < 0) {
				nonascii = 1;
			} else if (*c == '\n' || (*c == '\r' && *(c + 1) == '\n')) {
				*c = '\0';
			}
		}
		if (nonascii) {
			char *conv = c_iconv("utf8", "utf8", lineptr);
			if (conv) {
				printf("%s: line %llu utf8: %s\n", filename, linenum, conv);
			} else {
				conv = c_iconv("latin1", "utf8", lineptr);
				printf("%s: line %llu latin1: %s\n", filename, linenum, conv);
			}
			c_free(conv);
		}
	}
	if (errno) {
		error("Cannot read %s: %s", filename, c_strerror(errno));
	}

	c_free(lineptr);
	if (fclose(input)) {
		error("Cannot close %s: %s", filename, c_strerror(errno));
	}
}

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s <input files>\n", pname);
		return 3;
	}

	while (*argv) {
		process(*argv++);
	}

	return return_code;
}
