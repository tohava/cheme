%option noyywrap
%{
#include "lexer.h"
%}

%%
\( return OPEN;
\) return CLOSE;
[0-9]+ num = atoi(yytext); return NUMBER;
[^\t\ \n\(\)]+ strcpy(word, yytext); return WORD;
[\t\ \n]+  return SPACE;
. return UNKNOWN;
%%
