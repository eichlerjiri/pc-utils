struct hmap_item {
	void *key;
	void *value;
	struct hmap_item *next;
};

struct hmap {
	size_t capacity;
	size_t size;
	size_t fill_limit;
	size_t (*hash)(const void*);
	int (*equals)(const void*, const void*);
	struct hmap_item **data;
};

static void hmap_init(struct hmap *m, size_t (*hash)(const void*), int (*equals)(const void*, const void*)) {
	m->capacity = 8;
	m->size = 0;
	m->fill_limit = 6;
	m->hash = hash;
	m->equals = equals;

	size_t alloc_size = m->capacity * sizeof(struct hmap_item*);
	m->data = c_malloc(alloc_size);
	memset(m->data, 0, alloc_size);
}

static void hmap_resize(struct hmap *m) {
	size_t orig_capacity = m->capacity;
	struct hmap_item **orig_data = m->data;

	m->capacity *= 2;
	m->fill_limit *= 2;

	size_t alloc_size = m->capacity * sizeof(struct hmap_item*);
	m->data = c_malloc(alloc_size);
	memset(m->data, 0, alloc_size);

	for (int i = 0; i < orig_capacity; i++) {
		struct hmap_item *item = orig_data[i];
		while (item) {
			struct hmap_item *next = item->next;

			struct hmap_item **link = m->data + (m->hash(item->key) & (m->capacity - 1));
			item->next = *link;
			*link = item;

			item = next;
		}
	}

	c_free(orig_data);
}

static void hmap_destroy(struct hmap *m) {
	c_free(m->data);
}

static void *hmap_get(struct hmap *m, const void *key) {
	struct hmap_item **link = m->data + (m->hash(key) & (m->capacity - 1));
	struct hmap_item *item;
	while ((item = *link)) {
		if (m->equals(key, item->key)) {
			return item->value;
		}
		link = &item->next;
	}
	return NULL;
}

static void hmap_put(struct hmap *m, void *key, void *value) {
	struct hmap_item **link = m->data + (m->hash(key) & (m->capacity - 1));
	struct hmap_item *item;
	while ((item = *link)) {
		if (m->equals(key, item->key)) {
			c_free(item->value);
			item->value = value;
			return;
		}
		link = &item->next;
	}

	item = c_malloc(sizeof(struct hmap_item));
	item->next = NULL;
	*link = item;

	item->key = key;
	item->value = value;
	m->size++;

	if (m->size > m->fill_limit) {
		hmap_resize(m);
	}
}

static void hmap_remove_direct(struct hmap *m, struct hmap_item **link) {
	struct hmap_item *item = *link;
	*link = item->next;
	c_free(item->key);
	c_free(item->value);
	c_free(item);
	m->size--;
}

static void hmap_remove(struct hmap *m, const void *key) {
	struct hmap_item **link = m->data + (m->hash(key) & (m->capacity - 1));
	struct hmap_item *item;
	while ((item = *link)) {
		if (m->equals(key, item->key)) {
			hmap_remove_direct(m, link);
			return;
		}
		link = &item->next;
	}
}
