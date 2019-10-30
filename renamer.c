#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "utils/stdlib_e.h"
#include "utils/stdio_e.h"
#include "utils/string_e.h"
#include "utils/unistd_e.h"
#include "utils/syswait_e.h"
#include "utils/alist.h"

static int do_rename(char *old, char *new) {
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
			printf_e("Renamed %s to %s\n", old, new);
		}
	}
	return 0;
}

static int process(char **argv, size_t cnt, FILE *tmp, char **in, size_t *insize, struct alist *list) {
	ssize_t linelen;
	while ((linelen = getline_e(in, insize, tmp)) != -1) {
		char *ind = *in;
		if (ind[linelen - 1] == '\n') {
			ind[linelen - 1] = '\0';
		}
		alist_add(list, strdup_e(ind));
	}

	if (cnt != list->size) {
		fprintf(stderr, "Different number of lines: original %li, new %li\n", cnt, list->size);
		return 2;
	}

	int ret = 0;
	for (size_t i = 0; i < cnt; i++) {
		if (do_rename(argv[i], list->data[i])) {
			ret = 2;
		}
	}
	return ret;
}

static int run(char **argv) {
	char *pname = *argv++;
	if (!*argv) {
		printf_e("Usage: %s <files to be renamed>\n", pname);
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
	FILE *tmp = fdopen_e(mkstemps_e(tmpfilename, 4), "w");

	for (size_t i = 0; i < cnt; i++) {
		fprintf_e(tmp, "%s\n", argv[i]);
	}

	fclose_e(tmp);

	int pid = fork_e();
	if (pid) {
		int wstatus = 0;
		waitpid_e(pid, &wstatus, 0);
		if (!WIFEXITED(wstatus)) {
			fprintf(stderr, "Editor didn't terminate normally\n");
			return 2;
		}
		int code = WEXITSTATUS(wstatus);
		if (code) {
			fprintf(stderr, "Editor exited with error code %i\n", code);
			return 2;
		}
	} else {
		const char *params[] = {"vim", tmpfilename, NULL};
		execvp("vim", (char**) params);
		fprintf(stderr, "Error starting vim: %s\n", strerror(errno));
		return 2;
	}

	struct alist list;
	alist_init(&list);

	tmp = fopen_e(tmpfilename, "r");

	char *in = NULL;
	size_t insize = 0;

	int ret = process(argv, cnt, tmp, &in, &insize, &list);

	free(in);

	fclose_e(tmp);
	remove_e(tmpfilename);

	alist_destroy(&list);

	return ret;
}

int main(int argc, char **argv) {
	int ret = run(argv);
	fflush_e(stdout);
	return ret;
}
