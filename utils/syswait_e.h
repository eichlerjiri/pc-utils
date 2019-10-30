static pid_t waitpid_e(pid_t pid, int *wstatus, int options) {
	pid_t ret = waitpid(pid, wstatus, options);
	if (ret <= 0) {
		fprintf(stderr, "Error waitpid: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
