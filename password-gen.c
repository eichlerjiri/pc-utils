#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "utils/stdio_utils.h"

static int gen_password(const char *filename, FILE *input, int length) {
	const char *chars = "abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ123456789";
	int num_chars = (int) strlen(chars);

	char password[256];
	for (int i = 0; i < length; i++) {
		int c = fgetc(input);
		if (c < 0) {
			fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
			return 2;
		}
		password[i] = chars[c % num_chars];
	}
	password[length] = '\0';

	printf_safe("%s\n", password);

	return 0;
}

static int gen_passwords(const char *filename, FILE *input) {
	for (int i = 0; i < 4; i++) {
		if (gen_password(filename, input, 8)) {
			return 2;
		}
	}
	for (int i = 0; i < 4; i++) {
		if (gen_password(filename, input, 16)) {
			return 2;
		}
	}
	return 0;
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (*argv) {
		printf_safe("Usage: %s\n", pname);
		return 1;
	}

	const char *filename = "/dev/urandom";

	FILE *input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return 2;
	}

	int ret = gen_passwords(filename, input);

	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}

	return ret;
}

int main(int argc, char **argv) {
	int ret = run_program(argv);
	flush_safe(stdout);
	return ret;
}
