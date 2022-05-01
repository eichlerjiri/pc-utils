static int isatty_safe(int fd) {
	int ret = isatty(fd);
	if (!ret && errno != ENOTTY) {
		fprintf(stderr, "Error isatty: %s\n", strerror(errno));
	}
	return ret;
}
