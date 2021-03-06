#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "utils.h"
#include "ast.h"
#include "compiler.h"
#include "vm.h"

#define DEBUG

#ifdef DEBUG
#ifdef WIN32

#include <windows.h>
#define TIME_T LARGE_INTEGER 
#define FREQ_T LARGE_INTEGER 
#define TIME(t) QueryPerformanceCounter(&(t))
#define FREQ(f) QueryPerformanceFrequency(&(f))
#define ELASPED_TIME(t1, t2, freq) ((t2).QuadPart - (t1).QuadPart) * 1000.0 / (freq).QuadPart

#else

#include <sys/time.h>  
#define TIME_T struct timeval 
#define FREQ_T double 
#define TIME(t) gettimeofday(&(t), NULL)
#define FREQ(f) 1.0
#define ELASPED_TIME(t1, t2, freq) ((t2).tv_sec - (t1).tv_sec) * 1000.0 + ((t2).tv_usec - (t1).tv_usec) / 1000.0

#endif

#endif

int main (int argc, char** argvs) {
  //Check number of arguments
  if(argc != 2){
    printf("Expected 1 argument to commandline.\n");
    exit(-1);
  }

  //Read in AST
  char* filename = argvs[1];
  ScopeStmt* stmt = read_ast(filename);

  //Compile to bytecode
  Program* program = compile(stmt);

  //Read in bytecode
  //Program* program = load_bytecode(argvs[1]);
  //Interpret bytecode
#ifdef DEBUG
  TIME_T t1, t2;
  FREQ_T freq;
  FREQ(freq);
  TIME(t1);
#endif

  interpret_bc(program);

#ifdef DEBUG
  TIME(t2);
  double interp_time = ELASPED_TIME(t1, t2, freq);
  fprintf(stderr, "Interpret Time: %.4lf ms.\n", interp_time);
#endif

}


