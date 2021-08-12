#include "memory.h"

#include "vm.h"

void* cw_reallocate(void* block, size_t old_size, size_t new_size)
{
    if (new_size == 0)
    {
        free(block);
        return NULL;
    }

    void* result = realloc(block, new_size);
    if (result == NULL) exit(1);
    return result;
}

static Object* cw_object_alloc(VM* vm, size_t size, ObjectType type)
{
    Object* object = cw_reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm->objects;
    vm->objects = object;
    return object;
}

static void cw_object_free(Object* object)
{
    switch (object->type)
    {
    case OBJ_STRING:
    {
      cwString* str = (cwString*)object;
      CW_FREE_ARRAY(char, str->raw, str->len + 1);
      cw_reallocate(object, sizeof(cwString), 0);
      break;
    }
    }
}

cwString* cw_str_alloc(VM* vm, char* src, size_t len)
{
    cwString* str = (cwString*)cw_object_alloc(vm, sizeof(cwString), OBJ_STRING);
    str->len = len;
    str->raw = src;
    return str;
}

void cw_free_objects(VM* vm)
{
    Object* object = vm->objects;
    while (object != NULL)
    {
        Object* next = object->next;
        cw_object_free(object);
        object = next;
    }
}