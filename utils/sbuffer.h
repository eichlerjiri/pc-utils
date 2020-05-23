struct sbuffer {
	size_t capacity, size;
	char *data;
};

static void sbuffer_init(struct sbuffer *buf) {
	buf->capacity = 8;
	buf->size = 0;
	buf->data = malloc_safe(buf->capacity);
}

static void sbuffer_destroy(struct sbuffer *buf) {
	free(buf->data);
}

static void sbuffer_assure_capacity(struct sbuffer *buf, size_t add) {
	while (buf->capacity < buf->size + add) {
		buf->capacity *= 2;
		buf->data = realloc_safe(buf->data, buf->capacity);
	}
}

static void sbuffer_add_c(struct sbuffer *buf, char c) {
	sbuffer_assure_capacity(buf, 2);
	buf->data[buf->size++] = c;
	buf->data[buf->size] = '\0';
}

static void sbuffer_add_s(struct sbuffer *buf, const char *s) {
	sbuffer_assure_capacity(buf, strlen(s) + 1);
	char c;
	while ((c = *s++)) {
		buf->data[buf->size++] = c;
	}
	buf->data[buf->size] = '\0';
}

static void sbuffer_add_sn(struct sbuffer *buf, const char *s, size_t n) {
	sbuffer_assure_capacity(buf, n + 1);
	char c;
	while (n-- && (c = *s++)) {
		buf->data[buf->size++] = c;
	}
	buf->data[buf->size] = '\0';
}

static void sbuffer_rem(struct sbuffer *buf, size_t n) {
	if (n < buf->size) {
		buf->size -= n;
	} else {
		buf->size = 0;
	}
	buf->data[buf->size] = '\0';
}

static void sbuffer_clear(struct sbuffer *buf) {
	buf->size = 0;
	buf->data[buf->size] = '\0';
}

static char sbuffer_last_c(struct sbuffer *buf) {
	if (buf->size) {
		return buf->data[buf->size - 1];
	} else {
		return '\0';
	}
}
