static int regexec_safe(const regex_t *compiled, const char *string, size_t nmatch, regmatch_t *matchptr, int eflags) {
	int ret = regexec(compiled, string, nmatch, matchptr, eflags);
	if (ret && ret != REG_NOMATCH) {
		size_t bufsize = regerror(ret, compiled, NULL, 0);
		char *buf = malloc_safe(bufsize);
		regerror(ret, compiled, buf, bufsize);
		fprintf(stderr, "Regex error: %s\n", buf);
		exit(3);
	}
	return ret;
}

static int regcomp_trace(regex_t *preg, const char *regex, int cflags) {
	int ret = regcomp(preg, regex, cflags);
	if (!ret) {
		trace_printf("A REGEX %p regcomp (%s)\n", (void*) preg, regex);
	}
	return ret;
}

static void regfree_trace(regex_t *preg) {
	trace_printf("F REGEX %p regfree\n", (void*) preg);
	regfree(preg);
}
