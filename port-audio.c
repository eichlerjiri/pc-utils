#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

char *combine_path(const char *dir, const char *file) {
	const char *sep = "";
	if (dir[strlen(dir) - 1] != '/') {
		sep = "/";
	}
	return asprintfx("%s%s%s", dir, sep, file);
}

int prepdir(const char *pathname) {
	printf("Creating directory: %s\n", pathname);

	int ret = mkdir(pathname, 0755);
	if (ret && errno != EEXIST) {
		error("Cannot create directory %s: %s", pathname, strerror(errno));
		return ret;
	}
	return 0;
}

int prepdir_rec(char *path, size_t base_length) {
	char *cur = path + base_length;
	char *last = NULL;

	char *new;
	do {
		new = strchrnul(cur, '/');
		if (cur != new) {
			if (last) {
				*last = '\0';
				int ret = prepdir(path);
				*last = '/';

				if (ret) return ret;
			}
			last = new;
		}
		cur = new + 1;
	} while (*new);
	return 0;
}

void process_file(char *in, char *out) {
	char *dot = strrchr(out, '.');
	if (dot) {
		if (!strcasecmp(dot, ".mp3") || !strcasecmp(dot, ".m4a")) {
			char p0[] = "ffmpeg";
			char p1[] = "-y";
			char p2[] = "-i";
			char p4[] = "-vn";
			char p5[] = "-codec:a";
			char p6[] = "copy";
			char p7[] = "-map_metadata";
			char p8[] = "-1";
			char *params[] = {p0, p1, p2, in, p4, p5, p6, p7, p8, out, NULL};
			execvpx_wait(p0, params);
		} else if(!strcasecmp(dot, ".flac") || !strcasecmp(dot, ".ape")) {
			strcpy(dot, ".mp3");
			char p0[] = "ffmpeg";
			char p1[] = "-y";
			char p2[] = "-i";
			char p4[] = "-vn";
			char p5[] = "-ab";
			char p6[] = "320k";
			char p7[] = "-map_metadata";
			char p8[] = "-1";
			char *params[] = {p0, p1, p2, in, p4, p5, p6, p7, p8, out, NULL};
			execvpx_wait(p0, params);
		}
	}
}

void process(const char *in, const char *out, const char *subpath, unsigned char type_hint, int top) {
	char *in_full = combine_path(in, subpath);
	char *out_full = combine_path(out, subpath);

	if (type_hint == DT_UNKNOWN || type_hint == DT_LNK) {
		struct stat statbuf;
		if (stat(in_full, &statbuf)) {
			error("Cannot stat %s: %s", in_full, strerror(errno));
			goto out_free;
		}
		if (S_ISDIR(statbuf.st_mode)) {
			type_hint = DT_DIR;
		}
	}

	if (top) {
		if (prepdir_rec(out_full, strlen(out))) goto out_free;
	}

	if (type_hint == DT_DIR) {
		DIR *d = opendir(in_full);
		if (!d) {
			error("Cannot open dir %s: %s", in_full, strerror(errno));
			goto out_free;
		}

		if (prepdir(out_full)) goto out_closedir;

		struct dirent *de;
		while ((errno = 0) || (de = readdir(d))) {
			if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
				process(in_full, out_full, de->d_name, de->d_type, 0);
			}
		}
		if (errno) {
			error("Cannot read dir %s: %s", in_full, strerror(errno));
		}

out_closedir:
		if (closedir(d)) {
			error("Cannot close dir %s: %s", in_full, strerror(errno));
		}
	} else {
		process_file(in_full, out_full);
	}

out_free:
	free(in_full);
	free(out_full);
}

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s -in <input dir> -out <output dir> <input subfiles, subdirs>\n", pname);
		return 3;
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
			fatal("Invalid argument: %s", arg);
		}
	}
	if (!in || !out || !*argv) {
		fatal("Missing arguments");
	}

	while (*argv) {
		process(in, out, *argv++, DT_UNKNOWN, 1);
	}

	return return_code;
}
