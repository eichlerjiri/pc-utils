struct alist {
	size_t capacity, size;
	void **data;
};

static void alist_init(struct alist *list) {
	list->capacity = 8;
	list->size = 0;
	list->data = malloc_safe(list->capacity * sizeof(void*));
}

static void alist_destroy(struct alist *list) {
	for (size_t i = 0; i < list->size; i++) {
		free(list->data[i]);
	}
	free(list->data);
}

static void alist_add(struct alist *list, void *ptr) {
	if (list->capacity <= list->size) {
		list->capacity *= 2;
		list->data = realloc_safe(list->data, list->capacity * sizeof(void*));
	}
	list->data[list->size++] = ptr;
}
