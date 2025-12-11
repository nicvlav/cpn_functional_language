cilisp: clean y.tab.c lex.yy.c
	gcc -g cilisp.c lex.yy.c y.tab.c -o cilisp -lm

y.tab.c:
	yacc -d cilisp.y

lex.yy.c: y.tab.c
	lex cilisp.l

clean:
	rm -f cilisp lex.yy.c y.tab.c y.tab.h
