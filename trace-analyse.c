#include "common.c"
#include "hmap.c"

static size_t hash_str(const void* key) {
	const unsigned char* str = key;
	size_t ret = 1u;
	while (*str) {
		ret = 31u * ret + *str++;
	}
	return ret;
}

static int equals_str(const void* key1, const void* key2) {
	const char* str1 = key1;
	const char* str2 = key2;
	return !strcmp(str1, str2);
}

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s <trace.txt file>\n", pname);
		return 3;
	}

	char *filename = *argv++;

	if (*argv) {
		fatal("Too many arguments");
	}

	FILE *input = fopen(filename, "r");
	if (!input) {
		fatal("Cannot open %s: %s", filename, c_strerror(errno));
	}

	struct hmap map;
	hmap_init(&map, hash_str, equals_str);

	char flag[100];
	char type[100];
	char address[100];
	char function[100];
	char buffer[200];

	unsigned long linenum = 0;
	char *lineptr = NULL;
	size_t n = 0;
	while (c_getline_tryin(&lineptr, &n, input) >= 0) {
		linenum++;
		if (sscanf(lineptr, "%99s %99s %99s %99s", flag, type, address, function) != 4 ||
		(flag[0] != 'A' && flag[0] != 'F')) {
			fatal("Line %i: Invalid format", linenum);
		}

		sprintf(buffer, "%s %s", type, address);
		if (flag[0] == 'A') {
			if (hmap_get(&map, buffer)) {
				printf("Line %lu: Repeated alloc: %s %s\n", linenum, buffer, function);
			} else {
				hmap_put(&map, c_strdup(buffer), c_strdup(function));
			}
		} else {
			if (!hmap_get(&map, buffer)) {
				printf("Line %lu: Free before alloc: %s %s\n", linenum, buffer, function);
			} else {
				hmap_remove(&map, buffer);
			}
		}
	}
	if (errno) {
		fatal("Cannot read %s: %s", filename, c_strerror(errno));
	}

	if (map.size) {
		printf("Remaining:\n");
		for (int i = 0; i < map.capacity; i++) {
			struct hmap_item **link = map.data + i;
			struct hmap_item *item;
			while ((item = *link)) {
				char *key = item->key;
				char *value = item->value;
				printf("\t%s %s\n", key, value);

				struct hmap_item **next = &item->next;
				hmap_remove_direct(&map, link);
				link = next;
			}
		}
	}

	hmap_destroy(&map);
	c_free(lineptr);
	if (fclose(input)) {
		fatal("Cannot close %s: %s", filename, c_strerror(errno));
	}
	return return_code;
}
