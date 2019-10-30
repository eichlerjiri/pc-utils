static char *strdup_e(const char *s) {
	char *ret = strdup(s);
	if (!ret) {
		fprintf(stderr, "Error strdup: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
