#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

#include "utils/stdio_utils.h"
#include "utils/stdlib_utils.h"
#include "utils/string_utils.h"
#include "utils/strlist.h"
#include "utils/parser.h"
#include "utils/exec.h"

struct strlist inbuf, outbuf, namebuf;

static int find_video_file(const char *filename) {
	const char *ext[] = {".avi", ".mkv", ".mp4", NULL};

	strlist_resize(&namebuf, 0);
	strlist_add_sn(&namebuf, filename, strlen(filename));

	char *dot;
	while ((dot = strrchr(namebuf.data, '.'))) {
		strlist_resize(&namebuf, (size_t) (dot - namebuf.data));

		for (int i = 0; ext[i]; i++) {
			size_t orig_len = namebuf.size;
			strlist_add_sn(&namebuf, ext[i], strlen(ext[i]));

			if (!access(namebuf.data, F_OK)) {
				return 0;
			}

			strlist_resize(&namebuf, orig_len);
		}
	}

	return 2;
}

static int determine_fps(const char *filename, float *res) {
	if (find_video_file(filename)) {
		fprintf(stderr, "No adjacent video file for: %s\n", filename);
		return 2;
	}

	strlist_resize(&outbuf, 0);

	const char *params[] = {"ffprobe", "-select_streams", "v", "-of", "default=noprint_wrappers=1:nokey=1", "-show_entries", "stream=r_frame_rate", namebuf.data, NULL};
	if (exec_and_wait(params[0], params, &outbuf)) {
		return 2;
	}

	char *s = outbuf.data;
	unsigned long nomin, denom;

	int err = parse_unsigned_long_strict(&s, &nomin);
	err += parse_char_match(&s, '/');
	err += parse_unsigned_long_strict(&s, &denom);

	if (err || nomin <= 0 || denom <= 0) {
		fprintf(stderr, "Invalid ffprobe response for file: %s\n", namebuf.data);
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

	strlist_resize(&outbuf, 0);

	unsigned long linenum = 0;
	while (1) {
		strlist_resize(&inbuf, 0);
		if (strlist_readline(&inbuf, input) == -1) {
			break;
		}
		linenum++;

		int err = parse_line_end(inbuf.data, &inbuf.size, NULL);
		char *s = inbuf.data;
		unsigned long from, to;
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
		size_t written = (size_t) sprintf(buffer, "%lu\r\n", linenum);
		strlist_add_sn(&outbuf, buffer, written);

		char srtbuf1[64], srtbuf2[64];
		print_srt_time(srtbuf1, (unsigned long) ((float) from / fps * 1000));
		print_srt_time(srtbuf2, (unsigned long) ((float) to / fps * 1000));
		written = (size_t) sprintf(buffer, "%s --> %s\r\n", srtbuf1, srtbuf2);
		strlist_add_sn(&outbuf, buffer, written);

		int italic = 0;
		while (*s) {
			char c = *s++;
			if (c == '|') {
				if (italic) {
					strlist_add_sn(&outbuf, "</i>", strlen("</i>"));
					italic = 0;
				}
				strlist_add_sn(&outbuf, "\r\n", strlen("\r\n"));
			} else if (c == '{') {
				if (!strncmp(s, "y:i}", 4)) {
					strlist_add_sn(&outbuf, "<i>", strlen("<i>"));
					italic = 1;
					s += 4;
				} else {
					fprintf(stderr, "%s: line %lu: Invalid format\n", filename, linenum);
					return 2;
				}
			} else {
				strlist_add(&outbuf, c);
			}
		}
		if (italic) {
			strlist_add_sn(&outbuf, "</i>", strlen("</i>"));
		}

		strlist_add_sn(&outbuf, "\r\n\r\n", strlen("\r\n\r\n"));
	}
	if (!feof(input)) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	strlist_resize(&namebuf, 0);
	strlist_add_sn(&namebuf, filename, (size_t) (dot - filename));
	strlist_add_sn(&namebuf, ".srt", strlen(".srt"));

	FILE *output = fopen(namebuf.data, "w");
	if (!output) {
		fprintf(stderr, "Error opening file %s for writing: %s\n", namebuf.data, strerror(errno));
		return 2;
	}

	int ret = 0;

	if (fwrite(outbuf.data, outbuf.size, 1, output) != 1) {
		fprintf(stderr, "Error writing file %s\n", namebuf.data);
		ret = 2;
	}

	if (fclose(output)) {
		fprintf(stderr, "Error closing file %s for writing: %s\n", namebuf.data, strerror(errno));
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

	strlist_init(&inbuf);
	strlist_init(&outbuf);
	strlist_init(&namebuf);

	int ret = 0;

	while (*argv) {
		if (process_filename(*argv++)) {
			ret = 2;
		}
	}

	strlist_destroy(&inbuf);
	strlist_destroy(&outbuf);
	strlist_destroy(&namebuf);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
