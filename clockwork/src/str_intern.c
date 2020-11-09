#include "str_intern.h"

#include "tb_stretchy.h"

typedef struct
{
	size_t len;
	const char* str;
} InternStr;

static Arena str_arena;
static InternStr* intern_table;

const char* str_intern_range(const char* start, const char* end)
{
	size_t len = end - start;
	for (InternStr* it = intern_table; it != tb_stretchy_last(intern_table); ++it)
	{
		if (it->len == len && strncmp(it->str, start, len) == 0)
			return it->str;
	}

	char* str = arena_alloc(&str_arena, len + 1);
	memcpy(str, start, len);
	str[len] = '\0';
	tb_stretchy_push(intern_table, ((InternStr){ len, str }));
	return str;
}

const char* str_intern(const char* str)
{
	return str_intern_range(str, str + strlen(str));
}

Arena* get_str_arena()
{
	return &str_arena;
}