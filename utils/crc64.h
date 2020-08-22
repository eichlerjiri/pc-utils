static void generate_crc64_table(unsigned long crc64_table[256]) {
	for (unsigned long i = 0; i < 256; i++) {
		unsigned long crc = 0;
		unsigned long c = i << 56;

		for (int j = 0; j < 8; j++) {
			if ((crc ^ c) & 0x8000000000000000ull) {
				crc = (crc << 1) ^ 0x42F0E1EBA9EA3693ull;
			} else {
				crc <<= 1;
			}
			c <<= 1;
		}

		crc64_table[i] = crc;
	}
}

static void crc64(unsigned long crc64_table[256], unsigned long *crc, unsigned char c) {
	int t = ((*crc >> 56) ^ c) & 0xFF;
	*crc = crc64_table[t] ^ (*crc << 8);
}
