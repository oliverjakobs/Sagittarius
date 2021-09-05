#ifndef CLOCKWORK_STATEMENT_H
#define CLOCKWORK_STATEMENT_H

#include "common.h"

int cw_parse_expression(cwRuntime* cw);
int cw_parse_statement(cwRuntime* cw);
int cw_parse_declaration(cwRuntime* cw);

#endif /* !CLOCKWORK_STATEMENT_H */