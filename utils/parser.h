static int parse_word(char **str, struct strlist *res) {
	char *s = *str;
	while (*s && isspace(*s)) {
		s++;
	}

	int err = 1;
	while (*s && !isspace(*s)) {
		strlist_add(res, *s++);
		err = 0;
	}
	if (err) {
		return 1;
	}

	*str = s;
	return 0;
}

static int parse_long(char **str, long *res) {
	char *end = NULL;
	errno = 0;

	long ret = strtol(*str, &end, 10);
	if (errno || *str == end) {
		return 1;
	}

	*res = ret;
	*str = end;
	return 0;
}

static int parse_unsigned_long_strict(char **str, unsigned long *res) {
	char *end = NULL;
	errno = 0;

	unsigned long ret = strtoul(*str, &end, 10);
	if (errno || *str == end) {
		return 1;
	}

	char *it = *str;
	while (it < end) {
		if (!isdigit(*it++)) {
			return 1;
		}
	}

	*res = ret;
	*str = end;
	return 0;
}

static int parse_float(char **str, float *res) {
	char *end = NULL;
	errno = 0;

	float ret = strtof(*str, &end);
	if (errno || *str == end) {
		return 1;
	}

	*res = ret;
	*str = end;
	return 0;
}

static int parse_char(char **str, char *res) {
	char *s = *str;
	if (!*s) {
		return 1;
	}

	*res = *s++;
	*str = s;
	return 0;
}

static int parse_char_match(char **str, char match) {
	char *s = *str;
	if (*s++ != match) {
		return 1;
	}

	*str = s;
	return 0;
}

static int parse_string_match(char **str, const char *match) {
	char *s = *str;
	while (*match) {
		if (!*s || *s++ != *match++) {
			return 1;
		}
	}

	*str = s;
	return 0;
}

static int parse_line_end(char *str, size_t linelen, char *res) {
	if (strlen(str) != linelen) {
		return 1;
	}
	while (*str && *str != '\r' && *str != '\n') {
		str++;
	}
	if (*str && strcmp(str, "\r\n") && strcmp(str, "\n") && strcmp(str, "\r")) {
		return 1;
	}

	if (res) {
		strcpy(res, str);
	}
	*str = '\0';
	return 0;
}
