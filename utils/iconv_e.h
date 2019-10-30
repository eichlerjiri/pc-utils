static iconv_t iconv_open_e(const char *tocode, const char *fromcode) {
	iconv_t ret = iconv_open(tocode, fromcode);
	if (ret == (iconv_t) -1) {
		fprintf(stderr, "Error iconv_open: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
