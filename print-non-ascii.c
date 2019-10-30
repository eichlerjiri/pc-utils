#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <iconv.h>
#include "utils/stdlib_e.h"
#include "utils/stdio_e.h"
#include "utils/iconv_e.h"

struct res {
	iconv_t cd_utf8;
	iconv_t cd_latin1;
	size_t insize;
	size_t outsize;
	char *in;
	char *out;
};

static int nonascii(char *in, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (in[i] < 0) {
			return 1;
		}
	}
	return 0;
}

static size_t iconv_full(iconv_t cd, char *in, size_t inlen, char *out, size_t outlen) {
	char *outbuf = out;
	size_t ret = iconv(cd, &in, &inlen, &outbuf, &outlen);
	if (ret == (size_t) -1) {
		return ret;
	} else {
		return (size_t) (outbuf - out);
	}
}

static int process(FILE *input, char *filename, struct res *c) {
	unsigned long linenum = 0;
	size_t linelen;
	while ((linelen = (size_t) getline_em(&c->in, &c->insize, input)) != (size_t) -1) {
		linenum++;

		if (c->in[linelen - 1] == '\n') {
			linelen--;
		}

		if (nonascii(c->in, linelen)) {
			if (c->outsize < linelen * 4) {
				c->outsize = linelen * 4;
				c->out = realloc_e(c->out, c->outsize);
			}

			const char* code = "utf8";
			if (!c->cd_utf8) {
				c->cd_utf8 = iconv_open_e("utf8", code);
			}
			size_t outlen = iconv_full(c->cd_utf8, c->in, linelen, c->out, c->outsize);

			if (outlen == (size_t) -1) {
				code = "latin1";
				if (!c->cd_latin1) {
					c->cd_latin1 = iconv_open_e("utf8", code);
				}
				outlen = iconv_full(c->cd_latin1, c->in, linelen, c->out, c->outsize);
			}

			printf_e("%s: line %lu %s: ", filename, linenum, code);
			fwrite_e(c->out, outlen, 1, stdout);
			putchar_e('\n');
		}
	}
	if (errno) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		return 2;
	}
	return 0;
}

static int process_arg(char *filename, struct res *c) {
	FILE *input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = process(input, filename, c);

	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	return ret;
}

static int run(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_e("Usage: %s <input files>\n", pname);
		return 1;
	}

	struct res c = {0};

	int ret = 0;
	while (*argv) {
		if (process_arg(*argv++, &c)) {
			ret = 2;
		}
	}

	if (c.cd_utf8) {
		iconv_close(c.cd_utf8);
	}
	if (c.cd_latin1) {
		iconv_close(c.cd_latin1);
	}
	free(c.in);
	free(c.out);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run(argv);
	fflush_e(stdout);
	return ret;
}
