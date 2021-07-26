struct hmap_item {
	void *key, *value;
	struct hmap_item *next;
};

struct hmap {
	size_t capacity, size, fill_limit;
	size_t (*hash)(const void*);
	int (*equals)(const void*, const void*);
	struct hmap_item **data;
	void (*free_key)(void*);
	void (*free_value)(void*);
};

static size_t hash_ptr(const void* key) {
	return 31u * (unsigned long) key;
}

static int equals_ptr(const void* key1, const void* key2) {
	return key1 == key2;
}

static size_t hash_str(const void* key) {
	const unsigned char* str = key;
	size_t ret = 1u;
	while (*str) {
		ret = 31u * ret + *str++;
	}
	return ret;
}

static int equals_str(const void* key1, const void* key2) {
	const char* str1 = key1;
	const char* str2 = key2;
	return !strcmp(str1, str2);
}

static void nofree(void *ptr) {
}

static void hmap_init(struct hmap *m, size_t (*hash)(const void*), int (*equals)(const void*, const void*), void (*free_key)(void*), void (*free_value)(void*)) {
	m->capacity = 8;
	m->size = 0;
	m->fill_limit = 6;
	m->hash = hash;
	m->equals = equals;
	m->free_key = free_key;
	m->free_value = free_value;

	m->data = calloc_safe(m->capacity, sizeof(struct hmap_item*));
}

static void hmap_resize(struct hmap *m) {
	size_t orig_capacity = m->capacity;
	struct hmap_item **orig_data = m->data;

	m->capacity *= 2;
	m->fill_limit *= 2;

	m->data = calloc_safe(m->capacity, sizeof(struct hmap_item*));

	for (size_t i = 0; i < orig_capacity; i++) {
		struct hmap_item *item = orig_data[i];
		while (item) {
			struct hmap_item *next = item->next;

			struct hmap_item **link = m->data + (m->hash(item->key) & (m->capacity - 1));
			item->next = *link;
			*link = item;

			item = next;
		}
	}

	free(orig_data);
}

static void hmap_destroy(struct hmap *m) {
	for (size_t i = 0; i < m->capacity; i++) {
		struct hmap_item *item = m->data[i];
		while (item) {
			struct hmap_item *next = item->next;
			m->free_key(item->key);
			m->free_value(item->value);
			free(item);
			item = next;
		}
	}
	free(m->data);
}

static void *hmap_get(struct hmap *m, const void *key) {
	struct hmap_item *item = m->data[m->hash(key) & (m->capacity - 1)];
	while (item) {
		if (m->equals(key, item->key)) {
			return item->value;
		}
		item = item->next;
	}
	return NULL;
}

static void hmap_put(struct hmap *m, void *key, void *value) {
	struct hmap_item **link = m->data + (m->hash(key) & (m->capacity - 1));
	struct hmap_item *item;
	while ((item = *link)) {
		if (m->equals(key, item->key)) {
			m->free_key(item->key);
			m->free_value(item->value);
			item->key = key;
			item->value = value;
			return;
		}
		link = &item->next;
	}

	item = malloc_safe(sizeof(struct hmap_item));
	item->next = NULL;
	*link = item;

	item->key = key;
	item->value = value;
	m->size++;

	if (m->size > m->fill_limit) {
		hmap_resize(m);
	}
}

static void hmap_remove(struct hmap *m, const void *key) {
	struct hmap_item **link = m->data + (m->hash(key) & (m->capacity - 1));
	struct hmap_item *item;
	while ((item = *link)) {
		if (m->equals(key, item->key)) {
			m->free_key(item->key);
			m->free_value(item->value);
			*link = item->next;
			free(item);
			m->size--;
			return;
		}
		link = &item->next;
	}
}
