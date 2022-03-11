struct unilist {
	size_t capacity;
	size_t size;
	size_t item_size;
	void *data;
};

static void unilist_init(struct unilist *list, size_t item_size) {
	list->capacity = 8;
	list->size = 0;
	list->item_size = item_size;
	list->data = malloc_safe(list->capacity * list->item_size);
}

static void unilist_destroy(struct unilist *list) {
	free(list->data);
}

static void unilist_assure_capacity(struct unilist *list, size_t add) {
	while (list->capacity <= list->size + add) {
		list->capacity *= 2;
		list->data = realloc_safe(list->data, list->capacity * list->item_size);
	}
}

static void unilist_add(struct unilist *list, void *ptr) {
	unilist_assure_capacity(list, 1);
	memmove((char*) list->data + list->item_size * list->size++, ptr, list->item_size);
}
