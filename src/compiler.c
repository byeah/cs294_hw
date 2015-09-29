#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "utils.h"
#include "ast.h"
#include "compiler.h"
#include "bytecode.h"

Program* compile (ScopeStmt* stmt) {
  printf("Compiling Program:\n");
  print_scopestmt(stmt);
  printf("\n");
  exit(-1);
}

