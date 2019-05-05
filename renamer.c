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

int main(int argc, char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		fprintf(stderr, "Usage: %s <files to be renamed>\n", pname);
		return 2;
	}

	char tmpfilename[] = "/tmp/renamer XXXXXX.txt";
	FILE *tmp = c_fdopen(c_mkstemps(tmpfilename, 4), "w");

	size_t cnt = 0;
	do {
		replace_char(argv[cnt], '\n', '?');
		c_fprintf(tmp, "%s\n", argv[cnt]);
	} while(argv[++cnt]);

	c_fclose(tmp);

	int pid = c_fork();
	if (pid) {
		c_waitpid(pid);
	} else {
		char p0[] = "vim";
		char *params[] = {p0, tmpfilename, NULL};
		c_execvp(p0, params);
	}

	struct alist list;
	alist_init(&list);

	tmp = c_fopen(tmpfilename, "r");

	char *lineptr = NULL;
	size_t n = 0;
	while (c_getline_nonull(&lineptr, &n, tmp) != -1) {
		trim(lineptr);
		alist_add(&list, c_strdup(lineptr));
	}
	c_free(lineptr);

	c_fclose(tmp);
	c_remove(tmpfilename);

	if (cnt != list.size) {
		fatal("Different number of lines: original %li, new %li", cnt, list.size);
	}

	for (size_t i = 0; i < cnt; i++) {
		char *old = argv[i];
		char *new = list.data[i];

		if (strcmp(old, new)) {
			if (access(old, F_OK)) {
				fatal("File %s does not exist", old);
			} else if (!access(new, F_OK)) {
				fatal("File %s already exists", new);
			} else {
				c_rename(old, new);
			}
		}
	}

	for (size_t i = 0; i < list.size; i++) {
		c_free(list.data[i]);
	}
	alist_destroy(&list);
	c_fflush(stdout);
	return 0;
}
