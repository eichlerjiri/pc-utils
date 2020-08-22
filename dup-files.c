#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "utils/stdio_utils.h"
#include "utils/stdlib_utils.h"
#include "utils/string_utils.h"
#include "utils/alist.h"
#include "utils/hmap.h"
#include "utils/crc64.h"

unsigned long crc64_table[256];
struct hmap map;

static int process_file(struct alist *path) {
	int input = open(path->cdata, O_RDONLY);
	if (input == -1) {
		fprintf(stderr, "Error opening file %s: %s\n", path->cdata, strerror(errno));
		return 2;
	}

	int ret = 0;
	unsigned long crc = 0;

	unsigned char buf[4096];
	ssize_t cnt;
	while ((cnt = read(input, buf, sizeof(buf))) > 0) {
		for (ssize_t i = 0; i < cnt; i++) {
			crc64(crc64_table, &crc, buf[i]);
		}
	}
	if (cnt == -1) {
		fprintf(stderr, "Error reading file %s: %s\n", path->cdata, strerror(errno));
		ret = 2;
	}
	if (close(input) == -1) {
		fprintf(stderr, "Error closing file %s: %s\n", path->cdata, strerror(errno));
		ret = 2;
	}

	if (!ret) {
		char *path_dup = hmap_get(&map, (void*) crc);
		if (path_dup) {
			printf_safe("Duplicate file %s - the same as %s\n", path->cdata, path_dup);
		} else {
			hmap_put(&map, (void*) crc, strdup_safe(path->cdata));
		}
	}

	return ret;
}

static int process_dir(struct alist *path);
static int process_dir_item(struct alist *path, unsigned char type_hint) {
	if (type_hint == DT_UNKNOWN || type_hint == DT_LNK) {
		struct stat statbuf;
		if (stat(path->cdata, &statbuf)) {
			fprintf(stderr, "Error retrieving entry %s: %s\n", path->cdata, strerror(errno));
			return 2;
		}
		if (S_ISDIR(statbuf.st_mode)) {
			type_hint = DT_DIR;
		}
	}

	if (type_hint == DT_DIR) {
		return process_dir(path);
	} else {
		return process_file(path);
	}
}

static int process_dir(struct alist *path) {
	DIR *d = opendir(path->cdata);
	if (!d) {
		fprintf(stderr, "Error opening directory %s: %s\n", path->cdata, strerror(errno));
		return 2;
	}

	int ret = 0;

	struct dirent *de;
	while (!(errno = 0) && (de = readdir(d))) {
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			size_t path_size = path->size;

			if (path->cdata[path->size - 1] != '/') {
				alist_add_c(path, '/');
			}
			alist_add_s(path, de->d_name);

			if (process_dir_item(path, de->d_type)) {
				ret = 2;
			}

			alist_rem_c(path, path->size - path_size);
		}
	}
	if (errno) {
		fprintf(stderr, "Error reading directory %s: %s\n", path->cdata, strerror(errno));
		ret = 2;
	}
	if (closedir(d)) {
		fprintf(stderr, "Error closing directory %s: %s\n", path->cdata, strerror(errno));
		ret = 2;
	}

	return ret;
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_safe("Usage: %s <directory to search>\n", pname);
		return 1;
	}

	char *dir = *argv++;
	if (*argv) {
		fprintf(stderr, "Too many arguments\n");
		return 2;
	}

	generate_crc64_table(crc64_table);
	hmap_init(&map, hash_ptr, equals_ptr, 0);

	struct alist path;
	alist_init_c(&path);

	alist_add_s(&path, dir);

	int ret = process_dir(&path);

	alist_destroy_c(&path);

	hmap_destroy(&map);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	fflush_safe(stdout);
	return ret;
}
