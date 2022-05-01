static int open_trace(const char *pathname, int flags) {
	int fd = open(pathname, flags);
	trace_printf("A FD %i open (%s)\n", fd, pathname);
	return fd;
}

static int close_trace(int fd) {
	trace_printf("F FD %i close\n", fd);
	return close(fd);
}
