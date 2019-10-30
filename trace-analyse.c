#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "utils/stdlib_e.h"
#include "utils/stdio_e.h"
#include "utils/string_e.h"
#include "utils/hmap.h"

static int process(FILE *input, char *filename, char **in, size_t *insize, struct hmap *map) {
	char flag[100];
	char type[100];
	char address[100];
	char function[100];
	char buffer[200];

	unsigned long linenum = 0;
	ssize_t linelen;
	while ((linelen = getline_em(in, insize, input)) != -1) {
		linenum++;

		if (sscanf(*in, "%99s %99s %99s %99s", flag, type, address, function) != 4
		|| (flag[0] != 'A' && flag[0] != 'F')) {
			fprintf(stderr, "Line %lu: Invalid format\n", linenum);
			return 2;
		}

		sprintf(buffer, "%s %s", type, address);
		if (flag[0] == 'A') {
			if (hmap_get(map, buffer)) {
				printf_e("Line %lu: Repeated alloc: %s %s\n", linenum, buffer, function);
			} else {
				hmap_put(map, strdup_e(buffer), strdup_e(function));
			}
		} else {
			if (!hmap_get(map, buffer)) {
				printf_e("Line %lu: Free before alloc: %s %s\n", linenum, buffer, function);
			} else {
				hmap_remove(map, buffer);
			}
		}
	}
	if (errno) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		return 2;
	}
	if (map->size) {
		printf_e("Remaining:\n");
		for (int i = 0; i < map->capacity; i++) {
			struct hmap_item *item = map->data[i];
			while (item) {
				char *key = item->key;
				char *value = item->value;
				printf_e("\t%s %s\n", key, value);
				item = item->next;
			}
		}
	}
	return 0;
}

static int run(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_e("Usage: %s <trace.txt file>\n", pname);
		return 1;
	}

	char *filename = *argv++;
	if (*argv) {
		fprintf(stderr, "Too many arguments\n");
		return 2;
	}

	FILE *input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	char *in = NULL;
	size_t insize = 0;

	struct hmap map;
	hmap_init(&map, hash_str, equals_str);

	int ret = process(input, filename, &in, &insize, &map);

	hmap_destroy(&map);

	free(in);

	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	return ret;
}

int main(int argc, char **argv) {
	int ret = run(argv);
	fflush_e(stdout);
	return ret;
}
