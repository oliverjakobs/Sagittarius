#ifndef TB_STRETCHY_H
#define TB_STRETCHY_H

#include <stdlib.h>

#define TB_STRETCHY_HDR_ELEM	size_t
#define TB_STRETCHY_HDR_SIZE	2 * sizeof(TB_STRETCHY_HDR_ELEM)

#define tb_stretchy__hdr(b) ((TB_STRETCHY_HDR_ELEM*)(void*)(b) - 2)
#define tb_stretchy__cap(b) tb_stretchy__hdr(b)[0]
#define tb_stretchy__len(b) tb_stretchy__hdr(b)[1]

#define tb_stretchy__max(a, b) ((a) >= (b) ? (a) : (b))

#define tb_stretchy_push(b, v) (tb_stretchy__grow(&(b), sizeof(*(b))) ? (b)[tb_stretchy__len(b)++] = (v), 1 : 0)
#define tb_stretchy_size(b) ((b) ? tb_stretchy__len(b) : 0)
#define tb_stretchy_free(b) ((b) ? (free(tb_stretchy__hdr(b)), (b) = NULL) : 0);
#define tb_stretchy_last(b) ((b) ? (b) + tb_stretchy__len(b) : NULL)

static int tb_stretchy__grow(const void** buf, size_t elem_size)
{
	if (*buf && tb_stretchy__len(*buf) + 1 < tb_stretchy__cap(*buf))
		return 1;

	size_t new_len = tb_stretchy_size(*buf) + 1;
	size_t new_cap = tb_stretchy__max((*buf ? 2 * tb_stretchy__cap(*buf) : 1), new_len);
	size_t new_size = TB_STRETCHY_HDR_SIZE + new_cap * elem_size;

	TB_STRETCHY_HDR_ELEM* hdr = realloc(*buf ? tb_stretchy__hdr(*buf) : NULL, new_size);

	if (!hdr) return 0; /* out of memory */

	hdr[0] = new_cap;
	if (!*buf) hdr[1] = 0;

	*buf = hdr + 2;

	return 1;
}

#endif // !TB_STRETCHY_H
