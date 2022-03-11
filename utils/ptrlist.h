struct ptrlist {
	size_t capacity;
	size_t size;
	void **data;
};

static void ptrlist_init(struct ptrlist *list) {
	list->capacity = 8;
	list->size = 0;
	list->data = malloc_safe(list->capacity * sizeof(void*));
}

static void ptrlist_destroy(struct ptrlist *list) {
	for (size_t i = 0; i < list->size; i++) {
		free(list->data[i]);
	}
	free(list->data);
}

static void ptrlist_assure_capacity(struct ptrlist *list, size_t add) {
	while (list->capacity <= list->size + add) {
		list->capacity *= 2;
		list->data = realloc_safe(list->data, list->capacity * sizeof(void*));
	}
}

static void ptrlist_resize(struct ptrlist *list, size_t size) {
	list->size = size;
	ptrlist_assure_capacity(list, 0);
}

static void ptrlist_add(struct ptrlist *list, void *ptr) {
	ptrlist_assure_capacity(list, 1);
	list->data[list->size++] = ptr;
}
