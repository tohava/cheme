#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "lexer.h"

int num;
char word[1000];


bool str_int_eq(const char *str1, int x) {
	return x == -1 || atoi(str1) == x;
}

int main(int argc, char **argv) {
	std::vector<int> index;
	int token;
	extern int yyoffset;
	int prev_offset = -1;
	int offset;
	bool had_close = false;
	index.push_back(-1);
	while ((offset = yyoffset, (token = yylex())) != 0 &&
	       (had_close ||
	        (int)index.size() - (index.back() == -1 ? 1 : 0) != argc - 1 ||
	        !std::equal(argv + 1, argv + argc, index.begin(), str_int_eq)))
	{
		printf("token: %d\n", token);
		printf("index: ");
		for (size_t i = 0; i < index.size(); ++i) printf("%d ", index[i]);
		printf("\n");
		switch (token) {
		case OPEN:
			++index.back();
			index.push_back(-1);
			break;
		case CLOSE:
			had_close = true;
			index.pop_back();
			break;
		case SPACE:
			continue;
		default:
			++index.back();
		}
		had_close = false;
		prev_offset = offset;
	}
	printf("token: %d\n", token);
	printf("index: ");
	for (size_t i = 0; i < index.size(); ++i) printf("%d ", index[i]);
	printf("\n");
	offset = prev_offset;
	if (token)
		printf("%d", offset);
}
