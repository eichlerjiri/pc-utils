static void regcomp_e(regex_t *preg, const char *regex, int cflags) {
	int code = regcomp(preg, regex, cflags);
	if (code) {
		char buffer[256];
		regerror(code, preg, buffer, sizeof(buffer));
		fprintf(stderr, "Cannot compile regex %s: %s\n", regex, buffer);
		exit(3);
	}
}
