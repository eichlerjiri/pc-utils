#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <regex.h>
#include "utils/stdlib_e.h"
#include "utils/stdio_e.h"
#include "utils/regex_e.h"
#include "utils/sbuffer.h"

#define TIMEREGEX "([0-9]{2}):([0-9]{2}):([0-9]{2})([,\\.])([0-9]{3,4})"

struct res {
	regex_t reg1;
	regex_t reg2;
	regex_t reg3;
	size_t insize;
	char *in;
	struct sbuffer out;
};

static long to_millis(const char *hours, const char *minutes, const char *secs, const char *millis) {
	return atol(millis) + 1000 * (atol(secs) + 60 * (atol(minutes) + (60 * atol(hours))));
}

static void to_srt(char *buffer, long millis) {
	long secs = millis / 1000;
	long minutes = secs / 60;
	long hours = minutes / 60;
	millis %= 1000;
	secs %= 60;
	minutes %= 60;
	sprintf(buffer, "%02li:%02li:%02li,%03li", hours, minutes, secs, millis);
}

static int process(const char *filename, FILE *input, long from, long to, int rewrite, long diff, float diff_fps,
		struct res *c) {
	const char *fixing;
	if (rewrite) {
		fixing = "[FIXING] ";
	} else {
		fixing = "";
	}

	int state = 0;
	long subnum = 0;
	long subnum_write = 0;
	long last_millis = 0;

	regmatch_t m[11];
	unsigned long linenum = 0;
	ssize_t linelen;
	while ((linelen = getline_em(&c->in, &c->insize, input)) != -1) {
		linenum++;
		char *s = c->in;

		if (linelen != strlen(s)) {
			fprintf(stderr, "%s: line %lu: Contains NULL byte\n", filename, linenum);
			return 2;
		}
		if (regexec(&c->reg1, s, 3, m, 0)) {
			fprintf(stderr, "%s: line %lu: Invalid format\n", filename, linenum);
			return 2;
		}
		if (strcmp(s + m[2].rm_so, "\r\n")) {
			printf_e("%s%s: line %lu: Invalid newline\n", fixing, filename, linenum);
		}
		s[m[1].rm_eo] = '\0';

		char buffer[256];
		if (state == 0) {
			if (!strcmp(s, "")) {
				printf_e("%s%s: line %lu: Extra blank line\n", fixing, filename, linenum);
			}
			if (regexec(&c->reg2, s, 1, m, 0)) {
				fprintf(stderr, "%s: line %lu: Invalid format\n", filename, linenum);
				return 2;
			}

			long subnum_cur = atol(s);
			if (subnum_cur != subnum + 1) {
				printf_e("%s%s: line %lu: Invalid subtitle number\n", fixing, filename, linenum);
			}
			subnum = subnum_cur;

			if (rewrite) {
				sprintf(buffer, "%li\r\n", ++subnum_write);
				sbuffer_add(&c->out, buffer);
			}

			state = 1;
		} else if (state == 1) {
			if (regexec(&c->reg3, s, 11, m, 0)) {
				fprintf(stderr, "%s: line %lu: Invalid format\n", filename, linenum);
				return 2;
			}

			if (s[m[4].rm_so] != ',' || s[m[9].rm_so] != ',') {
				printf_e("%s%s: line %lu: Wrong dot format\n", fixing, filename, linenum);
			}
			if (m[5].rm_eo - m[5].rm_so != 3 || m[10].rm_eo - m[10].rm_so != 3) {
				printf_e("%s%s: line %lu: Wrong millis format\n", fixing, filename, linenum);
			}

			s[m[1].rm_eo] = '\0';
			s[m[2].rm_eo] = '\0';
			s[m[3].rm_eo] = '\0';
			s[m[5].rm_eo] = '\0';
			s[m[6].rm_eo] = '\0';
			s[m[7].rm_eo] = '\0';
			s[m[8].rm_eo] = '\0';
			s[m[10].rm_eo] = '\0';

			long millis1 = to_millis(s + m[1].rm_so, s + m[2].rm_so, s + m[3].rm_so, s + m[5].rm_so);
			long millis2 = to_millis(s + m[6].rm_so, s + m[7].rm_so, s + m[8].rm_so, s + m[10].rm_so);

			if (rewrite) {
				if ((from == LONG_MIN || from * 1000 <= millis1)
				&& (to == LONG_MIN || to * 1000 > millis1)) {
					long f;
					if (from != LONG_MIN) {
						f = from * 1000;
					} else {
						f = 0;
					}

					millis1 += diff + (long) (diff_fps * (float)(millis1 - f));
					millis2 += diff + (long) (diff_fps * (float)(millis2 - f));
					if (millis1 < 0 || millis2 < 0) {
						fprintf(stderr, "%s: line %lu: Negative time\n", filename, linenum);
						return 2;
					}
				}

				char srtbuf1[64];
				to_srt(srtbuf1, millis1);
				char srtbuf2[64];
				to_srt(srtbuf2, millis2);

				sprintf(buffer, "%s --> %s\r\n", srtbuf1, srtbuf2);
				sbuffer_add(&c->out, buffer);
			}

			if (millis1 >= millis2) {
				printf_e("%s: line %lu: Invalid time interval\n", filename, linenum);
			}
			if (millis1 < last_millis) {
				printf_e("%s: line %lu: Overlap\n", filename, linenum);
			}
			last_millis = millis2;

			state = 2;
		} else if (state == 2) {
			if (rewrite) {
				sbuffer_add(&c->out, s);
				sbuffer_add(&c->out, "\r\n");
			}
			if (!strcmp(s, "")) {
				state = 0;
			}
		}
	}
	if (errno) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	if (state != 0) {
		printf_e("%s%s: line %lu: Invalid end of file\n", fixing, filename, linenum);
	}
	if (rewrite && state == 2) {
		sbuffer_add(&c->out, "\r\n");
	}

	if (rewrite) {
		FILE *output = fopen(filename, "w");
		if (!output) {
			fprintf(stderr, "Error opening file %s for writing: %s\n", filename, strerror(errno));
			return 2;
		}

		if (fwrite(c->out.data, c->out.size, 1, output) != 1) {
			fprintf(stderr, "Error writing file %s\n", filename);
			return 2;
		}

		if (fclose(output)) {
			fprintf(stderr, "Error closing file %s for writing: %s\n", filename, strerror(errno));
			return 2;
		}
	}

	return 0;
}

static int process_arg(const char *filename, long from, long to, int rewrite, long diff, float diff_fps,
		struct res *c) {
	FILE *input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = process(filename, input, from, to, rewrite, diff, diff_fps, c);

	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	return ret;
}

static long longarg(const char *arg) {
	char *end = NULL;
	errno = 0;

	long ret = strtol(arg, &end, 10);
	if (errno || !*arg || *end) {
		return LONG_MIN;
	}
	return ret;
}

static float floatarg(const char *arg) {
	char *end = NULL;
	errno = 0;

	float ret = strtof(arg, &end);
	if (errno || !*arg || *end) {
		return -INFINITY;
	}
	return ret;
}

static int run(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_e("Usage: %s [arguments] <command> <SRT files to edit>\n"
			"\n"
			"command:\n"
			"    verify          check SRT file for common format problems\n"
			"    fix             rewrite SRT file to fix some format problems\n"
			"    +<millis>       forward subtitles for amount of milliseconds\n"
			"    -<millis>       backward subtitles for amount of milliseconds\n"
			"    <fps1>/<fps2>   change speed of subtitles by fps1/fps2\n"
			"\n"
			"arguments:\n"
			"    -from <secs>    time index from which do retime (optional)\n"
			"    -to <secs>      time index to which do retime (optional)\n", pname);
		return 1;
	}

	long from = LONG_MIN;
	long to = LONG_MIN;
	int rewrite = 0;
	long diff = 0;
	float diff_fps = 0;

	while (*argv) {
		char *arg = *argv++;
		char *ptr;
		if (!strcmp(arg, "-from") && *argv) {
			from = longarg(*argv++);
			if (from == LONG_MIN) {
				fprintf(stderr, "Invalid argument: %s\n", arg);
				return 2;
			}
		} else if (!strcmp(arg, "-to") && *argv) {
			to = longarg(*argv++);
			if (to == LONG_MIN) {
				fprintf(stderr, "Invalid argument: %s\n", arg);
				return 2;
			}
		} else if (!strcmp(arg, "verify")) {
			break;
		} else if (!strcmp(arg, "fix")) {
			rewrite = 1;
			break;
		} else if ((ptr = strchr(arg, '/'))) {
			*ptr = '\0';
			float fps_from = floatarg(arg);
			*ptr = '/';
			float fps_to = floatarg(ptr + 1);
			if (fps_from == -INFINITY || fps_to == -INFINITY || !fps_from || !fps_to) {
				fprintf(stderr, "Invalid command: %s\n", arg);
				return 2;
			}
			diff_fps = fps_from / fps_to - 1;
			rewrite = 1;
			break;
		} else {
			diff = longarg(arg);
			if (diff == LONG_MIN) {
				fprintf(stderr, "Invalid command: %s\n", arg);
				return 2;
			}
			rewrite = 1;
			break;
		}
	}
	if (!*argv) {
		fprintf(stderr, "Missing arguments\n");
		return 2;
	}

	struct res c = {0};
	regcomp_e(&c.reg1, "^([^\r\n]*)([\r\n]*)$", REG_EXTENDED);
	regcomp_e(&c.reg2, "^[0-9]{1,8}$", REG_EXTENDED);
	regcomp_e(&c.reg3, "^" TIMEREGEX " --> " TIMEREGEX "$", REG_EXTENDED);
	if (rewrite) {
		sbuffer_init(&c.out);
	}

	int ret = 0;
	while (*argv) {
		if (process_arg(*argv++, from, to, rewrite, diff, diff_fps, &c)) {
			ret = 2;
		}
	}

	regfree(&c.reg1);
	regfree(&c.reg2);
	regfree(&c.reg3);
	free(c.in);
	if (rewrite) {
		sbuffer_destroy(&c.out);
	}

	return ret;
}

int main(int argc, char **argv) {
	int ret = run(argv);
	fflush_e(stdout);
	return ret;
}
