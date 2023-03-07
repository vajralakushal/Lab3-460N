all: compile

compile:
	make clean
	./assembler.linux jmp_test.asm output.obj
	gcc -std=c99 -o simulate lc3bsim3.c
	./simulate ucode3 output.obj

clean:
	rm -f simulate
	rm -f *.obj
	rm -f dumpsim

debug:
	make clean
	./assembler.linux jmp_test.asm output.obj
	gcc -std=c99 -g -o simulate lc3bsim3.c
	gdb --args ./simulate ucode3 output.obj