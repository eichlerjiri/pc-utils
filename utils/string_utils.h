static char *memdup_safe(void *ptr, size_t cnt) {
	void *ret = malloc_safe(cnt);
	memcpy(ret, ptr, cnt);
	return ret;
}
