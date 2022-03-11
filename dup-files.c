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
#include "utils/strlist.h"
#include "utils/hmap.h"
#include "utils/crc64.h"

unsigned long crc64_table[256];
struct hmap map;

static int process_file(struct strlist *path) {
	int input = open(path->data, O_RDONLY);
	if (input == -1) {
		fprintf(stderr, "Error opening file %s: %s\n", path->data, strerror(errno));
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
		fprintf(stderr, "Error reading file %s: %s\n", path->data, strerror(errno));
		ret = 2;
	}
	if (close(input) == -1) {
		fprintf(stderr, "Error closing file %s: %s\n", path->data, strerror(errno));
		ret = 2;
	}

	if (!ret) {
		char *path_dup = hmap_get(&map, (void*) crc);
		if (path_dup) {
			printf_safe("Duplicate file %s - the same as %s\n", path->data, path_dup);
		} else {
			hmap_put(&map, (void*) crc, strdup_safe(path->data));
		}
	}

	return ret;
}

static int process_item(struct strlist *path, unsigned char type_hint) {
	if (type_hint == DT_UNKNOWN || type_hint == DT_LNK) {
		struct stat statbuf;
		if (stat(path->data, &statbuf)) {
			fprintf(stderr, "Error retrieving entry %s: %s\n", path->data, strerror(errno));
			return 2;
		}
		if (S_ISDIR(statbuf.st_mode)) {
			type_hint = DT_DIR;
		}
	}

	if (type_hint == DT_REG) {
		return process_file(path);
	} else if (type_hint != DT_DIR) {
		return 0;
	}

	DIR *d = opendir(path->data);
	if (!d) {
		fprintf(stderr, "Error opening directory %s: %s\n", path->data, strerror(errno));
		return 2;
	}

	int ret = 0;

	struct dirent *de;
	while (!(errno = 0) && (de = readdir(d))) {
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			size_t path_size = path->size;

			if (path->data[path->size - 1] != '/') {
				strlist_add(path, '/');
			}
			strlist_add_s(path, de->d_name);

			if (process_item(path, de->d_type)) {
				ret = 2;
			}

			strlist_resize(path, path_size);
		}
	}
	if (errno) {
		fprintf(stderr, "Error reading directory %s: %s\n", path->data, strerror(errno));
		ret = 2;
	}
	if (closedir(d)) {
		fprintf(stderr, "Error closing directory %s: %s\n", path->data, strerror(errno));
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
	hmap_init(&map, hash_ptr, equals_ptr, nofree, free);

	struct strlist path;
	strlist_init(&path);

	strlist_add_s(&path, dir);

	int ret = process_item(&path, DT_DIR);

	strlist_destroy(&path);

	hmap_destroy(&map);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
