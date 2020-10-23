#ifndef TB_STRETCHY_H
#define TB_STRETCHY_H

#include <stdlib.h>

#define stretchy__hdr(b) ((size_t*)(void*)(b) - 2)
#define stretchy__cap(b) stretchy__hdr(b)[0]
#define stretchy__len(b) stretchy__hdr(b)[1]

#define stretchy__grow(b, n) (*((void **)&(b)) = stretchy__realloc((b), stretchy_size(b) + (n), sizeof(*(b))))

#define stretchy__need_space(b, n) ((b) == NULL || stretchy__len(b) + (n) >= stretchy__cap(b))
#define stretchy__make_space(b, n) (stretchy__need_space(b, (n)) ? stretchy__grow(b, n) : 0)

#define stretchy__push_checked(b, v) ((b) ? (b)[stretchy__len(b)++] = (v) : 0)

#define stretchy_push(b, v) (stretchy__make_space(b, 1), stretchy__push_checked(b, v))
#define stretchy_size(b) ((b) ? stretchy__len(b) : 0)
#define stretchy_free(b) ((b) ? free(stretchy__hdr(b)), 0 : 0);
#define stretchy_last(b) ((b)[stretchy__len(b) - 1])

static void* stretchy__realloc(const void* buf, size_t new_len, size_t elem_size)
{
	size_t c = buf ? 2 * stretchy__cap(buf) : 0;
	size_t new_cap = c > new_len ? c : new_len;
	size_t new_size = 2 * sizeof(size_t) + new_cap * elem_size;

	size_t* hdr = realloc(buf ? stretchy__hdr(buf) : NULL, new_size);

	if (!hdr) /* out of memory, try to force a NULL pointer exception later */
		return (void*)(2 * sizeof(size_t));

	hdr[0] = new_cap;
	if (!buf) hdr[1] = 0;

	return hdr + 2;
}

#endif // !TB_STRETCHY_H
