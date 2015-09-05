#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "ast.h"
#include "interpreter.h"

int obj_type(Obj* o) {
	return o->type;
}

IntObj* make_int_obj(int value) {
	IntObj* o = malloc(sizeof(IntObj));
	o->type = Int;
	o->value = value;
	return o;
}

IntObj* add(IntObj* x, IntObj *y) {
	return make_int_obj(x->value + y->value);
}

IntObj* subtract(IntObj* x, IntObj *y) {
	return make_int_obj(x->value - y->value);
}

IntObj* multiply(IntObj* x, IntObj *y) {
	return make_int_obj(x->value * y->value);
}

IntObj* divide(IntObj* x, IntObj *y) {
	return make_int_obj(x->value / y->value);
}

IntObj* modulo(IntObj* x, IntObj *y) {
	return make_int_obj(x->value % y->value);
}

Obj* eq(IntObj* x, IntObj *y) {
	return (x->value == y->value) ? make_int_obj(0) : make_mull_obj();
}

Obj* lt(IntObj* x, IntObj *y) {
	return (x->value < y->value) ? make_int_obj(0) : make_mull_obj();
}

Obj* le(IntObj* x, IntObj *y) {
	return (x->value <= y->value) ? make_int_obj(0) : make_mull_obj();
}

Obj* gt(IntObj* x, IntObj *y) {
	return (x->value > y->value) ? make_int_obj(0) : make_mull_obj();
}

Obj* ge(IntObj* x, IntObj *y) {
	return (x->value >= y->value) ? make_int_obj(0) : make_mull_obj();
}

NullObj* make_mull_obj() {
	NullObj* o = malloc(sizeof(NullObj));
	o->type = Null;
	return o;
}

void interpret(ScopeStmt* stmt) {
	printf("Interpret program:\n");
	print_scopestmt(stmt);
	printf("\n");
}
