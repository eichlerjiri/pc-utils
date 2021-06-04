#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include "utils/stdlib_utils.h"
#include "utils/stdio_utils.h"
#include "utils/string_utils.h"
#include "utils/alist.h"
#include "utils/parser.h"
#include "utils/exec.h"

size_t insize;
char *inbuf;
struct alist outbuf, namebuf;

static int find_video_file(const char *filename) {
	const char *ext[] = {".avi", ".mkv", ".mp4", NULL};

	alist_rem_c(&namebuf, namebuf.size);
	alist_add_s(&namebuf, filename);

	char *dot;
	while ((dot = strrchr(namebuf.cdata, '.'))) {
		alist_rem_c(&namebuf, (size_t) (dot - namebuf.cdata));

		for (int i = 0; ext[i]; i++) {
			size_t orig_len = namebuf.size;
			alist_add_s(&namebuf, ext[i]);

			if (!access(namebuf.cdata, F_OK)) {
				return 0;
			}

			alist_rem_c(&namebuf, namebuf.size - orig_len);
		}
	}

	return 2;
}

static int determine_fps(const char *filename, float *res) {
	if (find_video_file(filename)) {
		fprintf(stderr, "No adjacent video file for: %s\n", filename);
		return 2;
	}

	alist_rem_c(&outbuf, outbuf.size);

	const char *params[] = {"ffprobe", "-select_streams", "v", "-of", "default=noprint_wrappers=1:nokey=1", "-show_entries", "stream=r_frame_rate", namebuf.cdata, NULL};
	if (exec_and_wait(params[0], params, &outbuf)) {
		return 2;
	}

	char *s = outbuf.cdata;
	unsigned long nomin, denom;

	int err = parse_unsigned_long_strict(&s, &nomin);
	err += parse_char_match(&s, '/');
	err += parse_unsigned_long_strict(&s, &denom);

	if (err || nomin <= 0 || denom <= 0) {
		fprintf(stderr, "Invalid ffprobe response for file: %s\n", namebuf.cdata);
		return 2;
	}

	*res = (float) nomin / (float) denom;
	return 0;
}

static void print_srt_time(char *buffer, unsigned long millis) {
	unsigned long secs = millis / 1000;
	unsigned long minutes = secs / 60;
	unsigned long hours = minutes / 60;
	millis %= 1000;
	secs %= 60;
	minutes %= 60;
	sprintf(buffer, "%02lu:%02lu:%02lu,%03lu", hours, minutes, secs, millis);
}

static int process_file(const char *filename, FILE *input) {
	char *dot = strrchr(filename, '.');
	if (!dot || strcasecmp(dot, ".sub")) {
		fprintf(stderr, "Filename does not end with .sub: %s\n", filename);
		return 2;
	}

	float fps;
	if (determine_fps(filename, &fps)) {
		return 2;
	}

	alist_rem_c(&outbuf, outbuf.size);

	unsigned long linenum = 0;
	size_t linelen;
	while ((linelen = (size_t) getline(&inbuf, &insize, input)) != (size_t) -1) {
		linenum++;

		char *s = inbuf;
		unsigned long from, to;

		int err = parse_line_end(s, linelen, NULL);
		err += parse_char_match(&s, '{');
		err += parse_unsigned_long_strict(&s, &from);
		err += parse_char_match(&s, '}');
		err += parse_char_match(&s, '{');
		err += parse_unsigned_long_strict(&s, &to);
		err += parse_char_match(&s, '}');

		if (err || !*s) {
			fprintf(stderr, "%s: line %lu: Invalid format\n", filename, linenum);
			return 2;
		}

		char buffer[256];
		sprintf(buffer, "%lu\r\n", linenum);
		alist_add_s(&outbuf, buffer);

		char srtbuf1[64], srtbuf2[64];
		print_srt_time(srtbuf1, (unsigned long) ((float) from / fps * 1000));
		print_srt_time(srtbuf2, (unsigned long) ((float) to / fps * 1000));
		sprintf(buffer, "%s --> %s\r\n", srtbuf1, srtbuf2);
		alist_add_s(&outbuf, buffer);

		int italic = 0;
		while (*s) {
			char c = *s++;
			if (c == '|') {
				if (italic) {
					alist_add_s(&outbuf, "</i>");
					italic = 0;
				}
				alist_add_s(&outbuf, "\r\n");
			} else if (c == '{') {
				if (!strncmp(s, "y:i}", 4)) {
					alist_add_s(&outbuf, "<i>");
					italic = 1;
					s += 4;
				} else {
					fprintf(stderr, "%s: line %lu: Invalid format\n", filename, linenum);
					return 2;
				}
			} else {
				alist_add_c(&outbuf, c);
			}
		}
		if (italic) {
			alist_add_s(&outbuf, "</i>");
		}

		alist_add_s(&outbuf, "\r\n\r\n");
	}
	if (!feof(input)) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	alist_rem_c(&namebuf, namebuf.size);
	alist_add_sn(&namebuf, filename, (size_t) (dot - filename));
	alist_add_s(&namebuf, ".srt");

	FILE *output = fopen(namebuf.cdata, "w");
	if (!output) {
		fprintf(stderr, "Error opening file %s for writing: %s\n", namebuf.cdata, strerror(errno));
		return 2;
	}

	int ret = 0;

	if (fwrite(outbuf.data, outbuf.size, 1, output) != 1) {
		fprintf(stderr, "Error writing file %s\n", namebuf.cdata);
		ret = 2;
	}

	if (fclose(output)) {
		fprintf(stderr, "Error closing file %s for writing: %s\n", namebuf.cdata, strerror(errno));
		ret = 2;
	}

	return ret;
}

static int process_filename(const char *filename) {
	FILE *input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = process_file(filename, input);

	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}

	return ret;
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_safe("Usage: %s <SUB files to convert>\n", pname);
		return 1;
	}

	alist_init_c(&outbuf);
	alist_init_c(&namebuf);

	int ret = 0;
	while (*argv) {
		if (process_filename(*argv++)) {
			ret = 2;
		}
	}

	alist_destroy_c(&outbuf);
	alist_destroy_c(&namebuf);
	free(inbuf);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
