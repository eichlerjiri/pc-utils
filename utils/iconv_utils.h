static size_t iconv_direct(iconv_t *cd, const char *tocode, const char *fromcode,
		char *in, size_t inlen, char **out, size_t *outsize) {
	if (!*cd) {
		*cd = iconv_open(tocode, fromcode);
		if (*cd == (iconv_t) -1) {
			fprintf(stderr, "Error initializing iconv: %s\n", strerror(errno));
			exit(3);
		}
	}

	if (*outsize < inlen * 4) {
		*outsize = inlen * 8;
		*out = realloc_safe(*out, *outsize);
	}

	char *outbuf = *out;
	size_t outlen = *outsize;
	size_t ret = iconv(*cd, &in, &inlen, &outbuf, &outlen);
	if (ret == (size_t) -1) {
		return (size_t) -1;
	} else {
		return (size_t) (outbuf - *out);
	}
}

static void iconv_close_if_opened(iconv_t cd) {
	if (cd) {
		iconv_close(cd);
	}
}
