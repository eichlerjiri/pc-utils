#include "common.c"
#include "sbuffer.c"
#include <limits.h>
#include <math.h>

#define TIMEREGEX "([0-9]{2}):([0-9]{2}):([0-9]{2})([,\\.])([0-9]{3,4})"

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

static void process(const char *filename, long from, long to, int rewrite, long diff, float diff_fps) {
	FILE *input = fopen(filename, "r");
	if (!input) {
		error("Cannot open %s: %s", filename, c_strerror(errno));
		return;
	}

	regex_t reg1, reg2, reg3;
	c_regcomp(&reg1, "^([^\r\n]*)([\r\n]*)$");
	c_regcomp(&reg2, "^[0-9]{1,8}$");
	c_regcomp(&reg3, "^" TIMEREGEX " --> " TIMEREGEX "$");
	regmatch_t m[11];

	struct sbuffer out;
	if (rewrite) {
		sbuffer_init(&out);
	}

	const char *fixing = rewrite ? "[FIXING] " : "";
	int state = 0;
	long subnum = 0;
	long subnum_write = 0;
	long last_millis = 0;

	unsigned long linenum = 0;
	char *s = NULL;
	size_t n = 0;
	while (c_getline_tryin(&s, &n, input) >= 0) {
		linenum++;

		if (regexec(&reg1, s, 3, m, 0)) {
			error("%s: line %lu: Invalid format", filename, linenum);
			goto end_free;
		}
		if (strcmp(s + m[2].rm_so, "\r\n")) {
			printf("%s%s: line %lu: Invalid newline\n", fixing, filename, linenum);
		}
		s[m[1].rm_eo] = '\0';

		char buffer[256];
		if (state == 0) {
			if (!strcmp(s, "")) {
				printf("%s%s: line %lu: Extra blank line\n", fixing, filename, linenum);
			}
			if (regexec(&reg2, s, 1, m, 0)) {
				error("%s: line %lu: Invalid format", filename, linenum);
				goto end_free;
			}

			long subnum_cur = atol(s);
			if (subnum_cur != subnum + 1) {
				printf("%s%s: line %lu: Invalid subtitle number\n", fixing, filename, linenum);
			}
			subnum = subnum_cur;

			if (rewrite) {
				sprintf(buffer, "%li\r\n", ++subnum_write);
				sbuffer_add(&out, buffer);
			}

			state = 1;
		} else if (state == 1) {
			if (regexec(&reg3, s, 11, m, 0)) {
				error("%s: line %lu: Invalid format", filename, linenum);
				goto end_free;
			}

			if (s[m[4].rm_so] != ',' || s[m[9].rm_so] != ',') {
				printf("%s%s: line %lu: Wrong dot format\n", fixing, filename, linenum);
			}
			if (m[5].rm_eo - m[5].rm_so != 3 || m[10].rm_eo - m[10].rm_so != 3) {
				printf("%s%s: line %lu: Wrong millis format\n", fixing, filename, linenum);
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
				if ((from == LONG_MIN || from * 1000 <= millis1) &&
				(to == LONG_MIN || to * 1000 > millis1)) {
					long f = from != LONG_MIN ? from * 1000 : 0;
					millis1 += diff + (long) (diff_fps * (float)(millis1 - f));
					millis2 += diff + (long) (diff_fps * (float)(millis2 - f));
					if (millis1 < 0 || millis2 < 0) {
						error("%s: line %lu: Negative time", filename, linenum);
						goto end_free;
					}
				}

				char srtbuf1[64];
				to_srt(srtbuf1, millis1);
				char srtbuf2[64];
				to_srt(srtbuf2, millis2);

				sprintf(buffer, "%s --> %s\r\n", srtbuf1, srtbuf2);
				sbuffer_add(&out, buffer);
			}

			if (millis1 >= millis2) {
				printf("%s: line %lu: Invalid time interval\n", filename, linenum);
			}
			if (millis1 < last_millis) {
				printf("%s: line %lu: Overlap\n", filename, linenum);
			}
			last_millis = millis2;

			state = 2;
		} else if (state == 2) {
			if (rewrite) {
				sbuffer_add(&out, s);
				sbuffer_add(&out, "\r\n");
			}
			if (!strcmp(s, "")) {
				state = 0;
			}
		}
	}
	if (errno) {
		error("Cannot read %s: %s", filename, c_strerror(errno));
		goto end_free;
	}

	if (state != 0) {
		printf("%s%s: line %lu: Invalid end of file\n", fixing, filename, linenum);
	}
	if (rewrite && state == 2) {
		sbuffer_add(&out, "\r\n");
	}

	if (rewrite) {
		FILE *output = fopen(filename, "w");
		if (!output) {
			error("Cannot open %s for writing: %s", filename, c_strerror(errno));
			goto end_free;
		}
		if (fputs(out.data, output) <= 0) {
			error("Cannot write %s: %s", filename, c_strerror(errno));
		}
		if (fclose(output)) {
			error("Cannot close %s for writing: %s", filename, c_strerror(errno));
		}
	}

end_free:
	c_free(s);
	if (rewrite) {
		sbuffer_destroy(&out);
	}
	regfree(&reg1);
	regfree(&reg2);
	regfree(&reg3);
	if (fclose(input)) {
		error("Cannot close %s: %s", filename, c_strerror(errno));
	}
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

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s [arguments] <command> <SRT files to edit>\n"
			"\n"
			"command:\n"
			"    verify          check SRT file for common format problems\n"
			"    fix             rewrite SRT file to fix some format problems\n"
			"    +<millis>       forward subtitles for amount of milliseconds\n"
			"    -<millis>       backward subtitles for amount of milliseconds\n"
			"    <fps1>%%<fps2>   change speed of subtitles by fps1/fps2\n"
			"\n"
			"arguments:\n"
			"    -from <secs>    time index from which do retime (optional)\n"
			"    -to <secs>      time index to which do retime (optional)\n", pname);
		return 3;
	}

	long from = LONG_MIN;
	long to = LONG_MIN;
	int rewrite = 0;
	long diff = 0;
	float diff_fps = 0;

	while (1) {
		char *arg = *argv++;
		if (!arg) {
			fatal("Missing arguments");
		}

		char *ptr;
		if (!strcmp(arg, "-from") && *argv) {
			from = longarg(*argv++);
			if (from == LONG_MIN) {
				fatal("Invalid argument: %s", arg);
			}
		} else if (!strcmp(arg, "-to") && *argv) {
			to = longarg(*argv++);
			if (to == LONG_MIN) {
				fatal("Invalid argument: %s", arg);
			}
		} else if (!strcmp(arg, "verify")) {
			break;
		} else if (!strcmp(arg, "fix")) {
			rewrite = 1;
			break;
		} else if ((ptr = strchr(arg, '%'))) {
			*ptr = '\0';
			float fps_from = floatarg(arg);
			*ptr = '%';
			float fps_to = floatarg(ptr + 1);
			if (fps_from == -INFINITY || fps_to == -INFINITY || !fps_from || !fps_to) {
				fatal("Invalid command: %s", arg);
			}
			diff_fps = fps_from / fps_to - 1;
			rewrite = 1;
			break;
		} else {
			diff = longarg(arg);
			if (diff == LONG_MIN) {
				fatal("Invalid command: %s", arg);
			}
			rewrite = 1;
			break;
		}
	}

	while (*argv) {
		process(*argv++, from, to, rewrite, diff, diff_fps);
	}

	return return_code;
}
