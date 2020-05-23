static char *strdup_safe(const char *s) {
	char *ret = strdup(s);
	if (!ret) {
		fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int valid_line(const char *s, size_t length) {
	int i = 0;
	while (i < length && s[i] && s[i] != '\r' && s[i] != '\n') {
		i++;
	}
	while (i < length && (s[i] == '\r' || s[i] == '\n')) {
		i++;
	}
	return i == length;
}

static void remove_newline(char *s) {
	while (*s) {
		if (*s == '\n' || *s == '\r') {
			*s = '\0';
			break;
		}
		s++;
	}
}

static int ends_with(const char *haystack, const char *needle) {
	size_t haystack_len = strlen(haystack);
	size_t needle_len = strlen(needle);
	return haystack_len >= needle_len && !strcmp(haystack + (haystack_len - needle_len), needle);
}
