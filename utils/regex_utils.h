int regexec_safe(const regex_t *compiled, const char *string, size_t nmatch, regmatch_t *matchptr, int eflags) {
	int ret = regexec(compiled, string, nmatch, matchptr, eflags);
	if (ret && ret != REG_NOMATCH) {
		char buffer[4096];
		regerror(ret, compiled, buffer, sizeof(buffer));
		fprintf(stderr, "Regex error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
