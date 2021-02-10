#ifndef POLY1305_DONNA_H
#define POLY1305_DONNA_H

#include <stddef.h>

#define poly1305_block_size 16

typedef struct poly1305_context
{
	size_t aligner;
	unsigned char opaque[136];
} poly1305_context;

void poly1305_init(void *cc);
void poly1305(void *cc, const unsigned char *m, size_t bytes);
void poly1305_close(void *cc, char out[33]);

#endif /* POLY1305_DONNA_H */
