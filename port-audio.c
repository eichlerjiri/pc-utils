#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "utils/stdio_utils.h"
#include "utils/stdlib_utils.h"
#include "utils/dirent_utils.h"
#include "utils/strlist.h"
#include "utils/exec.h"

static int add_to_path(struct strlist *path, const char *subpath, int do_mkdir) {
	if (path->size && path->data[path->size - 1] == '/') {
		strlist_resize(path, path->size - 1);
	}

	char *next;
	do {
		next = strchrnul(subpath, '/');
		if (next != subpath) {
			strlist_add(path, '/');
			strlist_add_sn(path, subpath, (size_t) (next - subpath));

			if (do_mkdir) {
				printf_safe("Creating directory: %s\n", path->data);
				if (mkdir(path->data, 0755) && errno != EEXIST) {
					fprintf(stderr, "Error creating directory %s: %s\n", path->data, strerror(errno));
					return 2;
				}
			}
		}
		subpath = next + 1;
	} while (*next);
	return 0;
}

static int process_item(struct strlist *in, struct strlist *out, const char *subpath, unsigned char type_hint) {
	add_to_path(in, subpath, 0);

	if (type_hint == DT_UNKNOWN || type_hint == DT_LNK) {
		struct stat statbuf;
		if (stat(in->data, &statbuf)) {
			fprintf(stderr, "Error retrieving entry %s: %s\n", in->data, strerror(errno));
			return 2;
		}
		if (S_ISDIR(statbuf.st_mode)) {
			type_hint = DT_DIR;
		}
	}

	if (type_hint == DT_REG) {
		add_to_path(out, subpath, 0);

		char *dot = strrchr(out->data, '.');
		if (dot) {
			if (!strcasecmp(dot, ".mp3") || !strcasecmp(dot, ".m4a")) {
				const char *params[] = {"ffmpeg", "-y", "-i", in->data, "-vn", "-codec:a", "copy", "-map_metadata", "-1", out->data, NULL};
				return exec_and_wait(params[0], params, NULL);
			} else if (!strcasecmp(dot, ".flac") || !strcasecmp(dot, ".ape")) {
				strcpy(dot, ".mp3");
				const char *params[] = {"ffmpeg", "-y", "-i", in->data, "-vn", "-ab", "320k", "-map_metadata", "-1", out->data, NULL};
				return exec_and_wait(params[0], params, NULL);
			}
		}
		return 0;

	} else if (type_hint == DT_DIR) {
		if (add_to_path(out, subpath, 1)) {
			return 2;
		}

		DIR *d = opendir_trace(in->data);
		if (!d) {
			fprintf(stderr, "Error opening directory %s: %s\n", in->data, strerror(errno));
			return 2;
		}

		int ret = 0;

		while (1) {
			errno = 0;
			struct dirent *de = readdir(d);
			if (!de) {
				break;
			}

			if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
				size_t in_size = in->size;
				size_t out_size = out->size;

				if (process_item(in, out, de->d_name, de->d_type)) {
					ret = 2;
				}

				strlist_resize(in, in_size);
				strlist_resize(out, out_size);
			}
		}
		if (errno) {
			fprintf(stderr, "Error reading directory %s: %s\n", in->data, strerror(errno));
			ret = 2;
		}
		if (closedir_trace(d)) {
			fprintf(stderr, "Error closing directory %s: %s\n", in->data, strerror(errno));
			ret = 2;
		}

		return ret;

	} else {
		return 0;
	}
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_safe("Usage: %s -in <input dir> -out <output dir> <input subfiles, subdirs>\n", pname);
		return 1;
	}

	char *in_c = NULL;
	char *out_c = NULL;

	while (*argv && *argv[0] == '-') {
		char *arg = *argv++;

		if (!strcmp(arg, "--")) {
			break;
		} else if (!strcmp(arg, "-in") && *argv) {
			in_c = *argv++;
		} else if (!strcmp(arg, "-out") && *argv) {
			out_c = *argv++;
		} else {
			fprintf(stderr, "Invalid argument: %s\n", arg);
			return 2;
		}
	}
	if (!in_c || !out_c || !*argv) {
		fprintf(stderr, "Missing arguments\n");
		return 2;
	}

	struct strlist in, out;
	strlist_init(&in);
	strlist_init(&out);

	strlist_add_sn(&in, in_c, strlen(in_c));
	strlist_add_sn(&out, out_c, strlen(out_c));

	int ret = 0;

	char *subpath;
	while ((subpath = *argv++)) {
		size_t in_size = in.size;
		size_t out_size = out.size;

		if (process_item(&in, &out, subpath, DT_DIR)) {
			ret = 2;
		}

		strlist_resize(&in, in_size);
		strlist_resize(&out, out_size);
	}

	strlist_destroy(&in);
	strlist_destroy(&out);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
