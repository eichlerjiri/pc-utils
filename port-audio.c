#include "common.c"

static char *combine_path(const char *dir, const char *file) {
	const char *sep = "";
	if (dir[strlen(dir) - 1] != '/') {
		sep = "/";
	}
	return c_asprintf("%s%s%s", dir, sep, file);
}

static void prepdir(const char *pathname) {
	c_printf("Creating directory: %s\n", pathname);
	c_mkdir_tryexist(pathname, 0755);
}

static void prepdir_rec(char *path, size_t base_length) {
	char *cur = path + base_length;
	char *last = NULL;

	char *new;
	do {
		new = strchrnul(cur, '/');
		if (cur != new) {
			if (last) {
				*last = '\0';
				prepdir(path);
				*last = '/';
			}
			last = new;
		}
		cur = new + 1;
	} while (*new);
}

static void full_exec(const char *file, char *const argv[]) {
	int pid = c_fork();
	if (pid) {
		c_waitpid(pid);
	} else {
		c_execvp(file, argv);
	}
}

static void process_file(char *in, char *out) {
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
			full_exec(p0, params);
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
			full_exec(p0, params);
		}
	}
}

static void process(const char *in, const char *out, const char *subpath, unsigned char type_hint, int top) {
	char *in_full = combine_path(in, subpath);
	char *out_full = combine_path(out, subpath);

	if (type_hint == DT_UNKNOWN || type_hint == DT_LNK) {
		struct stat statbuf;
		c_stat(in_full, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			type_hint = DT_DIR;
		}
	}

	if (top) {
		prepdir_rec(out_full, strlen(out));
	}

	if (type_hint == DT_DIR) {
		DIR *d = c_opendir(in_full);

		prepdir(out_full);

		struct dirent *de;
		while ((de = readdir(d))) {
			if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
				process(in_full, out_full, de->d_name, de->d_type, 0);
			}
		}

		closedir(d);
	} else {
		process_file(in_full, out_full);
	}

	c_free(in_full);
	c_free(out_full);
}

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s -in <input dir> -out <output dir> <input subfiles, subdirs>\n", pname);
		return 2;
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

	c_fflush(stdout);
	return 0;
}
