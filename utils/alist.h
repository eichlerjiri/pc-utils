struct alist {
	size_t capacity, size, item_size;
	union {
		void *data;
		void **ptrdata;
		char **cptrdata;
		char *cdata;
	};
};

static void alist_init(struct alist *list, size_t item_size) {
	list->capacity = 8;
	list->size = 0;
	list->item_size = item_size;
	list->data = malloc_safe(list->capacity * list->item_size);
}

static void alist_init_ptr(struct alist *list) {
	alist_init(list, sizeof(void*));
}

static void alist_init_c(struct alist *list) {
	alist_init(list, sizeof(char));
	list->cdata[0] = '\0';
}

static void alist_destroy(struct alist *list) {
	free(list->data);
}

static void alist_destroy_ptr(struct alist *list) {
	for (size_t i = 0; i < list->size; i++) {
		free(list->ptrdata[i]);
	}
	alist_destroy(list);
}

static void alist_destroy_c(struct alist *list) {
	alist_destroy(list);
}

static void alist_assure_capacity(struct alist *list, size_t add) {
	while (list->capacity <= list->size + add) {
		list->capacity *= 2;
		list->data = realloc_safe(list->data, list->capacity * list->item_size);
	}
}

static void* alist_add(struct alist *list) {
	alist_assure_capacity(list, 1);
	return list->cdata + list->size++;
}

static void alist_add_ptr(struct alist *list, void *ptr) {
	alist_assure_capacity(list, 1);
	list->ptrdata[list->size++] = ptr;
}

static void alist_add_c(struct alist *list, char c) {
	alist_assure_capacity(list, 2);
	list->cdata[list->size++] = c;
	list->cdata[list->size] = '\0';
}

static void alist_add_sn(struct alist *list, const char *s, size_t n) {
	alist_assure_capacity(list, n + 1);
	memcpy(list->cdata + list->size, s, n);
	list->size += n;
	list->cdata[list->size] = '\0';
}

static void alist_add_s(struct alist *list, const char *s) {
	alist_add_sn(list, s, strlen(s));
}

static void alist_rem(struct alist *list, size_t n) {
	list->size -= n;
}

static void alist_rem_c(struct alist *list, size_t n) {
	alist_rem(list, n);
	list->cdata[list->size] = '\0';
}
