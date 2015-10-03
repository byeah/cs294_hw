#ifndef COMPILER_H
#define COMPILER_H
#include "ast.h"
#include "bytecode.h"

Program* compile (ScopeStmt* stmt);

#endif
