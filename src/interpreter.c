#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "utils.h"
#include "ast.h"
#include "interpreter.h"

void interpret (ScopeStmt* stmt) {
  printf("Interpret program:\n");
  print_scopestmt(stmt);
  printf("\n");  
}
