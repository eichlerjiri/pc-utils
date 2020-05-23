static int exec_and_wait(const char *file, const char **argv) {
	int pid = fork();
	if (pid == -1) {
		fprintf(stderr, "Error fork: %s\n", strerror(errno));
		exit(3);
	} else if (pid) {
		int wstatus = 0;
		pid_t ret = waitpid(pid, &wstatus, 0);
		if (ret <= 0) {
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
		return 0;
	} else {
		execvp(file, (char *const*) argv);
		fprintf(stderr, "Error starting %s: %s\n", file, strerror(errno));
		exit(2);
	}
}
