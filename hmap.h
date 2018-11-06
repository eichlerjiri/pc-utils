#include <stdlib.h>

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
	struct hmap_item *data;
};

void hmap_init(struct hmap *m, size_t (*hash)(const void*), int (*equals)(const void*, const void*));
void hmap_destroy(struct hmap *m);
void *hmap_get(struct hmap *m, const void *key);
void hmap_put(struct hmap *m, void *key, void *value);
void hmap_remove_direct(struct hmap *m, struct hmap_item *prev, struct hmap_item *item);
void hmap_remove(struct hmap *m, const void *key);
