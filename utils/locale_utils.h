char *setlocale_safe(int category, const char *locale) {
	char *ret = setlocale(category, locale);
	if (!ret) {
		fprintf(stderr, "Error setlocale\n");
		exit(3);
	}
	return ret;
}
