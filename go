lex lexer.l
gcc -Wall -g -c  lex.yy.c
g++ -Wall -g -c -DMAIN=1 read.cpp dummy.cpp
g++ -Wall -g -c read.cpp -o read_no_main.o
g++ -g lex.yy.o read.o dummy.o -o cheme
