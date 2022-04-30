struct unilist {
	size_t capacity;
	size_t size;
	size_t item_size;
	void *data;
};

static void unilist_init(struct unilist *list, size_t item_size) {
	list->capacity = 0;
	list->size = 0;
	list->item_size = item_size;
	list->data = NULL;
}

static void unilist_destroy(struct unilist *list) {
	free_safe(list->data);
}

static void unilist_assure_capacity(struct unilist *list, size_t add) {
	size_t needed = list->size + add;
	if (list->capacity < needed) {
		if (!list->capacity) {
			list->capacity = 8;
		}
		while (list->capacity < needed) {
			list->capacity *= 2;
		}
		list->data = realloc_safe(list->data, list->capacity * list->item_size);
	}
}

static void unilist_add(struct unilist *list, void *ptr) {
	unilist_assure_capacity(list, 1);
	memmove((char*) list->data + list->item_size * list->size++, ptr, list->item_size);
}
