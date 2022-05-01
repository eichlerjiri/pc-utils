static int iconv_direct(iconv_t *cd, const char *tocode, const char *fromcode, char *in, size_t inlen, struct strlist *out) {
	if (!*cd) {
		*cd = iconv_open(tocode, fromcode);
		if (*cd == (iconv_t) -1) {
			fprintf(stderr, "Error initializing iconv: %s\n", strerror(errno));
			exit(3);
		}
		trace_printf("A ICONV %p iconv_open (%s to %s)\n", *cd, fromcode, tocode);
	}

	size_t done = 0;

	while (1) {
		strlist_assure_capacity(out, out->size + done * 2);

		char *outptr = out->data + out->size + done;
		size_t outlen = out->capacity - out->size - done;
		size_t ret = iconv(*cd, &in, &inlen, &outptr, &outlen);

		done = (size_t) (outptr - out->data) - out->size;

		if (ret != (size_t) -1) {
			break;
		} else if (errno != E2BIG) {
			return -1;
		}
	}

	strlist_resize(out, out->size + done);
	return 0;
}

static void iconv_close_if_opened(iconv_t cd) {
	if (cd) {
		trace_printf("F ICONV %p iconv_close\n", cd);
		iconv_close(cd);
	}
}
