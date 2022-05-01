#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "utils/stdio_utils.h"
#include "utils/stdlib_utils.h"
#include "utils/string_utils.h"
#include "utils/strlist.h"
#include "utils/hmap.h"
#include "utils/parser.h"

struct strlist inbuf;
struct hmap map;
struct strlist sb;

static int process_file(FILE *input, char *filename) {
	unsigned long linenum = 0;
	while (1) {
		strlist_resize(&inbuf, 0);
		if (strlist_readline(&inbuf, input) == -1) {
			break;
		}
		linenum++;

		char *flag, *type, *ptr, *function;
		size_t flag_len, type_len, ptr_len, function_len;

		int err = parse_line_end(inbuf.data, &inbuf.size, NULL);
		char *in = inbuf.data;
		err += parse_word(&in, &flag, &flag_len);
		err += parse_word(&in, &type, &type_len);
		err += parse_word(&in, &ptr, &ptr_len);
		err += parse_word(&in, &function, &function_len);

		if (err || (strncmp("A", flag, flag_len) && strncmp("F", flag, flag_len))) {
			fprintf(stderr, "Line %lu: Invalid format\n", linenum);
			return 2;
		}

		strlist_resize(&sb, 0);
		strlist_add_sn(&sb, type, type_len);
		strlist_add(&sb, ' ');
		strlist_add_sn(&sb, ptr, ptr_len);

		if (!strncmp("A", flag, flag_len)) {
			if (hmap_get(&map, sb.data)) {
				printf_safe("Line %lu: Repeated alloc: %s %s\n", linenum, sb.data, function);
			} else {
				hmap_put(&map, memdup_safe(sb.data, sb.size + 1), memdup_safe(function, inbuf.size - (size_t) (function - inbuf.data) + 1));
			}
		} else {
			if (!hmap_get(&map, sb.data)) {
				printf_safe("Line %lu: Free before alloc: %s %s\n", linenum, sb.data, function);
			} else {
				hmap_remove(&map, sb.data);
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

	FILE *input = fopen_trace(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	strlist_init(&inbuf);
	hmap_init(&map, hash_str, equals_str, free_trace, free_trace);
	strlist_init(&sb);

	int ret = process_file(input, filename);

	strlist_destroy(&inbuf);
	hmap_destroy(&map);
	strlist_destroy(&sb);

	if (fclose_trace(input)) {
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
