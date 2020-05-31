#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <iconv.h>
#include "utils/stdlib_utils.h"
#include "utils/stdio_utils.h"
#include "utils/iconv_utils.h"

struct res {
	iconv_t cd_utf8, cd_latin1;
	size_t insize, outsize;
	char *in, *out;
};

static int nonascii(char *in, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (in[i] < 0) {
			return 1;
		}
	}
	return 0;
}

static int process_file(char *filename, struct res *c) {
	FILE *input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = 0;

	unsigned long linenum = 0;
	size_t inlen;
	while ((inlen = (size_t) getline_no_eol(&c->in, &c->insize, input)) != (size_t) -1) {
		linenum++;

		if (nonascii(c->in, inlen)) {
			const char* code = "utf8";
			size_t outlen = iconv_direct(&c->cd_utf8, "utf8", code, c->in, inlen, &c->out, &c->outsize);

			if (outlen == (size_t) -1) {
				code = "latin1";
				outlen = iconv_direct(&c->cd_latin1, "utf8", code, c->in, inlen, &c->out, &c->outsize);
			}

			printf_safe("%s: line %lu %s: ", filename, linenum, code);
			fwrite_safe(c->out, outlen, 1, stdout);
			putchar_safe('\n');
		}
	}
	if (!feof(input)) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}
	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}

	return ret;
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_safe("Usage: %s <input files>\n", pname);
		return 1;
	}

	struct res c = {0};

	int ret = 0;
	while (*argv) {
		if (process_file(*argv++, &c)) {
			ret = 2;
		}
	}

	iconv_close_if_opened(c.cd_utf8);
	iconv_close_if_opened(c.cd_latin1);
	free(c.in);
	free(c.out);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	fflush_safe(stdout);
	return ret;
}
