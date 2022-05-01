static SCREEN *newterm_safe(const char *type, FILE *outfd, FILE *infd) {
	SCREEN *ret = newterm(type, outfd, infd);
	if (!ret) {
		fprintf(stderr, "Cannot initialize terminal: %s\n", strerror(errno));
		exit(3);
	}
	trace_printf("A SCREEN %p newterm\n", (void*) ret);
	return ret;
}

static int getch_safe() {
	int ret = getch();
	if (ret == ERR) {
		fprintf(stderr, "Cannot read from keyboard: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static void delscreen_trace(SCREEN *sp) {
	trace_printf("F SCREEN %p delscreen\n", (void*) sp);
	delscreen(sp);
}
