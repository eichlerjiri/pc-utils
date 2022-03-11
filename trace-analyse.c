#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "utils/stdlib_utils.h"
#include "utils/stdio_utils.h"
#include "utils/string_utils.h"
#include "utils/strlist.h"
#include "utils/hmap.h"
#include "utils/parser.h"

size_t insize;
char *inbuf;
struct hmap map;
struct strlist sb;

static int parse_line(char *in, char **flag, char **type, char **function) {
	strlist_resize(&sb, 0);

	size_t flag_pos = sb.size;
	int err = parse_word(&in, &sb);
	strlist_add(&sb, '\0');

	size_t type_pos = sb.size;
	err += parse_word(&in, &sb);
	strlist_add(&sb, ' ');
	err += parse_word(&in, &sb);
	strlist_add(&sb, '\0');

	size_t function_pos = sb.size;
	err += parse_word(&in, &sb);

	*flag = sb.data + flag_pos;
	*type = sb.data + type_pos;
	*function  = sb.data + function_pos;
	return err;
}

static int process_file(FILE *input, char *filename) {
	unsigned long linenum = 0;
	while (getline_no_eol(&inbuf, &insize, input) != -1) {
		linenum++;

		char *flag, *type, *function;
		if (parse_line(inbuf, &flag, &type, &function) || (strcmp(flag, "A") && strcmp(flag, "F"))) {
			fprintf(stderr, "Line %lu: Invalid format\n", linenum);
			return 2;
		}

		if (!strcmp(flag, "A")) {
			if (hmap_get(&map, type)) {
				printf_safe("Line %lu: Repeated alloc: %s %s\n", linenum, type, function);
			} else {
				hmap_put(&map, strdup_safe(type), strdup_safe(function));
			}
		} else {
			if (!hmap_get(&map, type)) {
				printf_safe("Line %lu: Free before alloc: %s %s\n", linenum, type, function);
			} else {
				hmap_remove(&map, type);
			}
		}
	}
	if (!feof(input)) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		return 2;
	}
	if (map.size) {
		printf_safe("Remaining:\n");
		for (int i = 0; i < map.capacity; i++) {
			struct hmap_item *item = map.data[i];
			while (item) {
				char *key = item->key;
				char *value = item->value;
				printf_safe("\t%s %s\n", key, value);
				item = item->next;
			}
		}
	}
	return 0;
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_safe("Usage: %s <trace.txt file>\n", pname);
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

	hmap_init(&map, hash_str, equals_str, free, free);
	strlist_init(&sb);

	int ret = process_file(input, filename);

	free(inbuf);
	hmap_destroy(&map);
	strlist_destroy(&sb);

	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
