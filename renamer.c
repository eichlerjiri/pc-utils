#include "common.c"
#include "alist.c"
#include <ctype.h>

static void replace_char(char *str, char from, char to) {
	while (*str) {
		if (*str == from) {
			*str = to;
		}
		str++;
	}
}

static void trim(char *str) {
	char *pos = str;
	char *end = str;
	int some = 0;
	while (*pos) {
		if (!isspace(*pos)) {
			end = str + 1;
			some = 1;
		}

		if (some) {
			*str = *pos;
			str++;
		}
		pos++;
	}
	*end = '\0';
}

static void free_list(struct alist *list) {
	for (size_t i = 0; i < list->size; i++) {
		c_free(list->data[i]);
	}
	alist_destroy(list);
}

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s <files to be renamed>\n", pname);
		return 3;
	}

	char tmpfilename[] = "/tmp/renamer XXXXXX.txt";
	FILE *tmp = c_fdopen(c_mkstemps(tmpfilename, 4), "w");

	size_t cnt = 0;
	do {
		replace_char(argv[cnt], '\n', '?');
		c_fprintf(tmp, "%s\n", argv[cnt]);
	} while(argv[++cnt]);

	c_fclose(tmp);

	char p0[] = "vim";
	char *params[] = {p0, tmpfilename, NULL};
	c_execvp_wait(p0, params);

	struct alist list;
	alist_init(&list);

	tmp = c_fopen(tmpfilename, "r");

	char *lineptr = NULL;
	size_t n = 0;
	while (c_getline(&lineptr, &n, tmp) >= 0) {
		char *line = c_strdup(lineptr);
		trim(line);
		alist_add(&list, line);
	}
	c_free(lineptr);

	c_fclose(tmp);
	c_remove(tmpfilename);

	if (cnt != list.size) {
		free_list(&list);
		fatal("Different number of lines: original %li, new %li", cnt, list.size);
	}

	for (size_t i = 0; i < cnt; i++) {
		char *old = argv[i];
		char *new = list.data[i];

		if (strcmp(old, new)) {
			if (access(old, F_OK)) {
				error("File %s does not exist.", old);
			} else if (!access(new, F_OK)) {
				error("File %s already exists.", new);
			} else if (rename(old, new)) {
				error("Cannot rename %s: %s", old, c_strerror(errno));
			}
		}
	}

	free_list(&list);
	return return_code;
}
