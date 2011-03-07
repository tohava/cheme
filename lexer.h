#ifdef __cplusplus
extern "C" {
#endif

int yylex();
#define OPEN 1
#define CLOSE 2
#define NUMBER 3
#define WORD 4
#define STRING 5
#define SPACE 6
#define CHAR 7
#define UNKNOWN -1

extern char word[1000];
extern int num;

#ifdef __cplusplus
}
#endif
