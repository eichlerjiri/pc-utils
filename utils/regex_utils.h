int regexec_safe(const regex_t *compiled, const char *string, size_t nmatch, regmatch_t *matchptr, int eflags) {
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
