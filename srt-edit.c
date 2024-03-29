#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "utils/stdio_utils.h"
#include "utils/stdlib_utils.h"
#include "utils/string_utils.h"
#include "utils/strlist.h"
#include "utils/parser.h"

struct strlist inbuf, outbuf;

static int parse_srt_time(char **str, long *res, const char *filename, unsigned long linenum, const char *fixing) {
	unsigned long hours, minutes, seconds, millis;
	char dot = '\0';
	int err = parse_unsigned_long_strict(str, &hours);
	err += parse_char_match(str, ':');
	err += parse_unsigned_long_strict(str, &minutes);
	err += parse_char_match(str, ':');
	err += parse_unsigned_long_strict(str, &seconds);
	err += parse_char(str, &dot);
	err += parse_unsigned_long_strict(str, &millis);

	if (err || minutes >= 60 || seconds >= 60 || millis > 1000 || (dot != '.' && dot != ',')) {
		return 2;
	}
	if (dot != ',') {
		printf_safe("%s%s: line %lu: Wrong dot format\n", fixing, filename, linenum);
	}
	if (millis == 1000) {
		printf_safe("%s%s: line %lu: Wrong millis format\n", fixing, filename, linenum);
	}

	*res = (long) (millis + 1000 * (seconds + 60 * (minutes + (60 * hours))));
	return 0;
}

static void print_srt_time(char *buffer, long millis) {
	long secs = millis / 1000;
	long minutes = secs / 60;
	long hours = minutes / 60;
	millis %= 1000;
	secs %= 60;
	minutes %= 60;
	sprintf(buffer, "%02li:%02li:%02li,%03li", hours, minutes, secs, millis);
}

static int process_file(const char *filename, FILE *input, long from, long to, int rewrite, long diff, float diff_fps) {
	const char *fixing;
	if (rewrite) {
		strlist_resize(&outbuf, 0);

		fixing = "[FIXING] ";
	} else {
		fixing = "";
	}

	int state = 0;
	unsigned long subnum = 0, subnum_write = 0;
	long last_millis = 0;
	size_t last_finished_pos = 0;

	unsigned long linenum = 0;
	while (1) {
		strlist_resize(&inbuf, 0);
		if (strlist_readline(&inbuf, input) == -1) {
			break;
		}
		linenum++;

		char buffer[256];

		if (parse_line_end(inbuf.data, &inbuf.size, buffer)) {
			fprintf(stderr, "%s: line %lu: Invalid line format\n", filename, linenum);
			return 2;
		}
		if (strcmp(buffer, "\r\n")) {
			printf_safe("%s%s: line %lu: Invalid newline\n", fixing, filename, linenum);
		}

		if (state == 0) {
			if (!*inbuf.data) {
				printf_safe("%s%s: line %lu: Extra blank line\n", fixing, filename, linenum);
			} else {
				char *s = inbuf.data;
				unsigned long subnum_cur;
				int err = parse_unsigned_long_strict(&s, &subnum_cur);
				if (err || *s) {
					fprintf(stderr, "%s: line %lu: Invalid format\n", filename, linenum);
					return 2;
				}

				if (subnum_cur != subnum + 1) {
					printf_safe("%s%s: line %lu: Invalid subtitle number\n", fixing, filename, linenum);
				}
				subnum = subnum_cur;

				if (rewrite) {
					size_t written = (size_t) sprintf(buffer, "%li\r\n", ++subnum_write);
					strlist_add_sn(&outbuf, buffer, written);
				}

				state = 1;
			}

		} else if (state == 1) {
			char *s = inbuf.data;
			long millis1, millis2;
			int err = parse_srt_time(&s, &millis1, filename, linenum, fixing);
			err += parse_string_match(&s, " --> ");
			err += parse_srt_time(&s, &millis2, filename, linenum, fixing);
			if (err || *s) {
				fprintf(stderr, "%s: line %lu: Invalid format\n", filename, linenum);
				return 2;
			}

			if (rewrite) {
				if ((from == LONG_MIN || from * 1000 <= millis1) && (to == LONG_MIN || to * 1000 > millis1)) {
					long f;
					if (from != LONG_MIN) {
						f = from * 1000;
					} else {
						f = 0;
					}

					millis1 += diff + (long) (diff_fps * (float) (millis1 - f));
					millis2 += diff + (long) (diff_fps * (float) (millis2 - f));
					if (millis1 < 0 || millis2 < 0) {
						fprintf(stderr, "%s: line %lu: Negative time\n", filename, linenum);
						return 2;
					}
				}

				char srtbuf1[64], srtbuf2[64];
				print_srt_time(srtbuf1, millis1);
				print_srt_time(srtbuf2, millis2);
				size_t written = (size_t) sprintf(buffer, "%s --> %s\r\n", srtbuf1, srtbuf2);
				strlist_add_sn(&outbuf, buffer, written);
			}

			if (millis1 >= millis2) {
				printf_safe("%s: line %lu: Invalid time interval\n", filename, linenum);
			}
			if (millis1 < last_millis) {
				printf_safe("%s: line %lu: Overlap\n", filename, linenum);
			}
			last_millis = millis2;

			state = 2;

		} else if (state == 2 || state == 3) {
			if (rewrite) {
				strlist_add_sn(&outbuf, inbuf.data, inbuf.size);
				strlist_add_sn(&outbuf, "\r\n", strlen("\r\n"));
			}

			if (!*inbuf.data) {
				if (state == 2) {
					printf_safe("%s%s: line %lu: No text\n", fixing, filename, linenum);

					if (rewrite) {
						strlist_resize(&outbuf, last_finished_pos);
						subnum_write--;
					}
				}

				last_finished_pos = outbuf.size;
				state = 0;
			} else {
				state = 3;
			}
		}
	}
	if (!feof(input)) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	if (state != 0) {
		printf_safe("%s%s: line %lu: Invalid end of file\n", fixing, filename, linenum);

		if (rewrite) {
			if (state == 3) {
				strlist_add_sn(&outbuf, "\r\n", strlen("\r\n"));
			} else {
				strlist_resize(&outbuf, last_finished_pos);
			}
		}
	}

	return 0;
}

static int process_filename(const char *filename, long from, long to, int rewrite, long diff, float diff_fps) {
	FILE *input = fopen_trace(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = process_file(filename, input, from, to, rewrite, diff, diff_fps);

	if (fclose_trace(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}

	if (!ret && rewrite) {
		FILE *output = fopen_trace(filename, "w");
		if (!output) {
			fprintf(stderr, "Error opening file %s for writing: %s\n", filename, strerror(errno));
			return 2;
		}

		if (fwrite(outbuf.data, outbuf.size, 1, output) != 1) {
			fprintf(stderr, "Error writing file %s\n", filename);
			ret = 2;
		}

		if (fclose_trace(output)) {
			fprintf(stderr, "Error closing file %s for writing: %s\n", filename, strerror(errno));
			ret = 2;
		}
	}

	return ret;
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_safe("Usage: %s [arguments] <command> <SRT files to edit>\n"
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

	long from = LONG_MIN, to = LONG_MIN;
	int rewrite = 0;
	long diff = 0;
	float diff_fps = 0;

	while (*argv) {
		char *arg = *argv++;

		if (!strcmp(arg, "-from") && *argv) {
			char *in = *argv++;
			int err = parse_long(&in, &from);
			if (err || *in) {
				fprintf(stderr, "Invalid argument: %s\n", arg);
				return 2;
			}
		} else if (!strcmp(arg, "-to") && *argv) {
			char *in = *argv++;
			int err = parse_long(&in, &to);
			if (err || *in) {
				fprintf(stderr, "Invalid argument: %s\n", arg);
				return 2;
			}
		} else if (!strcmp(arg, "verify")) {
			break;
		} else if (!strcmp(arg, "fix")) {
			rewrite = 1;
			break;
		} else if (strchr(arg, '/')) {
			char *in = arg;
			float fps_from, fps_to;
			int err = parse_float(&in, &fps_from);
			err += parse_char_match(&in, '/');
			err += parse_float(&in, &fps_to);
			if (err || *in || !fps_from || !fps_to) {
				fprintf(stderr, "Invalid command: %s\n", arg);
				return 2;
			}
			diff_fps = fps_from / fps_to - 1;
			rewrite = 1;
			break;
		} else {
			char *in = arg;
			int err = parse_long(&in, &diff);
			if (err || *in) {
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

	strlist_init(&inbuf);
	if (rewrite) {
		strlist_init(&outbuf);
	}

	int ret = 0;
	while (*argv) {
		if (process_filename(*argv++, from, to, rewrite, diff, diff_fps)) {
			ret = 2;
		}
	}

	strlist_destroy(&inbuf);
	if (rewrite) {
		strlist_destroy(&outbuf);
	}

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
