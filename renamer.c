#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "utils/stdlib_utils.h"
#include "utils/stdio_utils.h"
#include "utils/string_utils.h"
#include "utils/unistd_utils.h"
#include "utils/alist.h"

static int rename_file(char *old, char *new) {
	if (strcmp(old, new)) {
		if (access(old, F_OK)) {
			fprintf(stderr, "File %s does not exist\n", old);
			return 2;
		} else if (!access(new, F_OK)) {
			fprintf(stderr, "File %s already exists\n", new);
			return 2;
		} else if (rename(old, new)) {
			fprintf(stderr, "Error renaming %s to %s: %s\n", old, new, strerror(errno));
			return 2;
		} else {
			printf_safe("Renamed %s to %s\n", old, new);
		}
	}
	return 0;
}

static int process_rename_list(char **argv, size_t cnt, FILE *tmp, char **in, size_t *insize, struct alist *list) {
	while (getline_no_eol_safe(in, insize, tmp) != -1) {
		alist_add(list, strdup_safe(*in));
	}

	if (cnt != list->size) {
		fprintf(stderr, "Different number of lines: original %li, new %li\n", cnt, list->size);
		return 2;
	}

	int ret = 0;
	for (size_t i = 0; i < cnt; i++) {
		if (rename_file(argv[i], list->data[i])) {
			ret = 2;
		}
	}
	return ret;
}

static int edit_and_rename(char **argv, size_t cnt, char *tmpfilename) {
	const char *params[] = {"vim", tmpfilename, NULL};
	if (exec_and_wait("vim", params)) {
		return 2;
	}

	struct alist list;
	alist_init(&list);

	FILE *tmp = fopen_safe(tmpfilename, "r");

	char *in = NULL;
	size_t insize = 0;

	int ret = process_rename_list(argv, cnt, tmp, &in, &insize, &list);

	free(in);
	fclose_safe(tmp);
	alist_destroy(&list);

	return ret;
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_safe("Usage: %s <files to be renamed>\n", pname);
		return 1;
	}

	size_t cnt = 0;
	while (argv[cnt]) {
		if (strchr(argv[cnt++], '\n')) {
			fprintf(stderr, "Filenames with newline not supported\n");
			return 2;
		}
	}

	char tmpfilename[] = "/tmp/renamer XXXXXX.txt";
	FILE *tmp = fdopen_safe(mkstemps_safe(tmpfilename, 4), "w");
	for (size_t i = 0; i < cnt; i++) {
		fprintf_safe(tmp, "%s\n", argv[i]);
	}
	fclose_safe(tmp);

	int ret = edit_and_rename(argv, cnt, tmpfilename);
	remove_safe(tmpfilename);
	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	fflush_safe(stdout);
	return ret;
}
