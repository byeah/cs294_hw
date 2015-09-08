#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "ast.h"
#include "interpreter.h"

#define DEBUG;


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
	return (x->value == y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

Obj* lt(IntObj* x, IntObj *y) {
	return (x->value < y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

Obj* le(IntObj* x, IntObj *y) {
	return (x->value <= y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

Obj* gt(IntObj* x, IntObj *y) {
	return (x->value > y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

Obj* ge(IntObj* x, IntObj *y) {
	return (x->value >= y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

NullObj* make_null_obj() {
	NullObj* o = malloc(sizeof(NullObj));
	o->type = Null;
	return o;
}

ArrayObj* make_array_obj(IntObj *length, Obj* init) { //Assume init is IntObj
    ArrayObj* o = malloc(sizeof(ArrayObj));
    o->type = Array;
    o->length = length->value;
    o->data = malloc(sizeof(int)*o->length);
    for(int i=0;i<o->length;i++)
        o->data[i] = ((IntObj*)init)->value;
    return o;
}

IntObj* array_length(ArrayObj* a) {
    return make_int_obj(a->length);
}

NullObj* array_set(ArrayObj* a, IntObj* i, Obj* v) { //Assume v is IntObj
    a->data[i->value] = ((IntObj*)v)->value;
    return make_null_obj();
}

Obj* array_get(ArrayObj* a, IntObj* i) {
    return (Obj*)make_int_obj(a->data[i->value]);
}

EnvObj* make_env_obj(Obj* parent) {
    EnvObj* env = malloc(sizeof(EnvObj));
    env->type = Env;
    env->table = ht_create(1007);
    return env;
}

void add_entry(EnvObj* env, char* name, Entry* entry) {
    ht_put(env->table, name, entry);
}

Entry* get_entry(EnvObj* env, char* name) {
    return (Entry*)ht_get(env->table, name);
}

Entry* make_var_entry(Obj* obj) {
    VarEntry* e = malloc(sizeof(VarEntry));
    e->type = Var;
    e->value = obj;
    return (Entry*) e;
}

Entry* make_func_entry(ScopeStmt* s, int nargs, char** args) {
    FuncEntry* e = malloc(sizeof(FuncEntry));
    e->type = Func;
    e->body = s;
    e->nargs = nargs;
    e->args =args;
    return (Entry*) e;
}


Obj* eval_exp (EnvObj* genv, EnvObj* env, Exp* e) {
    switch (e->tag) {
        case INT_EXP: {
            IntExp* e2 = (IntExp*)e;
            return (Obj*)make_int_obj(e2->value);
        }
        case NULL_EXP: {
            return (Obj*)make_null_obj();
        }
        case PRINTF_EXP: {
            PrintfExp* e2 = (PrintfExp*)e;
            printf("printf(");
            print_string(e2->format);
            for (int i = 0; i < e2->nexps; i++) {
                printf(", ");
                Obj* obj=eval_exp(genv, env, e2->exps[i]);
                if (obj->type == Int)
                    printf("%d",((IntObj*)obj)->value);
            }
            printf(")\n");
            return (Obj*)make_null_obj();
        }
        case ARRAY_EXP: {
            ArrayExp* e2 = (ArrayExp*)e;
            Obj* length = eval_exp(genv, env, e2->length);
            if (length->type!=Int) {
                printf("Error: array length is not int\n");
                exit(-1);
            }
            Obj* init = eval_exp(genv, env, e2->init);
            if (init->type!=Int) {
                printf("Error: array init value is not int\n");
                exit(-1);
            }
            return (Obj*)make_array_obj((IntObj*)length, init);
        }
        case OBJECT_EXP: {
            ObjectExp* e2 = (ObjectExp*)e;
            printf("object : (");
            for (int i = 0; i < e2->nslots; i++) {
                if (i > 0) printf(" ");
                //print_slotstmt(e2->slots[i]);
            }
            printf(")\n");
            return (Obj*)make_null_obj();
        }
        case SLOT_EXP: {
            SlotExp* e2 = (SlotExp*)e;
            print_exp(e2->exp);
            printf(".%s\n", e2->name);
            return (Obj*)make_null_obj();
        }
        case SET_SLOT_EXP: {
#ifdef DEBUG
			printf("debug: Slot Set\n");
#endif
            SetSlotExp* e2 = (SetSlotExp*)e;
            print_exp(e2->exp);
            printf(".%s = \n", e2->name);
            //print_exp(e2->value);
            return (Obj*)make_null_obj();
        }
        case CALL_SLOT_EXP: {
#ifdef DEBUG
			printf("debug: Slot Call\n");
#endif
            CallSlotExp* e2 = (CallSlotExp*)e;
#ifdef DEBUG
            printf("debug: Calling %s\n",e2->name);
#endif
            Obj* obj = eval_exp(genv, env, e2->exp);
//            return (Obj*)make_null_obj();
            if (obj->type == Int){
                IntObj* iobj = (IntObj*) obj;
                Obj* para = eval_exp(genv, env, e2->args[0]);
                if (para->type != Int){
                    printf("Error: Operand %s to Int must be Int\n",e2->name);
                    exit(-1);
                }
                IntObj* ipara = (IntObj*) para;
                if (strcmp(e2->name, "add") == 0)
                    return (Obj*)add(iobj, ipara);
                if (strcmp(e2->name, "sub") == 0)
                    return (Obj*)subtract(iobj, ipara);
                if (strcmp(e2->name, "mul") == 0)
                    return (Obj*)multiply(iobj, ipara);
                if (strcmp(e2->name, "div") == 0)
                    return (Obj*)divide(iobj, ipara);
                if (strcmp(e2->name, "mod") == 0)
                    return (Obj*)modulo(iobj, ipara);
                if (strcmp(e2->name, "lt") == 0)
                    return (Obj*)lt(iobj, ipara);
                if (strcmp(e2->name, "gt") == 0)
                    return (Obj*)gt(iobj, ipara);
                if (strcmp(e2->name, "le") == 0)
                    return (Obj*)le(iobj, ipara);
                if (strcmp(e2->name, "ge") == 0)
                    return (Obj*)ge(iobj, ipara);
                if (strcmp(e2->name, "eq") == 0)
                    return (Obj*)eq(iobj, ipara);
                printf("Error: Operator %s not recognized\n",e2->name);
                exit(-1);
            }
            if (obj->type == Array){
                ArrayObj* aobj = (ArrayObj*) obj;
//                printf("%s %d\n",e2->name,e2->nargs);
                if (strcmp(e2->name, "length") == 0)
                    return (Obj*)array_length(aobj);
                IntObj* index = (IntObj*)eval_exp(genv, env, e2->args[0]);
                if (strcmp(e2->name, "set") == 0){
                    array_set(aobj, index, eval_exp(genv, env, e2->args[1]));
                    return (Obj*)make_null_obj();
                }
                if (strcmp(e2->name, "get") == 0)
                    return array_get(aobj, index);
                printf("Error: Operator %s not recognized\n",e2->name);
                exit(-1);
            }
            for (int i = 0; i < e2->nargs; i++) {
                if (i > 0) printf(", ");
                //print_exp(e2->args[i]);
            }
            print_exp(e2->exp);
            printf(".%s(", e2->name);
            printf(")");
            return (Obj*)make_null_obj();
        }
        case CALL_EXP: {
            CallExp* e2 = (CallExp*)e;
            Entry* ent = get_entry(genv, e2->name);
            if (ent == NULL) {
                printf("Error: Function %s not found\n",e2->name);
                exit(-1);
            }
            if (ent->type != Func) {
                printf("Error: Calling %s that is not a Function \n",e2->name);
                exit(-1);
            }
            FuncEntry* func = (FuncEntry*)ent;
            if (e2->nargs != func->nargs){
                printf("Error: Args number not match when calling %s . Giving %d while %d is required\n",e2->name,e2->nargs,func->nargs);
                exit(-1);
            }
            EnvObj* new_env = make_env_obj(NULL);
            for (int i = 0; i < e2->nargs; i++) {
                add_entry(new_env, func->args[i], make_var_entry(eval_exp(genv, env, e2->args[i])));
            }
            return eval_stmt(genv, new_env, func->body);
        }
        case SET_EXP: {
            SetExp* e2 = (SetExp*)e;
            Obj* res = eval_exp(genv, env, e2->exp);
            if (get_entry(env, e2->name)!=NULL){
                add_entry(env, e2->name, make_var_entry(res));
            }
            else
                if (get_entry(genv, e2->name)!=NULL){
                    add_entry(genv, e2->name, make_var_entry(res));
                }
                else {
                    printf("Error: variable not found when setting its value\n");
                    exit(-1);
                }
            return (Obj*)make_null_obj();
        }
        case IF_EXP: {
#ifdef DEBUG 
			printf("debug: If\n");
#endif
            IfExp* e2 = (IfExp*)e;
            Obj* pred = eval_exp(genv, env, e2->pred);
            if (pred->type == Null)
                return eval_stmt(genv, env, e2->alt);
            else
                return eval_stmt(genv, env, e2->conseq);
        }
        case WHILE_EXP: {
#ifdef DEBUG
			printf("debug: While\n");
#endif
            WhileExp* e2 = (WhileExp*)e;
            while (eval_exp(genv, env, e2->pred)->type == Int) {
                eval_stmt(genv, env, e2->body);
            }
            return (Obj*)make_null_obj();
        }
        case REF_EXP: {
#ifdef DEBUG
			printf("debug: REF\n");
#endif
            RefExp* e2 = (RefExp*)e;
            Entry* ent = NULL;
            if (env != NULL)
                ent = get_entry(env, e2->name);
            if (ent == NULL)
                ent = get_entry(genv, e2->name);
            if (ent == NULL) {
                printf("Error: variable not found when referring\n");
                exit(-1);
            }
            if (ent->type == Func) {
                printf("Error: Should ref to variable rather than function\n");
                exit(-1);
            }
            return ((VarEntry*)ent)->value;
        }
        default:
            printf("Unrecognized Expression with tag %d\n", e->tag);
            exit(-1);
	}
    return NULL;
}


void exec_slotstmt (EnvObj* genv, EnvObj* env, EnvObj* obj, SlotStmt* s) {
    
}


void exec_stmt (EnvObj* genv, EnvObj* env, ScopeStmt* s) {
    switch (s->tag) {
        case VAR_STMT: {
            ScopeVar* s2 = (ScopeVar*)s;
            if (env == NULL)
                add_entry(genv, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
            else
                add_entry(env, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
            break;
        }
        case FN_STMT: {
#ifdef DEBUG
			printf("debug: FN\n");
#endif
            ScopeFn* s2 = (ScopeFn*)s;
            add_entry(genv, s2->name, make_func_entry(s2->body,s2->nargs,s2->args));
            break;
        }
		case SEQ_STMT: {
#ifdef DEBUG
			printf("debug: SEQ\n");
#endif
            ScopeSeq* s2 = (ScopeSeq*)s;
            exec_stmt(genv, env, s2->a);
            exec_stmt(genv, env, s2->b);
            break;
        }
        case EXP_STMT: {
            ScopeExp* s2 = (ScopeExp*)s;
            eval_exp(genv, env, s2->exp);
            break;
        }
        default:
            printf("Unrecognized scope statement with tag %d\n", s->tag);
            exit(-1);
    }
}

Obj* eval_stmt (EnvObj* genv, EnvObj* env, ScopeStmt* s) {
    switch (s->tag) {
        case VAR_STMT: {
#ifdef DEBUG
			printf("debug: Var\n");
#endif
            ScopeVar* s2 = (ScopeVar*)s;
#ifdef DEBUG
			printf("debug: %s\n", s2->name);
#endif
            if (env == NULL)
                add_entry(genv, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
            else
                add_entry(env, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
            return (Obj*)make_null_obj();
        }
        case FN_STMT: {
#ifdef DEBUG
			printf("debug: FN\n");
#endif
            ScopeFn* s2 = (ScopeFn*)s;
            add_entry(genv, s2->name, make_func_entry(s2->body,s2->nargs,s2->args));
            return (Obj*)make_null_obj();
        }
		case SEQ_STMT: {
#ifdef DEBUG
			printf("debug: SEQ\n");
#endif
            ScopeSeq* s2 = (ScopeSeq*)s;
            eval_stmt(genv, env, s2->a);
            return eval_stmt(genv, env, s2->b);
        }
        case EXP_STMT: {
#ifdef DEBUG
            printf("debug: EXP\n");
#endif
            ScopeExp* s2 = (ScopeExp*)s;
            return eval_exp(genv, env, s2->exp);
        }
        default:
            printf("Unrecognized scope statement with tag %d\n", s->tag);
            exit(-1);
    }
}

void interpret(ScopeStmt* s) {
	/*printf("Interpret program:\n");
	print_scopestmt(s);
	printf("\n");*/
    eval_stmt(make_env_obj(NULL), NULL, s);
}




