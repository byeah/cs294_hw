all:
	gcc -O3 -w -std=c99 src/cfeeny.c src/utils.c src/ast.c src/bytecode.c src/vm.s src/vm.c  src/compiler.c -o cfeeny -Wno-int-to-void-pointer-cast

warn:
	gcc -O3 -std=c99 src/cfeeny.c src/utils.c src/ast.c src/bytecode.c src/vm.s src/vm.c  src/compiler.c -o cfeeny -Wno-int-to-void-pointer-cast

