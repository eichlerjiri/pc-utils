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
#include "utils/stdlib_e.h"
#include "utils/stdio_e.h"
#include "utils/unistd_e.h"
#include "utils/syswait_e.h"

static char *combine_path(const char *dir, const char *file) {
	char *ret;
	if (dir[strlen(dir) - 1] == '/' || file[0] == '/') {
		asprintf_e(&ret, "%s%s", dir, file);
	} else {
		asprintf_e(&ret, "%s/%s", dir, file);
	}
	return ret;
}

static int prepdir(const char *pathname) {
	printf_e("Creating directory: %s\n", pathname);
	if (mkdir(pathname, 0755) && errno != EEXIST) {
		fprintf(stderr, "Error creating directory %s: %s\n", pathname, strerror(errno));
		return 2;
	}
	return 0;
}

static int prepdir_rec(char *path, size_t base_length) {
	char *cur = path + base_length;
	while (1) {
		char *new = strchrnul(cur, '/');
		if (new != cur) {
			char orig = *new;
			*new = '\0';
			int ret = prepdir(path);
			*new = orig;
			if (ret) {
				return ret;
			}
		}
		if (!*new) {
			return 0;
		}
		cur = new + 1;
	}
}

static int full_exec(const char *file, char *const argv[]) {
	int pid = fork_e();
	if (pid) {
		int wstatus = 0;
		waitpid_e(pid, &wstatus, 0);
		if (!WIFEXITED(wstatus)) {
			fprintf(stderr, "Editor didn't terminate normally\n");
			return 2;
		}
		int code = WEXITSTATUS(wstatus);
		if (code) {
			fprintf(stderr, "Editor exited with error code %i\n", code);
			return 2;
		}
	} else {
		execvp(file, argv);
		fprintf(stderr, "Error starting %s: %s\n", file, strerror(errno));
		return 2;
	}
	return 0;
}

static int process_file(char *in, char *out) {
	char *dot = strrchr(out, '.');
	if (dot) {
		if (!strcasecmp(dot, ".mp3") || !strcasecmp(dot, ".m4a")) {
			const char *params[] = {"ffmpeg", "-y", "-i", in, "-vn", "-codec:a", "copy",
				"-map_metadata", "-1", out, NULL};
			return full_exec("ffmpeg", (char**) params);
		} else if(!strcasecmp(dot, ".flac") || !strcasecmp(dot, ".ape")) {
			strcpy(dot, ".mp3");
			const char *params[] = {"ffmpeg", "-y", "-i", in, "-vn", "-ab", "320k",
				"-map_metadata", "-1", out, NULL};
			return full_exec("ffmpeg", (char**) params);
		}
	}
	return 0;
}

static int process(char *in, char *out, unsigned char type_hint, int create);
static int process_dir(char *in, char *out, DIR *d) {
	int ret = 0;
	struct dirent *de;
	while (!(errno = 0) && (de = readdir(d))) {
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			char *in_full = combine_path(in, de->d_name);
			char *out_full = combine_path(out, de->d_name);
			if (process(in_full, out_full, de->d_type, 1)) {
				ret = 2;
			}
			free(in_full);
			free(out_full);
		}
	}
	if (errno) {
		fprintf(stderr, "Error reading directory %s: %s\n", in, strerror(errno));
		return 2;
	}
	return ret;
}

static int process(char *in, char *out, unsigned char type_hint, int create) {
	if (type_hint == DT_UNKNOWN || type_hint == DT_LNK) {
		struct stat statbuf;
		if (stat(in, &statbuf)) {
			fprintf(stderr, "Error retrieving entry %s: %s\n", in, strerror(errno));
			return 2;
		}
		if (S_ISDIR(statbuf.st_mode)) {
			type_hint = DT_DIR;
		}
	}

	if (type_hint == DT_DIR) {
		if (create) {
			if (prepdir(out)) {
				return 2;
			}
		}

		DIR *d = opendir(in);
		if (!d) {
			fprintf(stderr, "Error opening directory %s: %s\n", in, strerror(errno));
			return 2;
		}

		int ret = process_dir(in, out, d);

		if (closedir(d)) {
			fprintf(stderr, "Error closing directory %s: %s\n", in, strerror(errno));
			return 2;
		}

		return ret;
	} else {
		return process_file(in, out);
	}
}

static int run(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_e("Usage: %s -in <input dir> -out <output dir> <input subfiles, subdirs>\n", pname);
		return 1;
	}

	char *in = NULL;
	char *out = NULL;

	while (*argv && *argv[0] == '-') {
		char *arg = *argv++;

		if (!strcmp(arg, "--")) {
			break;
		} else if (!strcmp(arg, "-in") && *argv) {
			in = *argv++;
		} else if (!strcmp(arg, "-out") && *argv) {
			out = *argv++;
		} else {
			fprintf(stderr, "Invalid argument: %s\n", arg);
			return 2;
		}
	}
	if (!in || !out || !*argv) {
		fprintf(stderr, "Missing arguments\n");
		return 2;
	}

	int ret = 0;
	while (*argv) {
		char *subpath = *argv++;
		char *in_full = combine_path(in, subpath);
		char *out_full = combine_path(out, subpath);
		if (prepdir_rec(out_full, strlen(out)) || process(in_full, out_full, DT_DIR, 0)) {
			ret = 2;
		}
		free(in_full);
		free(out_full);
	}
	return ret;
}

int main(int argc, char **argv) {
	int ret = run(argv);
	fflush_e(stdout);
	return ret;
}
