struct ptrlist {
	size_t capacity;
	size_t size;
	void **data;
};

static void ptrlist_init(struct ptrlist *list) {
	list->capacity = 0;
	list->size = 0;
	list->data = NULL;
}

static void ptrlist_destroy(struct ptrlist *list) {
	for (size_t i = 0; i < list->size; i++) {
		free_trace(list->data[i]);
	}

	free_trace(list->data);
}

static void ptrlist_assure_capacity(struct ptrlist *list, size_t add) {
	size_t needed = list->size + add;
	if (list->capacity < needed) {
		if (!list->capacity) {
			list->capacity = 8;
		}
		while (list->capacity < needed) {
			list->capacity *= 2;
		}
		list->data = realloc_safe(list->data, list->capacity * sizeof(void*));
	}
}

static void ptrlist_add(struct ptrlist *list, void *ptr) {
	ptrlist_assure_capacity(list, 1);
	list->data[list->size++] = ptr;
}

static void ptrlist_resize(struct ptrlist *list, size_t size) {
	list->size = size;
}
