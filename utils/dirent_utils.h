static DIR *opendir_trace(const char *name) {
	DIR *ret = opendir(name);
	if (ret) {
		trace_printf("A DIR %p opendir (%s)\n", (void*) ret, name);
	}
	return ret;
}

static int closedir_trace(DIR *dirp) {
	trace_printf("F DIR %p closedir\n", (void*) dirp);
	return closedir(dirp);
}
