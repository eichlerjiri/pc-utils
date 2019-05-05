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
		return 2;
	}

	char *filename = *argv++;
	if (*argv) {
		fatal("Too many arguments");
	}

	FILE *input = c_fopen(filename, "r");

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
	while (c_getline_nonull(&lineptr, &n, input) != -1) {
		linenum++;
		if (sscanf(lineptr, "%99s %99s %99s %99s", flag, type, address, function) != 4
		|| (flag[0] != 'A' && flag[0] != 'F')) {
			fatal("Line %lu: Invalid format", linenum);
		}

		sprintf(buffer, "%s %s", type, address);
		if (flag[0] == 'A') {
			if (hmap_get(&map, buffer)) {
				c_printf("Line %lu: Repeated alloc: %s %s\n", linenum, buffer, function);
			} else {
				hmap_put(&map, c_strdup(buffer), c_strdup(function), NULL, NULL);
			}
		} else {
			if (!hmap_get(&map, buffer)) {
				c_printf("Line %lu: Free before alloc: %s %s\n", linenum, buffer, function);
			} else {
				void *rkey = NULL;
				void *rvalue = NULL;
				hmap_remove(&map, buffer, &rkey, &rvalue);
				c_free(rkey);
				c_free(rvalue);
			}
		}
	}

	if (map.size) {
		c_printf("Remaining:\n");
		for (int i = 0; i < map.capacity; i++) {
			struct hmap_item *item = map.data[i];
			while (item) {
				char *key = item->key;
				char *value = item->value;
				c_printf("\t%s %s\n", key, value);
				c_free(key);
				c_free(value);

				item = item->next;
			}
		}
	}

	hmap_destroy(&map);
	c_free(lineptr);
	c_fclose(input);
	c_fflush(stdout);
	return 0;
}
