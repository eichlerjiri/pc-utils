#include "common.c"

static int nonascii(char *in, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (in[i] < 0) {
			return 1;
		}
	}
	return 0;
}

static size_t iconv_full(const char *tocode, const char *fromcode, char *in, size_t inlen, char *out, size_t outlen) {
	iconv_t cd = c_iconv_open(tocode, fromcode);
	size_t ret = c_iconv_tryin(cd, in, inlen, out, outlen);
	c_iconv_close(cd);
	return ret;
}

static void process(char *filename) {
	FILE *input = c_fopen(filename, "r");

	unsigned long linenum = 0;
	char *lineptr = NULL;
	size_t n = 0;
	size_t linelen;
	while ((linelen = (size_t) c_getline(&lineptr, &n, input)) != (size_t) -1) {
		linenum++;

		if (lineptr[linelen - 1] == '\n') {
			linelen--;
		}

		if (nonascii(lineptr, linelen)) {
			size_t outsize = linelen * 4;
			char *out = c_malloc(outsize);

			const char *code = "utf8";
			size_t outlen = iconv_full("utf8", code, lineptr, linelen, out, outsize);
			if (outlen == (size_t) -1) {
				code = "latin1";
				outlen = iconv_full("utf8", code, lineptr, linelen, out, outsize);
			}

			c_printf("%s: line %lu %s: ", filename, linenum, code);
			c_fwrite(out, outlen, 1, stdout);
			c_putchar('\n');
			c_free(out);
		}
	}

	c_free(lineptr);
	c_fclose(input);
}

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s <input files>\n", pname);
		return 2;
	}

	while (*argv) {
		process(*argv++);
	}

	c_fflush(stdout);
	return 0;
}
