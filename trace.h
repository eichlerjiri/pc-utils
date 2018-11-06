#ifndef TRACE_H
#define TRACE_H

void trace_start(const char *type, void *id, const char *comment);
void trace_end(const char *type, void *id, const char *comment);

#endif
