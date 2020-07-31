#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <iconv.h>
#include "utils/stdlib_utils.h"
#include "utils/stdio_utils.h"
#include "utils/iconv_utils.h"

iconv_t cd_utf8, cd_latin1;
size_t insize, outsize;
char *inbuf, *outbuf;

static int nonascii(char *in, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (in[i] < 0) {
			return 1;
		}
	}
	return 0;
}

static int process_file(char *filename) {
	FILE *input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = 0;

	unsigned long linenum = 0;
	size_t inlen;
	while ((inlen = (size_t) getline_no_eol(&inbuf, &insize, input)) != (size_t) -1) {
		linenum++;

		if (nonascii(inbuf, inlen)) {
			const char* code = "utf8";
			size_t outlen = iconv_direct(&cd_utf8, "utf8", code, inbuf, inlen, &outbuf, &outsize);

			if (outlen == (size_t) -1) {
				code = "latin1";
				outlen = iconv_direct(&cd_latin1, "utf8", code, inbuf, inlen, &outbuf, &outsize);
			}

			printf_safe("%s: line %lu %s: ", filename, linenum, code);
			fwrite_safe(outbuf, outlen, 1, stdout);
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

	int ret = 0;
	while (*argv) {
		if (process_file(*argv++)) {
			ret = 2;
		}
	}

	iconv_close_if_opened(cd_utf8);
	iconv_close_if_opened(cd_latin1);
	free(inbuf);
	free(outbuf);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	fflush_safe(stdout);
	return ret;
}
