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
#include "utils/alist.h"
#include "utils/exec.h"

char *inbuf;
size_t insize;

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

static int process_rename_list(char **argv, size_t cnt, struct alist *list) {
	if (cnt != list->size) {
		fprintf(stderr, "Different number of lines: original %li, new %li\n", cnt, list->size);
		return 2;
	}

	int ret = 0;
	for (size_t i = 0; i < cnt; i++) {
		if (rename_file(argv[i], list->ptrdata[i])) {
			ret = 2;
		}
	}
	return ret;
}

static int edit_and_rename(char **argv, size_t cnt, char *tmpfilename) {
	const char *params[] = {"vim", tmpfilename, NULL};
	if (exec_and_wait(params[0], params, NULL)) {
		return 2;
	}

	FILE *tmp = fopen(tmpfilename, "r");
	if (!tmp) {
		fprintf(stderr, "Error opening file %s: %s\n", tmpfilename, strerror(errno));
		return 2;
	}

	int ret = 0;

	struct alist list;
	alist_init_ptr(&list);

	while (getline_no_eol(&inbuf, &insize, tmp) != -1) {
		alist_add_ptr(&list, strdup_safe(inbuf));
	}
	if (!feof(tmp)) {
		fprintf(stderr, "Error reading file %s: %s\n", tmpfilename, strerror(errno));
		ret = 2;
	}
	if (fclose(tmp)) {
		fprintf(stderr, "Error closing file %s: %s\n", tmpfilename, strerror(errno));
		ret = 2;
	}

	if (!ret) {
		ret = process_rename_list(argv, cnt, &list);
	}

	alist_destroy_ptr(&list);

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
	int fd = mkstemps(tmpfilename, 4);
	if (!fd) {
		fprintf(stderr, "Error creating file %s: %s\n", tmpfilename, strerror(errno));
		return 2;
	}

	int ret = 0;

	FILE *tmp = fdopen(fd, "w");
	if (!tmp) {
		fprintf(stderr, "Error opening file %s: %s\n", tmpfilename, strerror(errno));
		ret = 2;
	}

	if (!ret) {
		for (size_t i = 0; i < cnt; i++) {
			if (fprintf(tmp, "%s\n", argv[i]) < 0) {
				fprintf(stderr, "Error reading file %s: %s\n", tmpfilename, strerror(errno));
				ret = 2;
				break;
			}
		}
		if (fclose(tmp)) {
			fprintf(stderr, "Error closing file %s: %s\n", tmpfilename, strerror(errno));
			ret = 2;
		}
	}

	if (!ret) {
		ret = edit_and_rename(argv, cnt, tmpfilename);
	}

	if (remove(tmpfilename)) {
		fprintf(stderr, "Error removing file %s: %s\n", tmpfilename, strerror(errno));
		ret = 2;
	}

	free(inbuf);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
