static FILE *ftrace;

static void trace_print(char op, const char *type, void *id, const char *comment) {
	if (!ftrace) {
		char filename[100];
		sprintf(filename, "trace.%i.txt", getpid());
		ftrace = fopen(filename, "w");
		if (!ftrace) {
			return;
		}
	}
	fprintf(ftrace, "%c %s %p %s\n", op, type, id, comment);
	fflush(ftrace);
}

static void trace_start(const char *type, void *id, const char *comment) {
	trace_print('A', type, id, comment);
}

static void trace_end(const char *type, void* id, const char *comment) {
	trace_print('F', type, id, comment);
}
