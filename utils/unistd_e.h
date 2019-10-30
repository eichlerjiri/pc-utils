static int fork_e() {
	int ret = fork();
	if (ret == -1) {
		fprintf(stderr, "Error fork: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
