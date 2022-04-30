struct strlist {
	size_t capacity;
	size_t size;
	char *data;
};

static void strlist_init(struct strlist *list) {
	list->capacity = 0;
	list->size = 0;
	list->data = (char*) "";
}

static void strlist_destroy(struct strlist *list) {
	if (list->capacity) {
		free_safe(list->data);
	}
}

static void strlist_assure_capacity(struct strlist *list, size_t add) {
	size_t needed = list->size + add + 1;
	if (list->capacity < needed) {
		if (!list->capacity) {
			list->capacity = 8;
			list->data = NULL;
		}
		while (list->capacity < needed) {
			list->capacity *= 2;
		}
		list->data = realloc_safe(list->data, list->capacity * sizeof(char));
	}
}

static void strlist_add(struct strlist *list, char c) {
	strlist_assure_capacity(list, 1);
	list->data[list->size++] = c;
	list->data[list->size] = '\0';
}

static void strlist_add_sn(struct strlist *list, const char *s, size_t n) {
	strlist_assure_capacity(list, n);
	memmove(list->data + list->size, s, n);
	list->size += n;
	list->data[list->size] = '\0';
}

static int strlist_readline(struct strlist *list, FILE *file) {
	int ret = -1;
	while (1) {
		int c = getc(file);
		if (c == EOF) {
			return ret;
		}

		ret = 0;
		strlist_add(list, (char) c);

		if (c == '\n') {
			return ret;
		}
	}
}

static void strlist_resize(struct strlist *list, size_t size) {
	if (list->size != size) {
		list->size = size;
		list->data[list->size] = '\0';
	}
}
