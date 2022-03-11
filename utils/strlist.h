struct strlist {
	size_t capacity;
	size_t size;
	char *data;
};

static void strlist_init(struct strlist *list) {
	list->capacity = 8;
	list->size = 0;
	list->data = malloc_safe(list->capacity * sizeof(char));
	list->data[0] = '\0';
}


static void strlist_destroy(struct strlist *list) {
	free(list->data);
}

static void strlist_assure_capacity(struct strlist *list, size_t add) {
	while (list->capacity <= list->size + add) {
		list->capacity *= 2;
		list->data = realloc_safe(list->data, list->capacity * sizeof(char));
	}
}

static void strlist_resize(struct strlist *list, size_t size) {
	list->size = size;
	strlist_assure_capacity(list, 1);
	list->data[list->size] = '\0';
}

static void strlist_add(struct strlist *list, char c) {
	strlist_assure_capacity(list, 2);
	list->data[list->size++] = c;
	list->data[list->size] = '\0';
}

static void strlist_add_sn(struct strlist *list, const char *s, size_t n) {
	strlist_assure_capacity(list, n + 1);
	memmove(list->data + list->size, s, n);
	list->size += n;
	list->data[list->size] = '\0';
}

static void strlist_add_s(struct strlist *list, const char *s) {
	strlist_add_sn(list, s, strlen(s));
}
