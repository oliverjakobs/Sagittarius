#include "resolve.h"

static Type* type_alloc(TypeID id, size_t size)
{
    Type* type = xcalloc(1, sizeof(Type));
    type->id = id;
    type->size = size;
    return type;
}

static const size_t PTR_SIZE = 8;

static Type type_int_val = { TYPE_INT };
static Type type_float_val = { TYPE_FLOAT };

Type* type_int() { return &type_int_val; }
Type* type_float() { return &type_float_val; }

typedef struct
{
    Type* elem;
    Type* ptr;
} CachedPointerType;

static CachedPointerType* cached_ptr_types;

Type* type_pointer(Type* elem)
{
    for (CachedPointerType* it = cached_ptr_types; it != tb_stretchy_last(cached_ptr_types); it++)
    {
        if (it->elem == elem)
            return it->ptr;
    }

    Type* type = type_alloc(TYPE_POINTER, PTR_SIZE);
    type->ptr.elem = elem;
    tb_stretchy_push(cached_ptr_types, ((CachedPointerType){elem, type}));
    return type;
}

typedef struct
{
    Type* elem;
    size_t size;
    Type* array;
} CachedArrayType;

static CachedArrayType* cached_array_types;

Type* type_array(Type* elem, size_t size)
{
    for (CachedArrayType* it = cached_array_types; it != tb_stretchy_last(cached_array_types); it++)
    {
        if (it->elem == elem && it->size == size)
            return it->array;
    }

    complete_type(elem);
    Type* type = type_alloc(TYPE_ARRAY, size * elem->size);
    type->array.elem = elem;
    type->array.size = size;
    tb_stretchy_push(cached_array_types, ((CachedArrayType){elem, size, type}));
    return type;
}

typedef struct
{
    Type** params;
    size_t num_params;
    Type* ret;
    Type* func;
} CachedFuncType;

static CachedFuncType* cached_func_types;

Type* type_func(Type** params, size_t num_params, Type* ret)
{
    for (CachedFuncType* it = cached_func_types; it != tb_stretchy_last(cached_func_types); it++)
    {
        if (it->num_params == num_params && it->ret == ret)
        {
            bool match = true;
            for (size_t i = 0; i < num_params; ++i)
            {
                if (it->params[i] != params[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
                return it->func;
        }
    }
    Type* type = type_alloc(TYPE_FUNC, PTR_SIZE);
    type->func.params = memdup(params, num_params * sizeof(*params));
    type->func.num_params = num_params;
    type->func.ret = ret;
    tb_stretchy_push(cached_func_types, ((CachedFuncType){params, num_params, ret, type}));
    return type;
}

void type_complete_struct(Type* type, TypeField* fields, size_t num_fields)
{
    assert(type->id == TYPE_COMPLETING);
    type->id = TYPE_STRUCT;
    type->size = 0;
    for (TypeField* it = fields; it != fields + num_fields; it++)
    {
        // TODO: Alignment, etc.
        type->size += it->type->size;
    }
    type->aggregate.fields = memdup(fields, num_fields * sizeof(*fields));
    type->aggregate.num_fields = num_fields;
}

void type_complete_union(Type* type, TypeField* fields, size_t num_fields)
{
    assert(type->id == TYPE_COMPLETING);
    type->id = TYPE_UNION;
    type->size = 0;
    for (TypeField* it = fields; it != fields + num_fields; it++)
        type->size = MAX(type->size, it->type->size);

    type->aggregate.fields = memdup(fields, num_fields * sizeof(*fields));
    type->aggregate.num_fields = num_fields;
}

Type* type_incomplete(Entity* entity)
{
    Type* type = type_alloc(TYPE_INCOMPLETE, 0);
    type->entity = entity;
    return type;
}

Entity** entities;
Entity** get_entities() { return entities; }

Entity* entity_new(EntityKind kind, const char* name, Decl* decl)
{
    Entity* entity = xcalloc(1, sizeof(Entity));
    entity->kind = kind;
    entity->name = name;
    entity->decl = decl;
    return entity;
}

Entity* entity_decl(Decl* decl)
{
    EntityKind kind = ENTITY_NONE;
    switch (decl->type)
    {
    case DECL_STRUCT:
    case DECL_UNION:
    case DECL_TYPEDEF:
    case DECL_ENUM:
        kind = ENTITY_TYPE;
        break;
    case DECL_VAR:
        kind = ENTITY_VAR;
        break;
    case DECL_CONST:
        kind = ENTITY_CONST;
        break;
    case DECL_FUNC:
        kind = ENTITY_FUNC;
        break;
    default:
        assert(0);
        break;
    }

    Entity* entity = entity_new(kind, decl->name, decl);
    if (decl->type == DECL_STRUCT || decl->type == DECL_UNION)
    {
        entity->state = ENTITY_RESOLVED;
        entity->type = type_incomplete(entity);
    }
    return entity;
}

Entity* entity_enum_const(const char* name, Decl* decl)
{
    return entity_new(ENTITY_ENUM_CONST, name, decl);
}

Entity* entity_get(const char* name)
{
    for (Entity** it = entities; it != tb_stretchy_last(entities); it++)
    {
        if ((*it)->name == name)
            return *it;
    }
    return NULL;
}

Entity* entity_install_decl(Decl* decl)
{
    Entity* entity = entity_decl(decl);
    tb_stretchy_push(entities, entity);
    if (decl->type == DECL_ENUM)
    {
        for (size_t i = 0; i < decl->enum_decl.num_items; i++)
            tb_stretchy_push(entities, entity_enum_const(decl->enum_decl.items[i].name, decl));
    }
    return entity;
}

Entity* entity_install_type(const char* name, Type* type)
{
    Entity* entity = entity_new(ENTITY_TYPE, name, NULL);
    entity->state = ENTITY_RESOLVED;
    entity->type = type;
    tb_stretchy_push(entities, entity);
    return entity;
}

typedef struct ResolvedExpr
{
    Type* type;
    bool is_lvalue;
    bool is_const;
    int64_t val;
} ResolvedExpr;

ResolvedExpr resolved_null;

ResolvedExpr resolved_rvalue(Type* type)
{
    return (ResolvedExpr) {
        .type = type
    };
}

ResolvedExpr resolved_lvalue(Type* type)
{
    return (ResolvedExpr) {
        .type = type,
        .is_lvalue = true
    };
}

ResolvedExpr resolved_const(int64_t val)
{
    return (ResolvedExpr) {
        .type = type_int(),
        .is_const = true,
        .val = val
    };
}

Entity** ordered_entities;
Entity** get_ordered_entities() { return ordered_entities; }

Type* resolve_typespec(Typespec* typespec);
Entity* resolve_name(const char* name);
int64_t resolve_int_const_expr(Expr* expr);
ResolvedExpr resolve_expr(Expr* expr);

void complete_type(Type* type)
{
    if (type->id == TYPE_COMPLETING)
    {
        fatal("Type completion cycle");
        return;
    }
    else if (type->id != TYPE_INCOMPLETE)
    {
        return;
    }
    type->id = TYPE_COMPLETING;
    Decl* decl = type->entity->decl;
    assert(decl->type == DECL_STRUCT || decl->type == DECL_UNION);
    TypeField* fields = NULL;
    for (size_t i = 0; i < decl->aggregate_decl.num_items; i++)
    {
        AggregateItem item = decl->aggregate_decl.items[i];
        Type* item_type = resolve_typespec(item.type);
        complete_type(item_type);
        for (size_t j = 0; j < item.num_names; j++)
        {
            tb_stretchy_push(fields, ((TypeField) { item.names[j], item_type }));
        }
    }
    if (decl->type == DECL_STRUCT)
    {
        type_complete_struct(type, fields, tb_stretchy_size(fields));
    }
    else
    {
        assert(decl->type == DECL_UNION);
        type_complete_union(type, fields, tb_stretchy_size(fields));
    }
    tb_stretchy_push(ordered_entities, type->entity);
}

Type* resolve_typespec(Typespec* typespec)
{
    switch (typespec->type)
    {
    case TYPESPEC_NAME:
    {
        Entity* entity = resolve_name(typespec->name);
        if (entity->kind != ENTITY_TYPE)
        {
            fatal("%s must denote a type", typespec->name);
            return NULL;
        }
        return entity->type;
    }
    case TYPESPEC_POINTER:
        return type_pointer(resolve_typespec(typespec->ptr.elem));
    case TYPESPEC_ARRAY:
        return type_array(resolve_typespec(typespec->array.elem), resolve_int_const_expr(typespec->array.size));
    case TYPESPEC_FUNC:
    {
        Type** args = NULL;
        for (size_t i = 0; i < typespec->func.num_args; i++)
            tb_stretchy_push(args, resolve_typespec(typespec->func.args[i]));

        return type_func(args, tb_stretchy_size(args), resolve_typespec(typespec->func.ret));
    }
    default:
        assert(0);
        return NULL;
    }
}

Type* resolve_decl_type(Decl* decl)
{
    assert(decl->type == DECL_TYPEDEF);
    return resolve_typespec(decl->typedef_decl.type);
}

Type* resolve_decl_var(Decl* decl)
{
    assert(decl->type == DECL_VAR);
    Type* type = NULL;
    if (decl->var_decl.type)
        type = resolve_typespec(decl->var_decl.type);

    if (decl->var_decl.expr)
    {
        ResolvedExpr result = resolve_expr(decl->var_decl.expr);
        if (type && result.type != type)
            fatal("Declared var type does not match inferred type");

        type = result.type;
    }
    complete_type(type);
    return type;
}

Type* resolve_decl_const(Decl* decl, int64_t* val)
{
    assert(decl->type == DECL_CONST);
    ResolvedExpr result = resolve_expr(decl->const_decl.expr);
    *val = result.val;
    return result.type;
}

void resolve_entity(Entity* entity)
{
    if (entity->state == ENTITY_RESOLVED)
        return;

    if (entity->state == ENTITY_RESOLVING)
    {
        fatal("Cyclic dependency");
        return;
    }

    assert(entity->state == ENTITY_UNRESOLVED);
    entity->state = ENTITY_RESOLVING;

    switch (entity->kind)
    {
    case ENTITY_TYPE:
        entity->type = resolve_decl_type(entity->decl);
        break;
    case ENTITY_VAR:
        entity->type = resolve_decl_var(entity->decl);
        break;
    case ENTITY_CONST:
        entity->type = resolve_decl_const(entity->decl, &entity->val);
        break;
    default:
        assert(0);
        break;
    }
    entity->state = ENTITY_RESOLVED;
    tb_stretchy_push(ordered_entities, entity);
}

void complete_entity(Entity* entity)
{
    resolve_entity(entity);
    if (entity->kind == ENTITY_TYPE)
        complete_type(entity->type);
}

Entity* resolve_name(const char* name)
{
    Entity* entity = entity_get(name);
    if (!entity)
    {
        fatal("Non-existent name");
        return NULL;
    }
    resolve_entity(entity);
    return entity;
}

ResolvedExpr resolve_expr_field(Expr* expr)
{
    assert(expr->type == EXPR_FIELD);
    ResolvedExpr left = resolve_expr(expr->field.expr);
    Type* type = left.type;
    complete_type(type);
    if (type->id != TYPE_STRUCT && type->id != TYPE_UNION)
    {
        fatal("Can only access fields on aggregate types");
        return resolved_null;
    }
    for (size_t i = 0; i < type->aggregate.num_fields; i++)
    {
        TypeField field = type->aggregate.fields[i];
        if (field.name == expr->field.name)
            return left.is_lvalue ? resolved_lvalue(field.type) : resolved_rvalue(field.type);
    }
    fatal("No field named '%s'", expr->field.name);
    return resolved_null;
}

ResolvedExpr resolve_expr_name(Expr* expr)
{
    assert(expr->type == EXPR_NAME);
    Entity* entity = resolve_name(expr->name);
    if (entity->kind == ENTITY_VAR)
        return resolved_lvalue(entity->type);
    else if (entity->kind == ENTITY_CONST)
        return resolved_const(entity->val);

    fatal("%s must be a var or const", expr->name);
    return resolved_null;
}

ResolvedExpr resolve_expr_unary(Expr* expr)
{
    assert(expr->type == EXPR_UNARY);
    ResolvedExpr operand = resolve_expr(expr->unary.expr);
    Type* type = operand.type;
    switch (expr->unary.op)
    {
    case TOKEN_ASTERISK:
        if (type->id != TYPE_POINTER)
            fatal("Cannot deref non-ptr type");

        return resolved_lvalue(type->ptr.elem);
    case TOKEN_AMPERSAND:
        if (!operand.is_lvalue)
            fatal("Cannot take address of non-lvalue");

        return resolved_rvalue(type_pointer(type));
    default:
        assert(0);
        return resolved_null;
    }
}

ResolvedExpr resolve_expr_binary(Expr* expr)
{
    assert(expr->type == EXPR_BINARY);
    assert(expr->binary.op == TOKEN_PLUS);
    ResolvedExpr left = resolve_expr(expr->binary.left);
    ResolvedExpr right = resolve_expr(expr->binary.right);

    if (left.type != type_int())
        fatal("left operand of + must be int");

    if (right.type != left.type)
        fatal("left and right operand of + must have same type");

    if (left.is_const && right.is_const)
        return resolved_const(left.val + right.val);
    else
        return resolved_rvalue(left.type);
}

ResolvedExpr resolve_expr(Expr* expr) {
    switch (expr->type)
    {
    case EXPR_INT:
        return resolved_const(expr->ival);
    case EXPR_NAME:
        return resolve_expr_name(expr);
    case EXPR_FIELD:
        return resolve_expr_field(expr);
    case EXPR_UNARY:
        return resolve_expr_unary(expr);
    case EXPR_BINARY:
        return resolve_expr_binary(expr);
    case EXPR_SIZEOF_EXPR:
    {
        ResolvedExpr result = resolve_expr(expr->sizeof_expr);
        Type* type = result.type;
        complete_type(type);
        return resolved_const(type->size);
    }
    case EXPR_SIZEOF_TYPE:
    {
        Type* type = resolve_typespec(expr->sizeof_type);
        complete_type(type);
        return resolved_const(type->size);
    }
    default:
        assert(0);
        return resolved_null;
    }
}

int64_t resolve_int_const_expr(Expr* expr)
{
    ResolvedExpr result = resolve_expr(expr);
    if (!result.is_const)
        fatal("Expected constant expression");

    return result.val;
}