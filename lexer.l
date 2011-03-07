%option noyywrap
%{
#include "lexer.h"
int yyoffset = 1;

#define YY_USER_ACTION yyoffset += yyleng;
%}


%%
\( return OPEN;
\) return CLOSE;
[0-9]+ num = atoi(yytext); return NUMBER;
[^\t\ \n\(\)\"\']+ strcpy(word, yytext); return WORD;
[\t\ \n]+  return SPACE;
. return UNKNOWN;
L?\"(\\.|[^\\"])*\" strcpy(word, yytext); return STRING;
L?'(\\.|[^\\'])+' strcpy(word, yytext); return CHAR;
%%
