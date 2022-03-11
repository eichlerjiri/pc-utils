static int exec_and_wait(const char *file, const char **argv, struct strlist *output) {
	int fildes[2];
	if (output) {
		if (pipe(fildes)) {
			fprintf(stderr, "Error pipe: %s\n", strerror(errno));
			exit(3);
		}
	}

	int pid = fork();
	if (pid == -1) {
		fprintf(stderr, "Error fork: %s\n", strerror(errno));
		exit(3);
	} else if (pid) {
		int ret = 0;

		if (output) {
			close(fildes[1]);

			ssize_t n;
			char buffer[4096];
			while ((n = read(fildes[0], buffer, sizeof(buffer))) > 0) {
				strlist_add_sn(output, buffer, (size_t) n);
			}
			if (n == -1) {
				fprintf(stderr, "Error read: %s\n", strerror(errno));
				ret = 2;
			}

			close(fildes[0]);
		}

		int wstatus = 0;
		pid_t wret = waitpid(pid, &wstatus, 0);
		if (wret <= 0) {
			fprintf(stderr, "Error waitpid: %s\n", strerror(errno));
			exit(3);
		}
		if (!WIFEXITED(wstatus)) {
			fprintf(stderr, "Program %s didn't terminate normally\n", file);
			return 2;
		}
		int code = WEXITSTATUS(wstatus);
		if (code) {
			fprintf(stderr, "Program %s exited with error code %i\n", file, code);
			return 2;
		}

		return ret;
	} else {
		if (output) {
			close(fildes[0]);

			if (dup2(fildes[1], 1) == -1) {
				fprintf(stderr, "Error dup2: %s\n", strerror(errno));
				exit(2);
			}
		}

		execvp(file, (char *const*) argv);
		fprintf(stderr, "Error starting %s: %s\n", file, strerror(errno));
		exit(2);
	}
}
