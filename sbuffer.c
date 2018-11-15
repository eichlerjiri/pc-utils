struct sbuffer {
	size_t capacity;
	size_t size;
	char *data;
};

void sbuffer_init(struct sbuffer *buf) {
	buf->capacity = 8;
	buf->size = 0;
	buf->data = c_malloc(buf->capacity);
}

void sbuffer_destroy(struct sbuffer *buf) {
	c_free(buf->data);
}

void sbuffer_add(struct sbuffer *buf, const char *str) {
	size_t to_add = strlen(str);
	while (buf->capacity <= buf->size + to_add) {
		buf->capacity *= 2;
		buf->data = c_realloc(buf->data, buf->capacity);
	}

	char c;
	while ((c = *str++)) {
		buf->data[buf->size++] = c;
	}
	buf->data[buf->size] = '\0';
}
