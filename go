lex lexer.l
gcc -Wall -g -c lex.yy.c
g++ -Wall -g -c read.cpp
g++ -g lex.yy.o read.o
