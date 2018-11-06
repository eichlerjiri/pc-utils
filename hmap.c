#include "hmap.h"
#include "common.h"
#include <string.h>

void hmap_init(struct hmap *m, size_t (*hash)(const void*), int (*equals)(const void*, const void*)) {
	m->capacity = 8;
	m->size = 0;
	m->fill_limit = 6;
	m->hash = hash;
	m->equals = equals;

	size_t alloc_size = m->capacity * sizeof(struct hmap_item);
	m->data = c_malloc(alloc_size);
	memset(m->data, 0, alloc_size);
}

void hmap_destroy(struct hmap *m) {
	c_free(m->data);
}

void hmap_remove_do(struct hmap_item *prev, struct hmap_item *item) {
	if (item->next) {
		struct hmap_item *next = item->next;
		item->key = next->key;
		item->value = next->value;
		item->next = next->next;
		c_free(next);
	} else if (prev) {
		prev->next = NULL;
		c_free(item);
	} else {
		item->key = NULL;
		item->value = NULL;
	}
}

void hmap_resize(struct hmap *m) {
	size_t orig_capacity = m->capacity;
	struct hmap_item *orig_data = m->data;

	m->capacity *= 2;
	m->size = 0;
	m->fill_limit *= 2;

	size_t alloc_size = m->capacity * sizeof(struct hmap_item);
	m->data = c_malloc(alloc_size);
	memset(m->data, 0, alloc_size);

	for (int i = 0; i < orig_capacity; i++) {
		struct hmap_item *item = orig_data + i;
		while (item->key) {
			hmap_put(m, item->key, item->value);
			hmap_remove_do(NULL, item);
		}
	}

	c_free(orig_data);
}

void *hmap_get(struct hmap *m, const void *key) {
	struct hmap_item *item = m->data + m->hash(key) % m->capacity;
	while (item && item->key) {
		if (m->equals(key, item->key)) {
			return item->value;
		}
		item = item->next;
	}
	return NULL;
}

void hmap_put(struct hmap *m, void *key, void *value) {
	struct hmap_item *prev = NULL;
	struct hmap_item *item = m->data + m->hash(key) % m->capacity;
	while (item && item->key) {
		if (m->equals(key, item->key)) {
			c_free(item->value);
			item->value = value;
			return;
		}
		prev = item;
		item = item->next;
	}

	if (prev) {
		item = c_malloc(sizeof(struct hmap_item));
		item->next = NULL;
		prev->next = item;
	}

	item->key = key;
	item->value = value;
	m->size++;

	if (m->size > m->fill_limit) {
		hmap_resize(m);
	}
}

void hmap_remove_direct(struct hmap *m, struct hmap_item *prev, struct hmap_item *item) {
	c_free(item->key);
	c_free(item->value);
	hmap_remove_do(prev, item);
	m->size--;
}

void hmap_remove(struct hmap *m, const void *key) {
	struct hmap_item *prev = NULL;
	struct hmap_item *item = m->data + m->hash(key) % m->capacity;
	while (item && item->key) {
		if (m->equals(key, item->key)) {
			hmap_remove_direct(m, prev, item);
			return;
		}
		prev = item;
		item = item->next;
	}
}
