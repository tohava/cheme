#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
 
#include <list>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <list>
#include <stack>
#include <queue>
#include <memory>

#include "lexer.h"

#include "cheme_types.h"

#define HIDDEN_CHEME_LT            "hidden_cheme_less_than"
#define HIDDEN_CHEME_GT            "hidden_cheme_greater_than"
#define HIDDEN_CHEME_EQ            "hidden_cheme_equals"
#define HIDDEN_CHEME_INT_TO_DOUBLE "hidden_cheme_int_to_double"
#define HIDDEN_CHEME_DOUBLE_TO_INT "hidden_cheme_double_to_int"
#define HIDDEN_CHEME_ADD_INTS      "hidden_cheme_add_ints"
#define HIDDEN_CHEME_SUB_INTS      "hidden_cheme_sub_ints"
#define HIDDEN_CHEME_MUL_INTS      "hidden_cheme_mul_ints"
#define HIDDEN_CHEME_CMP_INTS      "hidden_cheme_cmp_ints"
#define HIDDEN_CHEME_ANY_SIZE      "hidden_cheme_any_size"
#define HIDDEN_CHEME_ANY_TYPE_NAME "hidden_cheme_any_type_name"
#define HIDDEN_CHEME_ANY_DPTR_PTR  "hidden_cheme_any_dptr_ptr"
#define HIDDEN_CHEME_ANY_DPTR      "hidden_cheme_any_dptr"
#define HIDDEN_CHEME_ANY_SPTR      "hidden_cheme_any_sptr"
#define HIDDEN_CHEME_ANY_TYPE_PTR  "hidden_cheme_any_type_ptr"
#define HIDDEN_CHEME_ANY_TYPE      "any_type"




extern "C" {
	extern cheme_type_desc_data *hidden_cheme_type_desc_data00000000;
	extern cheme_type_desc_data *hidden_cheme_type_desc_data00000001;
	extern cheme_type_desc_data *hidden_cheme_type_desc_data00000002;
	extern cheme_type_desc_data *hidden_cheme_type_desc_data00000003;
	extern cheme_type_desc_data *hidden_cheme_type_desc_data00000004;
	extern const int base_type_count;
}
cheme_type_desc
    cheme_type_desc_int         = hidden_cheme_type_desc_data00000000,
    cheme_type_desc_ptr_char    = hidden_cheme_type_desc_data00000001,
    cheme_type_desc_ptr_anylist = hidden_cheme_type_desc_data00000002,
    cheme_type_desc_sym         = hidden_cheme_type_desc_data00000003,
	cheme_type_desc_char        = hidden_cheme_type_desc_data00000004;

bool is_first_and_last_layer = true;
std::string indices_file = "";
std::string first_layer_source = "";

void load_cur_token();

struct ErrorContext {
	long pos;
	int flat_index;
};

ErrorContext current_error_context;
int current_flat_index;

std::list<ErrorContext> error_contexts;

void push_error_context(ErrorContext ec) {
	error_contexts.push_back(ec);
}

void pop_error_context() {
	error_contexts.pop_back();
}


std::string error_context_to_string_multi_layer();
std::string error_context_to_string() {
	if (error_contexts.empty())
		return "Empty error context stack";
	char buf[256];
	if (is_first_and_last_layer) {
		sprintf(buf, "char %ld", error_contexts.back().pos);
		return buf;
	} else {
		return error_context_to_string_multi_layer();
	}
}

int temp_index = 1;

std::string make_temp_name(int temp_index) {
	char buf[80];
	sprintf(buf, "hidden_cheme_temp%08d", temp_index);
	return std::string(buf);
}

#define CHEME_TYPE_DESC_VAR_PREFIX "hidden_cheme_type_desc_data"
#define TYPE_DESC_NUM_LEN 8
#define TYPE_DESC_NUM_LEN_STR "8"
std::string make_type_desc_data_name(int index) {
	char buf[80];
	sprintf(buf, CHEME_TYPE_DESC_VAR_PREFIX"%0"TYPE_DESC_NUM_LEN_STR"d", index);
	return std::string(buf);
}

std::string make_tup_name(int index) {
	char buf[80];
	sprintf(buf, "struct hidden_cheme_tup_%08d", index);
	return std::string(buf);
}


std::string get_next_temp() {
	return make_temp_name(temp_index++);
}



int num;
char word[1000];


void cheme_printf_empty_stacks_to_stderr();

#define ABORT(x) ASSERT(false,x)
#define ABORT1(x,y) ASSERT1(false,x,y)
#define ASSERT(x,y) ((x) || (fputs((y),stderr), fprintf(stderr, "\nAt %s\n", error_context_to_string().c_str()), cheme_printf_empty_stacks_to_stderr(), fprintf(stderr, "At compiler source %s:%d\n", __FILE__, __LINE__) , abort(), true))
#define ASSERT1(x,y,z) ((x) || (fprintf(stderr, (std::string(y) + "\n").c_str(), z), ASSERT_EPILOGUE
#define ASSERT2(x,y,a,b) ((x) || (fprintf(stderr, (std::string(y) + "\n").c_str(), a, b), ASSERT_EPILOGUE
#define ASSERT_EPILOGUE fprintf(stderr, "\nAt %s\n", error_context_to_string().c_str()), cheme_printf_empty_stacks_to_stderr(), fprintf(stderr, "At compiler source %s:%d\n", __FILE__, __LINE__) , abort(), true))

typedef struct {
	char *buf_origin;
	char *buf_ptr;
	size_t buf_size;
} cheme_printf_context;

#define PRINTF_ZONE_CURRENT_BODY 0
#define PRINTF_ZONE_TOP 1
std::stack<cheme_printf_context> cheme_printf_top_zone_stack;
std::stack< std::stack<cheme_printf_context> > cheme_printf_body_zones_stack;
std::queue< char* > cheme_printf_done_body_zones;
std::stack<int> cheme_printf_zone_stack;

void cheme_printf_push_body() {
	cheme_printf_body_zones_stack.push(std::stack<cheme_printf_context>());
	cheme_printf_body_zones_stack.top().push(cheme_printf_context());
}

void cheme_printf_pop_body() {
	ASSERT(!cheme_printf_body_zones_stack.empty(),
	       "cheme_printf_pop_body: No body zone to pop");
	ASSERT(cheme_printf_body_zones_stack.top().size() == 1,
	       "cheme_printf_pop_body: popped body stack must be of size 1");
	cheme_printf_done_body_zones.push
	    (cheme_printf_body_zones_stack.top().top().buf_origin);
	cheme_printf_body_zones_stack.pop();
}

void cheme_printf_set_zone(int zone) {
	cheme_printf_zone_stack.push(zone);
}

void cheme_printf_unset_zone() {
	ASSERT(!cheme_printf_zone_stack.empty(),
	       "cheme_printf_unset_zone: no cheme_printf_set_zone to undo");
	cheme_printf_zone_stack.pop();
}

std::stack<cheme_printf_context> *cheme_printf_get_current_stack() {
	ASSERT(!cheme_printf_zone_stack.empty(),
	       "cheme_printf_get_current_stack: zone stack is empty");
	switch (cheme_printf_zone_stack.top()) {
	case PRINTF_ZONE_TOP:
		return &cheme_printf_top_zone_stack;
	case PRINTF_ZONE_CURRENT_BODY:
		ASSERT(!cheme_printf_body_zones_stack.empty(),
		       "cheme_printf_get_current_stack: there is nbo top body zone");
		return &cheme_printf_body_zones_stack.top();
	default:
		ABORT("cheme_printf_get_current_stack: got unknown zone code");
	}
}

int cheme_vprintf_to_context(cheme_printf_context &cur, const char *format,
                             va_list argp)
{
	if (cur.buf_origin == NULL) {
		cur.buf_ptr = cur.buf_origin = (char*)malloc(4096);
		cur.buf_size = 4096;
	}
	size_t written;
	while (1) {
		const size_t max_write =
		    (cur.buf_size - (cur.buf_ptr - cur.buf_origin));
		va_list old_argp; va_copy(old_argp, argp);
		written = vsnprintf(cur.buf_ptr, max_write, format, argp);
		va_copy(argp, old_argp);
		if (written >= max_write) {
			ptrdiff_t buf_offset = cur.buf_ptr - cur.buf_origin;
			cur.buf_origin = (char*)realloc(cur.buf_origin,
			                                cur.buf_size * 2);
			cur.buf_ptr = (cur.buf_origin + buf_offset);
			cur.buf_size *= 2;
		} else break;
	}
	cur.buf_ptr += written;
	va_end(argp);
	return written;
}

int cheme_printf(const char *format, ...) {
	va_list argp;
	std::stack<cheme_printf_context> *p = cheme_printf_get_current_stack();
	ASSERT(!p->empty(), "cheme_printf: empty stack");
	cheme_printf_context &cur = p->top();
	va_start(argp, format);
	int written = cheme_vprintf_to_context(cur, format, argp);
	va_end(argp);
	return written;
}

void cheme_printf_push() {
	std::stack<cheme_printf_context> *p = cheme_printf_get_current_stack();
	p->push(cheme_printf_context());
	cheme_printf_context &cur = p->top();
	cur.buf_origin = cur.buf_ptr = NULL;
	cur.buf_size = 0;
}

bool cheme_printf_is_stack_empty() {
	return cheme_printf_get_current_stack()->empty();
}

char *cheme_printf_pop() {

	ASSERT(!cheme_printf_get_current_stack()->empty(),
	       "cheme_printf_pop: cannot pop from an empty stack");
	cheme_printf_context cur = cheme_printf_get_current_stack()->top();
	cheme_printf_get_current_stack()->pop();
	return cur.buf_origin == NULL ? strdup("") : cur.buf_origin;
}


void cheme_printf_empty_stacks_to_stderr() {
	fprintf(stderr, "Emptying stacks:\n");
	cheme_printf_set_zone(PRINTF_ZONE_TOP);
	fprintf(stderr, "PRE_MAIN stack:\n");
	while (!cheme_printf_is_stack_empty()) {
		fprintf(stderr, "---Element---\n%s\n", cheme_printf_pop());
	}
	cheme_printf_unset_zone();

	cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
	fprintf(stderr, "MAIN stack:\n");
	while (!cheme_printf_is_stack_empty()) {
		fprintf(stderr, "---Element---\n%s\n", cheme_printf_pop());
	}
	cheme_printf_unset_zone();
}

#define printf DO NOT USE

template <class T>
class Maybe {
public:
	Maybe() { valid = false; }
	Maybe(const T &data) : valid(true), data(data) {}
	bool is_valid() const { return valid; }
	const T &get_data() const
	{ ASSERT(valid, "Maybe::data: valid is false"); return data; }
private:
	bool valid;
	T data;
};

template <class T, class S>
class BinTyped {
public:
	bool is_left() { return left; }
	bool is_right() { return !left; }
	const T &get_left() {
		ASSERT(is_left(), "BinTyped: get_left called when not left");
		return t;
	}
	const S &get_right() {
		ASSERT(is_right(), "BinTyped: get_right called when not right");
		return s;
	}
	void set(const T &t) { left = true;  this->t = t; }
	void set(const S &s) { left = false; this->s = s; }
	BinTyped(const T &t) : left(true)  { this->t = t; }
	BinTyped(const S &s) : left(false) { this->s = s; }
private:
	bool left;
	T t; S s;
};

template <class T> void incr(T &t) { ++t; }


template <class A, class B, class FUNC>
A foldl(const FUNC &func, A acc, const B begin, const B end) {	
	for (B it = begin;
	     it != end;
	     ++it)
	{
		acc =
		    func(acc,
		         *it);
	}
	return acc;
}

template <class A, class B, class FUNC>
A foldl(const FUNC &func, A acc, const B &list) {
	return foldl(func, acc, list.begin(), list.end());
}

template <class A, class B>
const A &pair_first(const std::pair<A,B> &pair) { return pair.first; }

template <int V, class T>
int fixed_value(const T &) { return V; }

template <class T>
typename T::const_iterator begin_func(const T &x) { return x.begin(); }

template <class T>
const T &list_at(const std::list<T> &l, int index) {
	typename std::list<T>::const_iterator i = l.begin();
	while(index--) ++i;
	return *i;
}


class Type {
public:
	std::string name;
	std::list<Type> type_params;
// public:
// 	int type;
// 	union {
// 		int i;
// 		char c;
// 	} u;
// 	std::list<Type> ret_and_params; // must be outside u
};

bool is_callable_type(const Type &type) {
	return type.name == "func";
}

class IsCallableWithNParams {
public: IsCallableWithNParams(size_t n) : n(n) {}
        bool operator()(const Type &t) { return is_callable_type(t) &&
		                                        t.type_params.size() == n; }
private: const size_t n;
};

bool same_types(const Type &t1, const Type &t2) {
#define CASE(X,Y) case X: return t2.type == X && t1.Y == t2.Y;

	return t1.name == t2.name &&
	    t1.type_params.size() == t2.type_params.size() &&
	    std::equal(t1.type_params.begin(), t1.type_params.end(),
	               t2.type_params.begin(), same_types);
#undef CASE
}



class SameType {
public: SameType(const Type &t) : t(t) {}
        bool operator()(const Type &t2) { return same_types(t, t2); } 
private: Type t;
};


std::string type_to_string(const Type &type) {
	if (type.type_params.empty()) return type.name;
	std::string str = "(";
	str += type.name;
	for (std::list<Type>::const_iterator it = type.type_params.begin();
	     it != type.type_params.end(); ++it)
	{
		str += " ";
		str += type_to_string(*it);
	}
	str += ")";
	return str;
}

int type_cmp(const Type &t1, const Type &t2) {
	return strcmp(type_to_string(t1).c_str(),
	              type_to_string(t2).c_str());
}


#define TOKEN_TYPE_EOF    0
#define TOKEN_TYPE_OPEN   1
#define TOKEN_TYPE_CLOSE  2
#define TOKEN_TYPE_INT    3
#define TOKEN_TYPE_WORD   4
#define TOKEN_TYPE_STRING 5
#define TOKEN_TYPE_CHAR   6
	
struct Token {
	int type;
	std::string str;
	int num;
	bool is_specific_word(const char *word) const {
		return type == TOKEN_TYPE_WORD && !strcmp(str.c_str(), word);
	}
	bool operator ==(const Token &tok) const
	{ return type == tok.type && str == tok.str; }
};

struct TermMetaData {
	void set_type(const Type &typ) { type = Maybe<Type>(typ); }
	const Type &get_type() {
		ASSERT(type.is_valid(),
		       "TermMetaData::get_type - there is no type");
		return type.get_data();
	}
	bool has_type() { return type.is_valid(); }
private:
	Maybe<Type> type;
};

#define TERM_TYPE_LIST 0
#define TERM_TYPE_SINGLE 1
struct Term {
	int type;
	std::list<Term> list;
	Term *parent;
	int index;
	ErrorContext ec;
	Token single;
	TermMetaData metadata;
	bool is_single_specific_word(const char *word) const {
		return type == TERM_TYPE_SINGLE && single.is_specific_word(word);
	}
	bool is_single_word() const {
		return type == TERM_TYPE_SINGLE && single.type == TOKEN_TYPE_WORD;
	}
	bool is_single_int() const {
		return type == TERM_TYPE_SINGLE && single.type == TOKEN_TYPE_INT;
	}
};

std::string add_term_to_string(const std::string &s, const Term &term) {
	std::string term_to_string(const Term &t);
	return s + " " + term_to_string(term);
}

std::string term_to_string(const Term &t) {
	switch (t.type) {
	case TERM_TYPE_SINGLE:
		switch (t.single.type) {
		case TOKEN_TYPE_INT:
		{
			char buf[128]; sprintf(buf, "%d", t.single.num); return buf;
		}
		case TOKEN_TYPE_WORD:
		case TOKEN_TYPE_STRING:
		case TOKEN_TYPE_CHAR:
			return t.single.str;
		default:
			ASSERT1(false, "term_to_string: Unknown t.single.type %d",
			        t.single.type);
		}
		break;
	case TERM_TYPE_LIST:
		return foldl(add_term_to_string, std::string("("), t.list) + ")";
	default:
		ASSERT1(false, "term_to_string: Unknown t.type %d", t.type);
	}
}

int term_cmp(const Term &t1, const Term &t2) {
	return strcmp(term_to_string(t1).c_str(),
	              term_to_string(t2).c_str());
}

Token make_simple_token(int type) {
	Token t;
	t.type = type;
	return t;
}

Token make_eof_token() { return make_simple_token(TOKEN_TYPE_EOF); }
Token make_open_token() { return make_simple_token(TOKEN_TYPE_OPEN); }
Token make_close_token() { return make_simple_token(TOKEN_TYPE_CLOSE); }


Token parse_int_to_token(const int num) {
	Token tok;
	tok.type = TOKEN_TYPE_INT;
	tok.num = num;
	return tok;
}

Token parse_word_to_token(const std::string &str) {
	Token tok;
	tok.type = TOKEN_TYPE_WORD;
	tok.str = str;
	return tok;
}

Token parse_string_to_token(const std::string &str) {
	Token tok;
	tok.type = TOKEN_TYPE_STRING;
	tok.str = str;
	return tok;
}

Token parse_char_to_token(const std::string &str) {
	Token tok;
	tok.type = TOKEN_TYPE_CHAR;
	tok.str = str;
	return tok;
}

Term parse_word_to_term(const std::string &str) {
	Term term;
	term.type = TERM_TYPE_SINGLE;
	term.single = parse_word_to_token(str);
	return term;
}

Token read_token_for_real() {
	start:
	extern int yyoffset;
	current_error_context.pos = yyoffset;
	switch (yylex()) {
	case 0: return make_eof_token();
	case OPEN: return make_open_token();
	case CLOSE: return make_close_token();
	case NUMBER: return parse_int_to_token(num);
	case WORD: return parse_word_to_token(word);
	case STRING: return parse_string_to_token(word);
	case CHAR: return parse_char_to_token(word);
	case SPACE: goto start;
	case UNKNOWN: fprintf(stderr, "Unknown token %s\n", word); ABORT("");
	default: ABORT("");
	}
// begin:
// 	int c = fgetc(f);
// 	switch(c) {
// 	case EOF:
// 		return EOF_TOKEN;
// 	case '(':
// 		return OPEN_TOKEN;
// 	case ')':
// 		return CLOSE_TOKEN;
// 	default:;
// 	}
// 	if (isspace(c))
// 		goto begin; // tail recurse
// 	std::string str; str += c;
// 	while ((c = fgetc(f)) != EOF && !isspace(c) && c != ')') {
// 		str += c;
// 	}
// 	if (c != EOF) ungetc(c, f);
// 	return parse_word_to_token(str);
}

Token hidden_cur_token;

void load_cur_token() { hidden_cur_token = read_token_for_real(); }

void init_parsing(FILE *f) {
	yyrestart(f);
	load_cur_token();
	current_flat_index = 0;
}


Token peep_token() {
	return hidden_cur_token;
}

Token read_token() {
	Token t = hidden_cur_token;
	load_cur_token();
	return t;
}

Term read_term(int index) {
	Term ret;
	ret.ec = current_error_context;
	const Token tok = read_token();
//	printf("read_term: got token with type %d\n", tok.type);
	switch (tok.type) {
	case TOKEN_TYPE_INT:
	case TOKEN_TYPE_WORD:
	case TOKEN_TYPE_STRING:
	case TOKEN_TYPE_CHAR:	
		ret.type = TERM_TYPE_SINGLE;
		ret.single = tok;
		break;
	case TOKEN_TYPE_OPEN:
	{
		ret.type = TERM_TYPE_LIST;
//		printf("read_term: recursing\n");
		int index2 = 0;
		while (peep_token().type != TOKEN_TYPE_CLOSE &&
		       peep_token().type != TOKEN_TYPE_EOF) {
//			printf("read_term: calling read_term\n");
			ret.list.push_back(read_term(index2++));
		}
		if (read_token().type != TOKEN_TYPE_CLOSE) {
			ABORT("read_term: expected )");
		}
		break;
	}
	default:
		push_error_context(current_error_context);
		ABORT1("read_term: expected ( or a simple term, "
		       "instead got token type %d\n", tok.type);
		pop_error_context();
	}
	ret.index = index;
	ret.ec.flat_index = current_flat_index++;
	return ret;
//	printf("read_term: done\n");
}

std::string error_context_to_string_multi_layer() {
	char buf[512];
	FILE *f = fopen(indices_file.c_str(), "r");
	init_parsing(f);
	int i;
	Term term;
	for (i = 0; i < error_contexts.back().flat_index + 1; ++i)
		term = read_term(i);
	fclose(f);
	std::string str = "/home/tohava/cheme/index2pos ";
	std::list<Term>::iterator it = term.list.begin();
	for ( ; it != term.list.end(); ++it) {
		sprintf(buf, "%d", it->single.num);
		str += " ";
		str += buf;
	}
	str += " < " + first_layer_source + " > tmp";
	system(str.c_str());
	f = fopen("tmp","r");
	int pos = -1;
	fscanf(f, "%d", &pos);
	fclose(f);
	sprintf(buf, "char %ld", (long int)pos);
	return buf;
}


cheme_anylist *convert_term_list_to_anylist(const std::list<Term> &l) {
	cheme_any convert_term_to_any(const Term &t);
	std::list<Term>::const_iterator it = l.begin();
	cheme_anylist *head = NULL;
	cheme_anylist **p = &head;
	while (it != l.end()) {
		*p = new cheme_anylist();
		(*p)->data.car = convert_term_to_any(*it);
		(*p)->data.cdr = NULL;
		p = &((*p)->data.cdr);
		++it;
	}
	return head;
}

cheme_any anylist_ptr_to_any(cheme_anylist *head) {
	cheme_any ret;
	ret.t = cheme_type_desc_ptr_anylist;
	memcpy(ret.s, &head, sizeof(head));
	return ret;
}

std::string unescape(std::string s) {
	std::string ret;
	ret.reserve(s.size());
	const char *p = s.c_str();
	while (*p != '\0') {
		if (*p == '\\') {
			++p;
#define SIMPLE(c,d) case c: ret += d; break;
			switch (*p) {
			SIMPLE('0','\0')
			SIMPLE('n','\n')
			SIMPLE('r','\r')
			SIMPLE('\\','\\')
			SIMPLE('"','"')
			default:
				ASSERT1(false, "unescape: unknown escape sequence (%c)", *p);
			}
			++p;
#undef SIMPLE			
		} else {
			ret += *p; ++p;
		}
	}
	return ret;
}

cheme_any convert_term_to_any(const Term &t) {
	cheme_any ret;
	switch (t.type) {
	case TERM_TYPE_SINGLE:
		switch (t.single.type) {
		case TOKEN_TYPE_INT:
			ret.t = cheme_type_desc_int;
			memcpy(ret.s, &t.single.num, sizeof(t.single.num));
			break;
		case TOKEN_TYPE_WORD:
		{
			ret.t = cheme_type_desc_sym;
			char *p;
			p = strdup(t.single.str.c_str());
			cheme_sym s = {p};
			memcpy(ret.s, &s, sizeof(s));
			break;
		}
		case TOKEN_TYPE_CHAR:
		{
			ret.t = cheme_type_desc_char;
			char c = unescape(t.single.str)[1];
			memcpy(ret.s, &c, sizeof(char));
			break;
		}
		case TOKEN_TYPE_STRING:
		{
			ret.t = cheme_type_desc_ptr_char;
			char *p = strdup
			              (unescape
			                   (t.single.str.substr
			                        (1, t.single.str.size() - 2)).c_str());
			memcpy(ret.s, &p, sizeof(char*));
			break;
		}
		default:
			ASSERT1(false, "read_term_as_any: Unknown token type %d\n", t.single.type);
		}
		break;
	case TERM_TYPE_LIST:
	{
		ret = anylist_ptr_to_any(convert_term_list_to_anylist(t.list));
	}
	}
	return ret;

}

cheme_any read_all_terms_from_file_as_any(FILE *f) {
	init_parsing(f);
	std::list<Term> all_terms;
	for (int i = 0; peep_token().type != TOKEN_TYPE_EOF; ++i)
		all_terms.push_back(read_term(i));
	return anylist_ptr_to_any(convert_term_list_to_anylist(all_terms));
}

cheme_any read_term_from_file_as_any(FILE *f) {
	init_parsing(f);
	return convert_term_to_any(read_term(0));
}

extern "C" {
	cheme_any read_term_as_any() {
		return convert_term_to_any(read_term(0));
	}

	cheme_any read_all_terms_as_any() {
		return read_all_terms_from_file_as_any(stdin);
	}

	cheme_any read_term_from_str(char *str) {
		FILE *f = fopen("quote_temp", "w+b"); //tmpfile();
		fputs(str, f);
		rewind(f);
		return read_term_from_file_as_any(f);
		fclose(f);
	}
	
	int init_read_term_as_any() {
		init_parsing(stdin);
		return 0;
	}
}

void set_term_parent_worker(Term *term, Term *parent) {
	term->parent = parent;
	if (term->type == TERM_TYPE_LIST) {
		std::list<Term>::iterator it = term->list.begin();
		for (; it != term->list.end(); ++it) {
			set_term_parent_worker(&*it, term);
		}
	}
}

void set_term_parent(Term &term) {
	set_term_parent_worker(&term, NULL);
}

void write_token(FILE *f, const Token &tok) {
	switch (tok.type) {
	case TOKEN_TYPE_OPEN:
		fputc('(', f);
		break;
	case TOKEN_TYPE_CLOSE:
		fputc(')', f);
		break;
	case TOKEN_TYPE_WORD:
		fputc(' ', f);
		fputs(tok.str.c_str(), f);
		fputc(' ', f);
		break;
	case TOKEN_TYPE_EOF:
		ABORT("Cannot display EOF");
	default:
		ABORT("write_token: unknown token");
	}
}

// class WriteTerm {
// public:
// 	WriteTerm(FILE *f) : f(f) {}
// 	void operator()(const Term &term) {
// 		void write_term(FILE *f, const Term &term);
// 		write_term(f, term);
// 	}
// private:
// 	FILE *f;
// };

// void write_term(FILE *f, const Term &term) {
// 	switch (term.type) {
// 	case TERM_TYPE_SINGLE:
// 		write_token(f, term.single);
// 		break;
// 	case TERM_TYPE_LIST:
// 		write_token(f, OPEN_TOKEN);
// 		std::for_each(term.list.begin(), term.list.end(), WriteTerm(f));
// 		write_token(f, CLOSE_TOKEN);
// 		break;
// 	default:
// 		ABORT("write_term: Unknown term");
// 	}
// }

Term force_to_list(const Term &term) {
	switch (term.type) {
	case TERM_TYPE_LIST: return term;
	case TERM_TYPE_SINGLE:
	{
		Term t;
		t.type = TERM_TYPE_LIST;
		t.list.push_back(term);
		return t;
	}
	default:
		ABORT("");
	}
}

bool same_terms(const Term &t1, const Term &t2);

bool same_term_lists(const std::list<Term> &t1,
                     const std::list<Term> &t2) {
	return t1.size() == t2.size() &&
	    std::equal(t1.begin(), t1.end(), t2.begin(), same_terms);
}

bool same_type_lists(const std::list<Type> &t1,
                     const std::list<Type> &t2) {
	return t1.size() == t2.size() &&
	    std::equal(t1.begin(), t1.end(), t2.begin(), same_types);
}

bool same_terms(const Term &t1, const Term &t2) {
#define CASE1(X,Y)   case X: return t2.type == X && t1.Y == t2.Y;
#define CASE2(X,Y,F) case X: return t2.type == X && F(t1.Y,t2.Y);
	switch (t1.type) {
	CASE1(TERM_TYPE_SINGLE,single);
	CASE2(TERM_TYPE_LIST,list,same_term_lists);
	default:
		ABORT("same_terms: Unknown term type");
	}
#undef CASE2
#undef CASE1
}


#define TYPE_TYPE_INT 0
#define TYPE_TYPE_CHAR 1
#define TYPE_TYPE_VOID 2

template <class T, class FUNC>
T filter(const T &values, const FUNC &fun) {
	T ret;
	typename T::const_iterator iter = values.begin();
	for (; iter != values.end(); ++iter) {
//		printf("filter: debug: %d\n", iter != values.end());
		if (fun(*iter)) {
//			printf("filter: pushing back\n");
			ret.push_back(*iter);
		}
	}
	return ret;
}

template <class T, class FUNC>
bool all(FUNC f, const T &list) {
	typename T::const_iterator it = list.begin();
	for (; it != list.end(); ++it) if (!f(it)) return false;
	return true;
}

template <class T, class FUNC, class S>
std::list<S> map(FUNC f, const T &list, S *dummy) {
	std::list<S> ret;
	for (typename T::const_iterator it = list.begin();
	     it != list.end(); ++it)
	{
		ret.push_back(f(*it));
	}
	return ret;
}

template <class T, class FUNC>
void set_map(FUNC f, T &t)
{
	std::transform(t.begin(), t.end(), t.begin(), t.end(), f);
}

template <class A, class B, class C, class F>
void set_filter_with_assoc2(const F &pred,A &list1, B &list2, C &list3) {
	ASSERT(list1.size() == list2.size() &&
	       list2.size() == list3.size(),
	       "set_filter_with_assoc2: lists should have same size");
	typename A::iterator a = list1.begin();
	typename B::iterator b = list2.begin();
	typename C::iterator c = list3.begin();
	for ( ; a != list1.end(); ++a, ++b, ++c) {
		if (pred(*a)) {
			list1.erase(a); list2.erase(b); list3.erase(c);
		}
	}
}



class FunctionManager {
private:
	class FunctionData {
	public:
		Type t;
		int scope_depth;
		int lambda_depth;
		std::string true_name;
		bool is_c_func;
	};
	typedef std::map<std::string, std::list<FunctionData> > map_t;
	typedef std::list< std::pair< std::string, std::list<map_t::iterator> > >
	    scope_t;
public:
	static bool accessible(const std::string &name)
	{
		FunctionData *p = get(name);
		return p != NULL && (p->lambda_depth == 0 ||
		                     p->lambda_depth == lambda_depth ||
		                     p->is_c_func);
	}
	
	static const Type &type(const std::string &name) {
		FunctionData *p = get(name);
		ASSERT(p != NULL, "FunctionManager::type: unknown name");
		return p->t;
	}
	static const std::string &true_name(const std::string &name) {
		FunctionData *p = get(name);
		ASSERT(p != NULL, "FunctionManager::true_name: unknown name");
		return p->true_name;
	}
	static void add_simple(const std::string &name, const Type &newtype)
	{
		add(name, newtype, name, "", false);
	}
	static void add(const std::string &name, const Type &newtype,
	                const std::string &true_name,
	                const std::string &dtor_str,
	                const bool is_c_func)
	{
		map_t::iterator it = map.insert
		    (std::make_pair(name, std::list<FunctionData>())).first;


		std::list<FunctionData> &list = it->second;
		if (!list.empty() && list.back().scope_depth == cur_scope_depth()) {
			fprintf(stderr,
			        "Attempt to add variable '%s' twice\n", name.c_str());
			ABORT("");
		}
		list.push_back(FunctionData());
		list.back().t = newtype;
		list.back().scope_depth = cur_scope_depth();
		list.back().lambda_depth = lambda_depth;
		list.back().true_name = true_name;
		list.back().is_c_func = is_c_func;
		scope.back().second.push_back(it);
		scope.back().first += dtor_str;
	}
	static void start_scope() {
		scope.push_back(scope_t::value_type());
	}
	static void list_pop_and_maybe_delete(map_t::iterator it) {
		it->second.pop_back();
		if (it->second.empty()) map.erase(it);
	}
	static void end_scope() {
		cheme_printf("%s", scope.back().first.c_str());
		std::for_each(scope.back().second.begin(), scope.back().second.end(),
		              list_pop_and_maybe_delete);
		scope.pop_back();
	}
	static void start_lambda(std::string str) {
		++lambda_depth;
		lambda_list.push_back(str);
	}
	static void end_lambda() {
		--lambda_depth;
		ASSERT(lambda_depth >= 0, "lambda depth cannot be negative");
		lambda_list.pop_back();
	}
	static std::list<std::string> &get_lambda_list() { return lambda_list; }
	static int cur_scope_depth() { return scope.size() - 1; }
	static bool in_global_scope() { return lambda_depth == 0;}
	
private:
	static FunctionData *get(const std::string &name) {
		map_t::iterator it = map.find(name);
		return it == map.end() ? NULL : &map.find(name)->second.back();
	}
	static map_t map;
	static scope_t scope;
	static int lambda_depth;
	static std::list<std::string> lambda_list;
};

FunctionManager::map_t FunctionManager::map;
FunctionManager::scope_t FunctionManager::scope;
int FunctionManager::lambda_depth = 0;
std::list<std::string> FunctionManager::lambda_list;

class TypeInfo {
public:
	int size;
};

typedef TypeInfo (*TypeInfoFunc_t)(const void *v,
                                   const std::list<Type> &type_params); 

#define SIMPLE_TYPEINFO_GETTER(type)									 \
TypeInfo get_##type##_type_info(const void *v, const std::list<Type> &ignored) \
{ TypeInfo t; t.size = sizeof(type); return t;}


SIMPLE_TYPEINFO_GETTER(int)
SIMPLE_TYPEINFO_GETTER(char)
SIMPLE_TYPEINFO_GETTER(cheme_bool)
SIMPLE_TYPEINFO_GETTER(cheme_unit)
SIMPLE_TYPEINFO_GETTER(double)
SIMPLE_TYPEINFO_GETTER(cheme_any)
SIMPLE_TYPEINFO_GETTER(cheme_sym)
SIMPLE_TYPEINFO_GETTER(cheme_type_desc)
TypeInfo get_func_type_info(const void *v, const std::list<Type> &ignored)
{ TypeInfo t; t.size = sizeof(void(*)()); return t; }
TypeInfo get_ptr_type_info(const void *v, const std::list<Type> &ignored)
{ TypeInfo t; t.size = sizeof(void*); return t; }
TypeInfo get_anylist_type_info(const void *v, const std::list<Type> &ignored)
{ struct { cheme_any a; void *p; } s; TypeInfo t; t.size = sizeof(s); return t; }
TypeInfo get_tup_type_info(const void *ignored, const std::list<Type> &t);
TypeInfo get_packed_type_info(const void *v, const std::list<Type> &ignored);


class TypePackManager {
public:
	static void add(const std::string &str) {
		std::map<std::string, Maybe<Type> >::iterator it = map.find(str);
		ASSERT(it == map.end(), "TypePackManager::add/1 - already there");
		map.insert(std::make_pair(str, Maybe<Type>()));
	}
	static void add(const std::string &str, const Type &type) {
		std::map<std::string, Maybe<Type> >::iterator it = map.find(str);
		if (it == map.end())
			map.insert(std::make_pair(str, Maybe<Type>(type)));
		else {
			ASSERT(!it->second.is_valid(),
			       "TypePackManager::add/2 - already there");
			it->second = Maybe<Type>(type);
		}
	}
	static bool is_type_pack(const std::string &str) {
		return map.find(str) != map.end();
	}
	static bool is_defined_type_pack(const std::string &str) {
		return try_unpack(str) != NULL;
	}
	static bool is_opaque_type_pack(const std::string &str) {
		return is_type_pack(str) && !is_defined_type_pack(str);
	}
	static const Type *try_unpack(const std::string &str) {
		std::map<std::string, Maybe<Type> >::const_iterator it = map.find(str);
		return (it == map.end() || !it->second.is_valid()) ?
		    NULL : &it->second.get_data();
	}
	static const Type &unpack(const std::string &str) {
		const Type *p = try_unpack(str);
		ASSERT1(p != NULL, "TypePackManager::unpack - unknown type %s",
		        str.c_str());
		return *p;
	}
private:
	static std::map<std::string, Maybe<Type> > map;
};

Type build_base_type(const std::string &str);
Type build_uni_poly_type(const std::string &str, const Type &type);
Type build_poly_type(const std::string &str,
                     const std::list<Type> &type_params);
class TypeManager {
public:
	static void init() {
#define NP(x) std::make_pair(x, (void *)NULL)
#define BTI(x) base_types.insert(x)
		BTI(std::make_pair("int",   NP(get_int_type_info)));
		BTI(std::make_pair("char",  NP(get_char_type_info)));
		BTI(std::make_pair("bool",  NP(get_cheme_bool_type_info)));
		BTI(std::make_pair("unit",  NP(get_cheme_unit_type_info)));
		BTI(std::make_pair("double",NP(get_double_type_info)));
		BTI(std::make_pair("any",   NP(get_cheme_any_type_info)));
		BTI(std::make_pair("type_desc", NP(get_cheme_type_desc_type_info)));
		BTI(std::make_pair("sym", NP(get_cheme_sym_type_info)));
		// we hope all funcptrs have same size
#define PTI(name,arity,func) \
poly_types.insert(std::make_pair(name, std::make_pair(arity, NP(func))))
		PTI("func", -1, get_func_type_info);
		PTI("tup", -1, get_tup_type_info);
		PTI("ptr", 1, get_ptr_type_info);

		// add anylist
		TypePackManager::add("anylist");
		BTI(std::make_pair("anylist", NP(get_anylist_type_info)));
		std::list<Type> l;
		l.push_back(build_base_type("any"));
		l.push_back(build_uni_poly_type("ptr", build_base_type("anylist")));
		TypePackManager::add("anylist", build_poly_type("tup", l));

		// add sym

#undef BTI
#undef PTI
#undef NP
	}
	static void add_base_type(const std::string &str,
	                          TypeInfoFunc_t func, const void *ptr) {
		base_types[str] = std::make_pair(func, ptr);
	}
	static bool is_base_type(const std::string &str) {
		return base_types.find(str) != base_types.end();
	}
	static bool is_poly_type(const std::string &str) {
		return poly_types.find(str) != poly_types.end();
	}
	static int poly_type_arity(const std::string &str) {
		ASSERT(is_poly_type(str),
		       "poly_type_arity: called with nonexistent type");
		return poly_types[str].first;
	}
	static std::pair<TypeInfoFunc_t, const void *> type_info_getter_pair
	    (const std::string &str)
	{
		ASSERT((is_poly_type(str) ? 1 : 0) + (is_base_type(str) ? 1 : 0) == 1,
		       "type_info_getter: unknown type");
		return is_poly_type(str) ?
		    poly_types[str].second : base_types[str];
	}
private:
	static std::map<std::string, std::pair<TypeInfoFunc_t, const void *> >
	    base_types;
	static std::map<std::string,
	                std::pair<int,
	                          std::pair<TypeInfoFunc_t,
	                                    const void *> > > poly_types;
};

std::map<std::string, std::pair<TypeInfoFunc_t, const void *> >
    TypeManager::base_types;
std::map<std::string,
                std::pair<int,
                          std::pair<TypeInfoFunc_t,
                                    const void *> > > TypeManager::poly_types;


class TypeInstanceManager {
private:
	class Compare {
	public:
		bool operator()(const Type &t1, const Type &t2)
		{ return type_cmp(t1, t2) < 0; }
	};
	typedef std::map<Type, int, Compare> map_t;
public:
	static int count() {
		return map.size();
	}
	static int add(const Type &type) {
		std::pair<map_t::iterator, bool> pair =
		    map.insert(std::make_pair(type, map.size()));
		return pair.first->second;
	}
	static int id(const Type &type) {
		int cnt = count();
		int ret = add(type);
		ASSERT1(cnt == count(),
		        "TypeInstanceManager::id - called for unknown type %s",
		        type_to_string(type).c_str());
		return ret;
	}
	static std::list< std::pair<Type, int> > all() {
		std::list< std::pair<Type, int> > ret;
		for (map_t::iterator it = map.begin(); it != map.end(); ++it)
			ret.push_back(*it);
		return ret;
	}
	static void init() {
#define ENTRY(longid, id, expr, size, name) \
TypeInstanceManager::add(expr);
#define LASTENTRY ENTRY
#include "base_types_table.h"
#undef LASTENTRY
#undef ENTRY
	}
	static map_t map;
};
TypeInstanceManager::map_t TypeInstanceManager::map;

TypeInfo type_info(const Type &t) {
	std::pair<TypeInfoFunc_t, const void *> pair =
	    TypeManager::type_info_getter_pair(t.name);
	return pair.first(pair.second, t.type_params);
}

bool is_not_list(const Term &term) {
	return term.type != TERM_TYPE_LIST;
}

Type build_base_type(const std::string &str) {
	Type t;
	ASSERT(TypeManager::is_base_type(str), "build_base_type: "
	                                       "called with unknown type");
	t.name = str;
	return t;
}

Type build_poly_type(const std::string &str,
                     const std::list<Type> &type_params) {
	Type t;
	ASSERT(TypeManager::is_poly_type(str),
	       "build_poly_type: called with unknown type");
	ASSERT(TypeManager::poly_type_arity(str),
	       "build_poly_type: called with bad arity");
	t.name = str;
	t.type_params = type_params;
	return t;
}

Type build_uni_poly_type(const std::string &str, const Type &type) {
	std::list<Type> l; l.push_back(type);
	return build_poly_type(str, l);
}

Type type_from_term(const Term &term) {
	Type ret;
	if (term.type == TERM_TYPE_SINGLE) {
		ASSERT(term.single.type == TOKEN_TYPE_WORD,
		       "type_from_term: base type name must be a word");
		ASSERT1(TypeManager::is_base_type(term.single.str),
		        "type_from_term: unknown base type %s",
		        term.single.str.c_str());
		ret.name = term.single.str;
	} else {
		ASSERT(!term.list.empty(),
		        "type_from_term: Found empty list where type should be");
		ASSERT(term.list.begin()->is_single_word(),
		       "type_from_term: poly type name must be a word");
		const std::string &name = term.list.begin()->single.str;
		ASSERT1(TypeManager::is_poly_type(name),
		        "type_from_term: unknown poly type %s", name.c_str());
		ASSERT1(((int)term.list.size() - 1 ==
		         TypeManager::poly_type_arity(name)) ||
		        TypeManager::poly_type_arity(name) == -1,
		        "type_from_term: bad arity for poly type parameters of %s",
		        name.c_str());
		ret.name = term.list.begin()->single.str;
		std::list<Term>::const_iterator it = term.list.begin(); ++it;
		for ( ; it != term.list.end(); ++it)
			ret.type_params.push_back(type_from_term(*it));
	}
	return ret;
}


TypeInfo get_tup_type_info(const void *ignored, const std::list<Type> &t)
{
	TypeInfo i;
	i.size = 0;
	int maxsize = 0;
	for (std::list<Type>::const_iterator it = t.begin(); it != t.end(); ++it) {
		const int size = type_info(*it).size;
		if (i.size % size != 0) i.size += size - (i.size % size);
		i.size += size;
		maxsize = size > maxsize ? size : maxsize;
	}
	if (i.size % maxsize != 0) i.size += maxsize - (i.size % maxsize);	
	return i;
}

TypeInfo get_packed_type_info(const void *v, const std::list<Type> &ignored)
{ return type_info(TypePackManager::unpack((const char*)v)); }

std::map<std::string, Maybe<Type> > TypePackManager::map;

int handle_term(Term &term);
int handle_term_for_fold(int ignored, Term &term) {
	return handle_term(term);
}


Type try_var_term_var_list_elem_type(const std::list<Term> &list) {
	ASSERT(!list.empty(), "try_var_term_var_list_elem_type: missing type");
	return type_from_term(*list.begin());
}

std::string try_var_term_var_list_elem_name(const std::list<Term> &list) {
	ASSERT(list.size() > 1, "try_var_term_var_list_elem_name: missing name");
	std::list<Term>::const_iterator it = list.begin(); ++it;
	ASSERT(it->is_single_word(),
	       "try_var_term_var_list_elem_name: name should be 2nd element");
	return it->single.str;
}

std::pair<int, Type> try_var_term_var_list_elem_init(std::list<Term> &list) {
	if (list.size() > 2) {
		std::list<Term>::iterator it = list.begin(); ++++it;
		int ret = handle_term(*it);
		return std::make_pair(ret, it->metadata.get_type());
	} else
		return std::make_pair(0, build_base_type("unit"));;
}

std::string word_from_term(const Term &term) {
	ASSERT(term.type == TERM_TYPE_SINGLE &&
	       term.single.type == TOKEN_TYPE_WORD,
	       "word_from_term: unsupported input");
	return term.single.str;
}


std::string try_var_term_var_list_elem_translate_basic
    (const bool is_decl,
     const std::string &tname,
     const std::string &name)
{
	return (std::string(is_decl ? "extern " : "") +
	        tname + " " + name);
}

std::string try_var_term_var_list_elem_translate
    (const Type &type,
     const std::string &name,
     const bool is_decl)
{
#define BASIC try_var_term_var_list_elem_translate_basic
	if (type.name == "func") {
		ASSERT(TypeManager::is_poly_type(type.name),
		       "func is not really func?");
		ASSERT(!type.type_params.empty(),
		       "try_var_term_var_list_elem_translate: "
		       "func without type_params???");
		std::list<Type>::const_iterator it = type.type_params.end();
		const std::list<Type>::const_iterator itRet = --it;
		std::string ret, params;
		params += "(";
		for (it = type.type_params.begin(); it != itRet; ++it) {
			if (it != type.type_params.begin()) params += ",";
			params += try_var_term_var_list_elem_translate(*it, "", false);
		}
		params += ")";
		ret = (try_var_term_var_list_elem_translate(*itRet, "", false) +
		       " " +
		       (is_decl ? "" : "(*") + name + (is_decl ? "" : ")"));
		return ret + params + "";
	} else if (type.name == "ptr") {
		return try_var_term_var_list_elem_translate(*type.type_params.begin(),
		                                            "*" + name, is_decl);
	} else if (type.name == "tup") {
		return try_var_term_var_list_elem_translate_basic
		    (is_decl, make_tup_name(TypeInstanceManager::add(type)), name);
	} else if (TypePackManager::is_type_pack(type.name)) {
		return try_var_term_var_list_elem_translate_basic
		    (is_decl, "struct " + type.name, name);
	} else {
		ASSERT(type.type_params.empty(),
		       "try_var_term_var_list_elem_translate: unknown type");
		return try_var_term_var_list_elem_translate_basic
		    (is_decl, type.name, name);
	}
}

void set_any_type(const std::string &name, const Type &type) {
	int typetd = TypeInstanceManager::add(type);
	cheme_printf
	    ("*"HIDDEN_CHEME_ANY_TYPE_PTR"(&%s) = %s;\n",
	     name.c_str(),
	     make_type_desc_data_name(typetd).c_str());
}

void init_any_to_unit(const std::string &name) {
	set_any_type(name, build_base_type("unit"));
}

void try_var_term_var_list(std::list<Term>::iterator it,
                           const std::list<Term>::const_iterator itEnd,
                           const bool is_decl) {
begin:
	if (it == itEnd)
		return;
	else {
		Term &term = *it;
		push_error_context(term.ec);
		ASSERT(term.type == TERM_TYPE_LIST &&
		       2 <= term.list.size() && term.list.size() <= (is_decl ? 2 : 3),
		       "Invalid variable format");
		pop_error_context();
		std::string name = try_var_term_var_list_elem_name(term.list);
		Type type = try_var_term_var_list_elem_type(term.list);
		const std::pair<int, Type> init_val_index_and_type =
		    try_var_term_var_list_elem_init(term.list); 
		int init_value_temp_index = init_val_index_and_type.first;
		ASSERT(init_value_temp_index == 0 ||
		       same_types(init_val_index_and_type.second, type),
		       "Variable initialization should use same type");
		std::string translation = try_var_term_var_list_elem_translate
		    (type, name, is_decl);
		bool is_c_func = type.name == "func" && init_value_temp_index < 0;
		bool in_global_scope = FunctionManager::in_global_scope();
		if (!is_c_func) {
			// inital lambdas are a special case, they write themselves
			cheme_printf_set_zone(in_global_scope ?
			                      PRINTF_ZONE_TOP : PRINTF_ZONE_CURRENT_BODY);
			cheme_printf("%s;\n",translation.c_str());
			cheme_printf_unset_zone();
			if (init_value_temp_index != 0) {
				cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
				cheme_printf("%s = %s;\n", name.c_str(),
				             make_temp_name(init_value_temp_index).c_str());
				cheme_printf_unset_zone();
			} else {
				if (same_types(type, build_base_type("any")))
					init_any_to_unit(name);
			}
		}
		if (!same_types(type, build_base_type("any"))) {
			std::string real_name = is_c_func && !in_global_scope ?
			    make_temp_name(-init_value_temp_index) : name;
			FunctionManager::add(name, type, real_name, "", is_c_func);
		} else {
			cheme_printf_push();
			cheme_printf("if (%s(&%s) > %d) free(*%s(&%s));\n",
			             HIDDEN_CHEME_ANY_SIZE, name.c_str(),
			             ANY_SIZE_THRESHOLD,
			             HIDDEN_CHEME_ANY_DPTR_PTR, name.c_str());
			std::auto_ptr<char> p(cheme_printf_pop());
			FunctionManager::add(name, type, name, std::string(p.get()), false);
		}
//		printf("Var %s Type %s\n", name.c_str(), type_to_string(type).c_str());
		++it;
		goto begin;
	}
}

bool is_specific_special_form(const Term &term, const std::string &str) {
	return (term.type == TERM_TYPE_LIST &&
	        term.list.begin()->type == TERM_TYPE_SINGLE &&
	        term.list.begin()->single.is_specific_word(str.c_str()));
}

int try_var_term(Term &term) {
	if (!is_specific_special_form(term, "var"))
		return 0;

	std::list<Term>::iterator it = term.list.begin(); ++it;
	try_var_term_var_list(it, term.list.end(), false);
	term.metadata.set_type(build_base_type("unit"));
	return temp_index++;
}

// void try_decl_term_decl_list_print_param(const std::string &elem) {
// 	printf("'%s' ", elem.c_str());
// }

// Type try_decl_term_decl_list_elem_name
//     (const std::list<Term> &list0)
// {
// 	std::list<Term> list = filter(list0, is_not_type_term);
// 	switch (list.size()) {
// 	case 0:
// 		ABORT("Missing name for function");
// 	case 1:
// 		return type_from_term(*list.begin());
// 	default:
// 		ABORT("function can only have one name");
// 	}
// }

// Term try_decl_term_decl_list_elem_ret
//     (const std::list<Term> &list0)
// {
// 	std::list<Term> list = filter(list0, is_type_term);
// 	switch (list.size()) {
// 	case 0:
// 		ABORT("Missing return type for function");
// 	case 1:
// 		return *list.begin();
// 	default:
// 		ABORT("function can only have one return type");
// 	}
// }


// void try_decl_term_decl_list_elem_params
//     (std::list<Term>::const_iterator it,
//      const std::list<Term>::const_iterator itEnd)
// {
// 	try_var_term_var_list(it, itEnd);
// }

// void print_param_pair(const std::pair<Term,std::string> &pair) {
// 	printf("%20s %20s\n", type_to_string(pair.first).c_str(), pair.second.c_str());
// }

// void try_decl_term_decl_list
//     (std::list<Term>::const_iterator it,
//      const std::list<Term>::const_iterator itEnd) {
// begin:
// 	if (it == itEnd)
// 		return;
// 	else {
// 		const Term &term = *it;
// 		ASSERT(term.type == TERM_TYPE_LIST &&
// 		       filter(term.list, is_not_list).empty(),
// 		      "Parameters to decl should be lists of lists");
// 		const std::string name = try_decl_term_decl_list_elem_name(term.list.begin()->list);
// 		const Term ret_type = try_decl_term_decl_list_elem_ret(term.list.begin()->list);
// 		printf("Function %s returns %s, parameters are:\n",
// 		       name.c_str(), word_from_term(ret_type).c_str());
// 		{
// 			std::list<Term>::const_iterator it2 = term.list.begin(); ++it2;
// 			try_decl_term_decl_list_elem_params(it2, term.list.end());
// 		}
// 		FunctionManager::add(name, ret_type);
// 		printf("---end of parameter list---\n");
// 	}
// }

int try_decl_term(Term &term) {
	if (!is_specific_special_form(term, "decl"))
		return 0;

	std::list<Term>::iterator it = term.list.begin(); ++it;
	try_var_term_var_list(it, term.list.end(), true);
	term.metadata.set_type(build_base_type("unit"));
	return temp_index++;
}

std::string is_app_form(const Term &term) {
	return (term.type == TERM_TYPE_LIST &&
	        term.list.begin()->type == TERM_TYPE_SINGLE &&
	        term.list.begin()->single.type == TOKEN_TYPE_WORD &&
	        FunctionManager::accessible(term.list.begin()->single.str)) ?
	    term.list.begin()->single.str : "";
}

void assignment_for_anys(const char *left_str,
                         const char *right_str) {
#define MAX_SIZE ANY_SIZE_THRESHOLD		
	cheme_printf("{\n");
	cheme_printf("int size1 = %s(&%s);\n",
	             HIDDEN_CHEME_ANY_SIZE, left_str);
	cheme_printf("int size2 = %s(&%s);\n",
	             HIDDEN_CHEME_ANY_SIZE, right_str);
	cheme_printf("if (size2 > %d) {\n"
	             "  if (size1 != size2 && size1 > %d) free(%s(&%s));\n"
	             "  *%s(&%s) = malloc(size2);\n"
	             "}",
	             MAX_SIZE, MAX_SIZE, HIDDEN_CHEME_ANY_DPTR, left_str,
	             HIDDEN_CHEME_ANY_DPTR_PTR, left_str);
	cheme_printf("else if (size1 >  %d && size2 <= %d) "
	             "free(%s(&%s));\n",
	             MAX_SIZE, MAX_SIZE,
	             HIDDEN_CHEME_ANY_DPTR, left_str);
	cheme_printf("if      (size2 <= %d) "
	             "%s = %s;\n",
	             MAX_SIZE, left_str, right_str);
	cheme_printf("else if (size2 >  %d) {"
	             "*%s(&%s) = *%s(&%s);\n"
	             "memcpy(%s(&%s),%s(&%s),size2);\n}\n",
	             MAX_SIZE, HIDDEN_CHEME_ANY_TYPE_PTR, left_str,
	             HIDDEN_CHEME_ANY_TYPE_PTR, right_str,
	             HIDDEN_CHEME_ANY_DPTR_PTR, left_str,
	             HIDDEN_CHEME_ANY_DPTR_PTR, right_str);
	cheme_printf("}\n");
#undef MAX_SIZE 
	
}						 

int try_app_term_params_any_dup(int middle_index) {
	int result_index = temp_index++;
	std::string s = try_var_term_var_list_elem_translate
	    (build_base_type("any"), make_temp_name(result_index), false);
	cheme_printf("%s;\n", s.c_str());
	init_any_to_unit(make_temp_name(result_index));
	assignment_for_anys(make_temp_name(result_index).c_str(),
	                    make_temp_name(middle_index).c_str());
	return result_index;
}

std::list<int> try_app_term_params
    (std::list<Type>::const_iterator itType,
     const std::list<Type>::const_iterator itTypeEnd,
     std::list<Term>::iterator itTerm,
     const std::list<Term>::const_iterator itTermEnd)
{
	std::list<int> ret;
begin:
	int at_end =
	    ((itType == itTypeEnd) ? 1 : 0) + ((itTerm == itTermEnd) ? 1 : 0);
	ASSERT(at_end % 2 == 0, "try_app_term_params: arity mismatch");
	if (!at_end) {
		int middle_index = handle_term(*itTerm);
		int result_index = same_types(*itType, build_base_type("any")) ?
		    try_app_term_params_any_dup(middle_index) : middle_index; 
		ret.push_back(result_index);
		ASSERT2(same_types(itTerm->metadata.get_type(), *itType),
		        "Inferred type '%s' of application argument does "
		        "not match expected '%s'",
		        type_to_string(itTerm->metadata.get_type()).c_str(),
		        type_to_string(*itType).c_str());
		++itType; ++itTerm;
		goto begin;
	}
	return ret;
}

int try_app_term(Term &term) {
	const std::string str = is_app_form(term);
	if (str.empty()) return 0;
	const Type t = FunctionManager::type(str);
	ASSERT(is_callable_type(t), "Attempt to do application on non-callable");
	const std::list<Type>::const_iterator itType = t.type_params.begin();
	const std::list<Type>::const_iterator itTypeEnd = t.type_params.end();
	std::list<Type>::const_iterator itTypeRet = itTypeEnd; --itTypeRet;
	std::list<Term>::iterator itTerm = term.list.begin(); ++itTerm;
	const std::list<Term>::iterator itTermEnd = term.list.end();
	const std::list<int> temp_indices = try_app_term_params(itType, itTypeRet,
	                                                        itTerm, itTermEnd);
//	printf("Application of %s\n", str.c_str());
	cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
	int result_index = temp_index++;
#define TVTVLET try_var_term_var_list_elem_translate
	{
		std::list<std::string> &ll = FunctionManager::get_lambda_list();
		if (std::find(ll.begin(), ll.end(), str) != ll.end()) {
			cheme_printf("%s;\n",
			             TVTVLET(t, FunctionManager::true_name(str).c_str(),
			                     true).c_str());
		}
	}
	cheme_printf("%s = %s(",
	       TVTVLET(*itTypeRet, make_temp_name(result_index).c_str(),
	               false).c_str(), FunctionManager::true_name(str).c_str());
#undef TVTVLET
	std::list<int>::const_iterator it = temp_indices.begin();
	for (; it != temp_indices.end(); ++it)
		cheme_printf("%s%s", it == temp_indices.begin() ? "" : ",",
		             make_temp_name(*it).c_str());
	cheme_printf(");\n");
	term.metadata.set_type(*itTypeRet);
	cheme_printf_unset_zone();
	return result_index;
}

int try_int_term(Term &term) {

	if (term.type == TERM_TYPE_SINGLE  &&
	    term.single.type == TOKEN_TYPE_INT)
	{
		cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
		const Type int_type = build_base_type("int");
		term.metadata.set_type(int_type);
		cheme_printf("%s = %d;\n",
		             try_var_term_var_list_elem_translate
		             (int_type, get_next_temp(), false).c_str(),
		             term.single.num);
		cheme_printf_unset_zone();
		return temp_index - 1;
	}
	else
		return 0;
}

int try_unit_term_unitv_var() {
#define TVTVLAW try_var_term_var_list_elem_translate
	cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
	cheme_printf("%s;\n", TVTVLAW(build_base_type("unit"), get_next_temp(),
	                              false).c_str());
	cheme_printf_unset_zone();
	return temp_index - 1;
#undef TVTVLAW
}

int try_unit_term(Term &term) {
	if (term.is_single_specific_word("unitv")) {
		term.metadata.set_type(build_base_type("unit"));
		return try_unit_term_unitv_var();
	}
	return 0;
}						 

int try_string_term(Term &term) {
	if (term.type == TERM_TYPE_SINGLE  &&
	    term.single.type == TOKEN_TYPE_STRING)
	{
		cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
		const Type type = build_uni_poly_type("ptr", build_base_type("char"));
		term.metadata.set_type(type);
		cheme_printf("%s = %s;\n",
		             try_var_term_var_list_elem_translate
		             (type, get_next_temp(), false).c_str(),
		             term.single.str.c_str());
		cheme_printf_unset_zone();
		return temp_index - 1;
	}
	else
		return 0;
}

int try_char_term(Term &term) {
	if (term.type == TERM_TYPE_SINGLE  &&
	    term.single.type == TOKEN_TYPE_CHAR)
	{
		cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
		const Type type = build_base_type("char");
		term.metadata.set_type(type);
		cheme_printf("%s = %s;\n",
		             try_var_term_var_list_elem_translate
		             (type, get_next_temp(), false).c_str(),
		             term.single.str.c_str());
		cheme_printf_unset_zone();
		return temp_index - 1;
	}
	else
		return 0;
}

int try_set_term(Term &term) {
	if (is_specific_special_form(term, "<-")) {
		ASSERT(term.list.size() == 3, "set special form should get 2 params");
		std::list<Term>::iterator it = term.list.begin();
		Term &ptr = *++it;
		const int ptr_temp_index = handle_term(ptr);
		Term &value = *++it;
		const int value_temp_index = handle_term(value);
		ASSERT(ptr.metadata.get_type().name == "ptr",
		       "Assignment must be done through pointer type");
		Type itype = *ptr.metadata.get_type().type_params.begin();
		ASSERT(same_types(itype, value.metadata.get_type()),
		                  "Assignment violates types");
		cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
#define LEFT_STR (make_temp_name(ptr_temp_index).c_str())
#define RIGHT_STR (make_temp_name(value_temp_index).c_str())
		if (same_types(itype, build_base_type("any")))
			assignment_for_anys(LEFT_STR, RIGHT_STR);
		else
			cheme_printf("*%s = %s;\n", LEFT_STR, RIGHT_STR);
#undef RIGHT_STR
#undef LEFT_STR
		cheme_printf_unset_zone();
		term.metadata.set_type(build_base_type("unit"));
		return try_unit_term_unitv_var();
	}
	return 0;
}

int try_and_term_or_term(Term &term, bool is_or) {
	if (is_specific_special_form(term, is_or ? "or" : "and")) {
		ASSERT(term.list.size() > 2, "and/or"
		       " special form should get at least 2 params");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		const int false_label_index = temp_index++;
//		const int true_label_index = temp_index++;
		const int finish_label_index = temp_index++;
		while (it != term.list.end()) {
			Term &term = *it++;
			const int value_temp_index = handle_term(term);
			ASSERT(same_types(term.metadata.get_type(),
			                  build_base_type("bool")),
			                  "operands to and should be booleans");
			cheme_printf("if (%c!(%s)) goto %s;\n",
			             is_or ? '!' : ' ',
			             make_temp_name(value_temp_index).c_str(),
			             make_temp_name(false_label_index).c_str());
		}
		const std::string result_var = get_next_temp();
#define TVTVLET try_var_term_var_list_elem_translate
		cheme_printf("%s;\n",
		             TVTVLET(build_base_type("bool"), result_var,
		                     false).c_str());
#undef TVTVLET											
		cheme_printf("%s = %ctrue;\ngoto %s;\n", 
		             result_var.c_str(), is_or ? '!' : ' ',
		             make_temp_name(finish_label_index).c_str());
		cheme_printf("%s: %s = %cfalse;\n", 
		             make_temp_name(false_label_index).c_str(),
		             result_var.c_str(), is_or ? '!' : ' ');
		cheme_printf("%s:;\n", make_temp_name(finish_label_index).c_str());
		term.metadata.set_type(build_base_type("bool"));
		return temp_index - 1;
	}
	return 0;
}

int try_and_term(Term &term) { return try_and_term_or_term(term, false); }
int try_or_term (Term &term) { return try_and_term_or_term(term, true);  }

int try_var_ref_term(Term &term) {
	if (term.is_single_word())
	{
		ASSERT(FunctionManager::accessible(term.single.str),
		       "Cannot access variable, or it does not exist");
		const Type &type = FunctionManager::type(term.single.str);
		std::string next_temp = get_next_temp();
		cheme_printf("%s = %s;\n",
		             try_var_term_var_list_elem_translate(type, next_temp,
		                                                  false).c_str(),
		             FunctionManager::true_name(term.single.str).c_str());
		term.metadata.set_type(type);
		return temp_index - 1;
	}
	return 0;
}

void try_while_term_translate_header() {
	cheme_printf("while (1) {\n");
}

void try_while_term_translate_cond(int cond_temp_index) {
	cheme_printf("if (!(%s)) break;\n",
	             make_temp_name(cond_temp_index).c_str());
}

void try_while_term_translate_footer() {
	cheme_printf("}\n");
}

int try_while_term(Term &term) {
	if (is_specific_special_form(term, "while")) {
		ASSERT(term.list.size() >= 2, "while should contain condition");
		std::list<Term>::iterator it1 = term.list.begin(); ++it1;
		std::list<Term>::iterator it2 = term.list.begin(); ++++it2;
		cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
		try_while_term_translate_header();
		FunctionManager::start_scope();
		const int cond_temp_index = handle_term(*it1);
		ASSERT(same_types(it1->metadata.get_type(),
		                  build_base_type("bool")),
		       "While condition should be a boolean");
		try_while_term_translate_cond(cond_temp_index);
		cheme_printf("{\n"); FunctionManager::start_scope();
		std::for_each(it2, term.list.end(), handle_term);
		cheme_printf("}\n"); FunctionManager::end_scope();
		FunctionManager::end_scope();
		try_while_term_translate_footer();
		term.metadata.set_type(build_base_type("unit"));
		cheme_printf_unset_zone();
		return try_unit_term_unitv_var();
	} 
	return 0;
}

int try_if_term(Term &term) {
	if (is_specific_special_form(term, "if")) {
		ASSERT(term.list.size() == 3 || term.list.size() == 4,
		       "bad number of if parameters, should be either "
		       "(if cond true_exp) or (if cond true_exp false_exp)");
		std::list<Term>::iterator it = term.list.begin();
		Term *cond = &*++it;
		Term *true_exp = &*++it;
		++it;
		Term *false_exp = it == term.list.end() ? NULL : &*it;
		const int cond_temp_index = handle_term(*cond);
		ASSERT(same_types(cond->metadata.get_type(),
		                  build_base_type("bool")),
		       "if condition should be a boolean");
		const std::string result_var = get_next_temp();
		const int result_index = temp_index - 1;
		cheme_printf_push();
		{
			cheme_printf("if (%s) {\n",
			             make_temp_name(cond_temp_index).c_str());
			const int true_index = handle_term(*true_exp);
			cheme_printf("%s = %s;\n", result_var.c_str(),
			             make_temp_name(true_index).c_str());
			cheme_printf("}");
			if (false_exp != NULL) {
				cheme_printf(" else {\n");
				const int false_index = handle_term(*false_exp);
				ASSERT2(same_types(false_exp->metadata.get_type(),
				                   true_exp->metadata.get_type()),
				        "if true expression and false expression must "
				        "have same types (current types are: '%s' and '%s'",
				        type_to_string(false_exp->metadata.get_type()).c_str(),
				        type_to_string(true_exp ->metadata.get_type()).c_str());
				cheme_printf("%s = %s;\n", result_var.c_str(),
				             make_temp_name(false_index).c_str());
				cheme_printf("}\n");
			} else {
				ASSERT(same_types(build_base_type("unit"),
				                  true_exp->metadata.get_type()),
				       "if true expression must have unit type");
			}
		}
		std::auto_ptr<char> p(cheme_printf_pop());
#define TVTVLET try_var_term_var_list_elem_translate
		cheme_printf("%s;\n", TVTVLET(true_exp->metadata.get_type(),
		                              result_var, false).c_str());
#undef TVTVLET
		cheme_printf("%s", p.get());
		term.metadata.set_type(true_exp->metadata.get_type());
		return result_index;
	}
	return 0;
}

int try_begin_term(Term &term) {
	if (is_specific_special_form(term, "begin")) {
		ASSERT(term.list.size() > 1,
		       "begin should wrap at least one expression");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		cheme_printf_push();
		FunctionManager::start_scope();
		const int result_index = temp_index++;
		cheme_printf("{\n");
		const int final_value_index = foldl(handle_term_for_fold, 0, it,
		                                    term.list.end());
		cheme_printf("%s = %s;\n",make_temp_name(result_index).c_str(),
		             make_temp_name(final_value_index).c_str());
		FunctionManager::end_scope();
		cheme_printf("}\n");
		std::auto_ptr<char> p(cheme_printf_pop());
		it = term.list.end(); --it;
#define TVTVLET try_var_term_var_list_elem_translate
		cheme_printf("\n%s;\n", TVTVLET(it->metadata.get_type(),
		                                make_temp_name(result_index),
		                                false).c_str());
#undef TVTVLET
		cheme_printf("%s", p.get());
		term.metadata.set_type(it->metadata.get_type());
		return result_index;
	}
	return 0;
}

bool print_lambda_param(bool is_first, const Term &term) {
	ASSERT(term.type == TERM_TYPE_LIST,
	       "lambda parameters must be desxrcibed by lists");
	cheme_printf("%c%s",
	             is_first ? ' ' : ',',
	             try_var_term_var_list_elem_translate
	                 (try_var_term_var_list_elem_type(term.list),
	                  try_var_term_var_list_elem_name(term.list),
	                  false).c_str());
	             
	return false;
}

bool is_var_init_term(Term &term) {
	return (term.index == 2 &&
	        term.parent != NULL &&
	        term.parent->parent != NULL &&
	        is_specific_special_form(*term.parent->parent, "var"));
}

int try_lambda_term_header(Term &term,
                           const std::string &name, const Type &type) {
	const bool is_const_lambda = is_var_init_term(term);
	std::list<Term>::iterator it = term.parent->list.begin();
	
	const int header_index = is_const_lambda ? 0 : temp_index - 1;
	it = term.list.begin(); ++it;
	ASSERT(it->type == TERM_TYPE_LIST, "second term of lambda should be param list");
	cheme_printf("%s(",
	             try_var_term_var_list_elem_translate(type, name.c_str(),
	                                                  false).c_str());
	foldl(print_lambda_param, true, it->list);
	cheme_printf(")\n{\n");
	return header_index;
}

void try_lambda_term_footer() {
	cheme_printf("}\n");
}


Type try_lambda_term_add_param_var(const Term &term) {
	ASSERT(term.type == TERM_TYPE_LIST, "lambda parameters should be described "
	       "by lists, same as variables");
	const Type ret = try_var_term_var_list_elem_type(term.list);
	FunctionManager::add_simple(try_var_term_var_list_elem_name(term.list),
	                            ret);
	return ret;
}

int try_lambda_term(Term &term) {
	if (is_specific_special_form(term, "lambda")) {
		ASSERT(term.list.size() > 2,
		       "lambda should contain parameters and at least one expression");

		const bool is_const_lambda = is_var_init_term(term);
		const bool in_global_scope = FunctionManager::in_global_scope();
		std::string sym_name = is_const_lambda ?
	        try_var_term_var_list_elem_name(term.parent->list) :
		    get_next_temp();
		std::string real_name = !is_const_lambda || in_global_scope ?
		    sym_name : get_next_temp();
		int lambda_index = is_const_lambda ?
		    -(temp_index - 1) : (temp_index - 1);
		std::list<Term>::iterator it = term.list.begin(); ++it;
		FunctionManager::start_scope();
		FunctionManager::start_lambda(is_const_lambda ? real_name : "");
		std::list<Type> type_params = ::map(try_lambda_term_add_param_var,
		                                    it->list, &*type_params.begin());
		Type type;
		if (is_const_lambda) {
			const Type declared_type =
			    try_var_term_var_list_elem_type(term.parent->list);
			std::list<Type>::const_iterator it =
			    declared_type.type_params.end(); --it;
			type_params.push_back(*it);
#define PRM_STR(params) (type_to_string(build_poly_type("func", \
                                                        (params))).c_str())
			ASSERT2(same_type_lists(type_params, declared_type.type_params),
			        "Declared lambda type does not match actual lambda. "
			        "Types are:\nDeclared: %s\nInferred: %s",
			        PRM_STR(declared_type.type_params),
			        PRM_STR(type_params));
#undef PRM_STR
			type = build_poly_type("func", type_params);
			// allow recursion
			FunctionManager::add(sym_name, type, real_name, "", true); 
		}
		cheme_printf_push_body();
		cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
			   
		it = term.list.begin(); ++it; ++it;
		cheme_printf_push();
		ASSERT(it->type == TERM_TYPE_LIST, "second term of lambda should be "
		      "param list");
		const int final_value_index = foldl(handle_term_for_fold, 0, it,
		                                    term.list.end());
		cheme_printf("return %s;\n", make_temp_name(final_value_index).c_str());
		std::auto_ptr<char> body(cheme_printf_pop());
		it = term.list.end(); --it;
		try_lambda_term_header(term, real_name, it->metadata.get_type());
		cheme_printf("%s",body.get());
		FunctionManager::end_scope(); FunctionManager::end_lambda();
		try_lambda_term_footer();
		cheme_printf_unset_zone();
		cheme_printf_pop_body();

		if (!is_const_lambda) { // otherwise, this was already done above
			type_params.push_back(it->metadata.get_type());
			type = build_poly_type("func", type_params);
		} else { // make sure declared return type matches actual return type
			std::list<Type>::iterator it2 = type_params.end(); --it2;
			ASSERT2(same_types(it->metadata.get_type(), *it2),
			        "Declared return type '%s' is not the same as "
			        "inferred return type '%s'",
			        type_to_string(*it2).c_str(),
			        type_to_string(it->metadata.get_type()).c_str());
		}


		term.metadata.set_type(type);
		return lambda_index;
	}
	return 0;
}

int try_any_is_term(Term &term) {
	if (is_specific_special_form(term, "any_is")) {
		ASSERT(term.list.size() == 3,
		       "any_is should accept type an expression of type any");
		std::list<Term>::iterator it = term.list.begin();
		const Type type = type_from_term(*++it);
		const int value_index = handle_term(*++it);
		const int result_index = temp_index++;
		cheme_printf
		    ("bool %s = (%s(&%s) == %s);\n",
		     make_temp_name(result_index).c_str(),
		     HIDDEN_CHEME_ANY_TYPE, make_temp_name(value_index).c_str(),
		     make_type_desc_data_name(TypeInstanceManager::add(type)).c_str());
		term.metadata.set_type(build_base_type("bool"));
		return result_index;
	}
	return 0;
}

void try_to_any_term_assignment(const std::string &any_str,
                                const std::string &value_str,
                                const bool reverse,
                                const int size) {
#define LEFT_AND_RIGHT(left_name, right_name, any_data_accessor)            \
    REVERSE_IF_NEEDED(left_name, right_name,                                \
	                  std::string(any_data_accessor) + "(&" + any_str + ")",\
                      std::string("&") + value_str)                         \

#define REVERSE_IF_NEEDED(left_name, right_name, left_value, right_value)   \
    const std::string left_name  = reverse ? (right_value) : (left_value ); \
    const std::string right_name = reverse ? (left_value ) : (right_value); \

	LEFT_AND_RIGHT(left_dstr, right_dstr, HIDDEN_CHEME_ANY_DPTR)
	LEFT_AND_RIGHT(left_sstr, right_sstr, HIDDEN_CHEME_ANY_SPTR)

	const std::string
	    &left_str = size > ANY_SIZE_THRESHOLD ? left_dstr : left_sstr,
	    &right_str = size > ANY_SIZE_THRESHOLD ? right_dstr : right_sstr;
	cheme_printf("memcpy(%s, %s, %d);\n",
	             left_str.c_str(), right_str.c_str(), size);

#undef REVERSE_IF_NEEDED
#undef LEFT_AND_RIGHT
}

int try_any_to_term(Term &term) {
	if (is_specific_special_form(term, "any_to")) {
		ASSERT(term.list.size() == 3,
		       "any_to should accept a type and an expression");
		std::list<Term>::iterator it = term.list.begin();
		const Type type = type_from_term(*++it);
		const int value_index = handle_term(*++it);
		ASSERT(same_types(it->metadata.get_type(), build_base_type("any")),
		       "Expression given to any_to should be of type 'any'");
		const int result_index = temp_index++;
		const std::string value_str(make_temp_name(value_index));
		const std::string result_str(make_temp_name(result_index));
		const std::string type_desc_data_str = make_type_desc_data_name
		    (TypeInstanceManager::add(type));

		cheme_printf("if (%s(&%s) != %s) {\n"
		             "  printf(\"dynamic type mismatch at %s, \"\n"
		             "         \"inferred %%s, expected %%s\\n\",\n"
		             "         %s(&%s), type_desc_name(%s));\n"
		             "  abort();\n"
		             "}\n",
		             HIDDEN_CHEME_ANY_TYPE, value_str.c_str(),
		             type_desc_data_str.c_str(),
		             error_context_to_string().c_str(),
		             HIDDEN_CHEME_ANY_TYPE_NAME, value_str.c_str(),
		             type_desc_data_str.c_str());
		cheme_printf("%s;\n",
		             try_var_term_var_list_elem_translate(type, result_str,
					                                      false).c_str());
		try_to_any_term_assignment(value_str, result_str, true,
		                           type_info(type).size);
		term.metadata.set_type(type);
		return result_index;
	}
	return 0;
}

int try_to_any_term_raw(Term &term) {
	const int value_index = handle_term(term);
	const std::string value_str(make_temp_name(value_index));
	const Type &type = term.metadata.get_type();
	const int size = type_info(type).size;
	const int result_index = temp_index++;
	const std::string result_str(make_temp_name(result_index));
	const std::string type_desc_data_str = make_type_desc_data_name
	    (TypeInstanceManager::add(type));
	ASSERT(!same_types(type, build_base_type("any")),
	       "to_any cannot be called on value of type any");
	cheme_printf("any %s; *%s(&%s) = %s;\n",
	             result_str.c_str(),
	             HIDDEN_CHEME_ANY_TYPE_PTR,
	             result_str.c_str(), type_desc_data_str.c_str());
	if (size > ANY_SIZE_THRESHOLD)
		cheme_printf("*%s(&%s) = malloc(%d);\n",
		             HIDDEN_CHEME_ANY_DPTR_PTR, result_str.c_str(), size);
	try_to_any_term_assignment(result_str, value_str, false,
	                           type_info(term.metadata.get_type()).size);
	return result_index;
}

int try_to_any_term(Term &term) {
	if (is_specific_special_form(term, "to_any")) {
		ASSERT(term.list.size() == 2,
		       "to_any should accept a single expression");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		term.metadata.set_type(build_base_type("any"));
		return try_to_any_term_raw(*it);
	}
	return 0;
}

int try_ref_term(Term &term) {
	if (is_specific_special_form(term, "ref")) {
		ASSERT(term.list.size() == 2,
		       "ref should accept only a single parameter which is a variable");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		std::string str = it->single.str;
		ASSERT(it->is_single_word(), "ref second parameter should be a word");
		ASSERT(FunctionManager::accessible(str),
		       "Cannot access variable or it does not exist");
		Type type = FunctionManager::type(str);
		Type ptype = build_uni_poly_type("ptr", type);
		const int result_index = temp_index++;
#define TVTVLET try_var_term_var_list_elem_translate
		cheme_printf("%s = &%s;\n",
		             TVTVLET(ptype, make_temp_name(result_index).c_str(),
	                         false).c_str(),
		             FunctionManager::true_name(str).c_str());
#undef TVTVLET
		term.metadata.set_type(ptype);
		return result_index;

		
	}
	return 0;
}

int try_ind_term(Term &term) {
	if (is_specific_special_form(term, "ind")) {
		ASSERT(term.list.size() == 2,
		       "ind should accept only a single parameter which is a pointer "
		       "expression");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		const int value_index = handle_term(*it);
		const Type &type = it->metadata.get_type();
		ASSERT(type.name == "ptr",
		       "ind second parameter type should be a pointer type");
		Type itype = *type.type_params.begin();
		const int result_index = temp_index++;
#define TVTVLET try_var_term_var_list_elem_translate
		cheme_printf("%s = *%s;\n",
		             TVTVLET(itype, make_temp_name(result_index).c_str(),
	                         false).c_str(),
		             make_temp_name(value_index).c_str());
#undef TVTVLET
		term.metadata.set_type(itype);
		return result_index;
	}
	return 0;
}

int try_type_pack_term(Term &term) {
	if (is_specific_special_form(term, "type_pack")) {
		ASSERT(term.list.size() == 2 || term.list.size() == 3,
		       "type_pack should accept one or two parameters");
		bool is_def = term.list.size() == 3;
		std::list<Term>::iterator it = term.list.begin();
		++it;
		if (is_def) ++it;
		ASSERT(it->is_single_word(),
		       "type_pack second parameter should be a word");
		std::string name = it->single.str; --it;
		TypeManager::add_base_type(name, get_packed_type_info, strdup(name.c_str()));
		TypePackManager::add(name);
		if (is_def) {
			Type type = type_from_term(*it);
			TypePackManager::add(name, type);
		}
		TypeInstanceManager::add(build_base_type(name));
		// cheme_printf_set_zone(FunctionManager::in_global_scope() ?
		//                       PRINTF_ZONE_TOP : PRINTF_ZONE_CURRENT_BODY);
		// cheme_printf("struct %s { %s; };\n",
		//              name.c_str(),
		//              try_var_term_var_list_elem_translate(type, "data",
		//                                                   false).c_str());
		// cheme_printf_unset_zone();
		term.metadata.set_type(build_base_type("unit"));
		return temp_index++;
	}
	return 0;
}

int try_pack_term(Term &term) {
	if (is_specific_special_form(term, "pack")) {
		ASSERT(term.list.size() == 3, "pack should accept two parameters");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		ASSERT(it->is_single_word(),
		       "pack first parameter should be a word");
		std::string name = it->single.str; ++it;
		ASSERT(TypePackManager::is_defined_type_pack(name),
		       "pack first parameter should be the name of a type pack");
		const Type t = build_base_type(name);
		int value_index = handle_term(*it);
		ASSERT2(same_types(it->metadata.get_type(),
		        TypePackManager::unpack(name)),
		        "type mismatch in packing, inferred '%s', expected '%s'",
			    type_to_string(it->metadata.get_type()).c_str(),
		        type_to_string(TypePackManager::unpack(name)).c_str());
		int result_index = temp_index++;
		std::string value_str = make_temp_name(value_index);
		std::string result_str = make_temp_name(result_index);
		cheme_printf("%s = {%s};\n",
		             try_var_term_var_list_elem_translate(t, result_str,
		                                                  false).c_str(),
		             value_str.c_str());
		term.metadata.set_type(t);
		return result_index;
	}
	return 0;
}

int try_unpack_term(Term &term) {
	if (is_specific_special_form(term, "unpack")) {
		ASSERT(term.list.size() == 2,
		       "unpack should accept a single parameter");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		int value_index = handle_term(*it);
		int result_index = temp_index++;
		std::string value_str = make_temp_name(value_index);
		std::string result_str = make_temp_name(result_index);
		Type t = it->metadata.get_type();
		Type t2 = TypePackManager::unpack(t.name);
		ASSERT(t.type_params.empty(),
		       "unpack term should not be parametric");
		std::string s = try_var_term_var_list_elem_translate(t2, result_str,
		                                                     false);
		cheme_printf("%s = %s.data;\n", s.c_str(), value_str.c_str());
		term.metadata.set_type(t2);
		return result_index;
	}
	return 0;
}

int try_at_term(Term &term) {
	if (is_specific_special_form(term, "at")) {
		ASSERT(term.list.size() == 3, "at should accept 2 parameters");
		std::list<Term>::iterator it = term.list.begin(); 
		int value_index = handle_term(*++it);
		Type value_type = it->metadata.get_type();
		ASSERT1(value_type.name == "tup" && value_type.type_params.size() > 0,
		        "1st parameter of at should be (tup ...), not %s",
		        type_to_string(value_type).c_str());
		ASSERT((++it)->is_single_int(),
		       "2nd parameter of at must be an int literal");
		int index = it->single.num;
		ASSERT1(index < (int)value_type.type_params.size(),
		        "at index is too big (should be less than %d)",
		        (int)value_type.type_params.size());
		ASSERT(index >= 0, "at index should be non-negative");
		Type result_type = list_at(value_type.type_params, index);
		int result_index = temp_index++;
		std::string s = try_var_term_var_list_elem_translate
		    (result_type, make_temp_name(result_index).c_str(), false);
		cheme_printf("%s = %s.elem%d;\n",
		             s.c_str(), make_temp_name(value_index).c_str(), index);
		term.metadata.set_type(result_type);
		return result_index;
	}
	return 0;
}

bool print_value_index_with_comma(bool is_first, const int value_index) {
	cheme_printf("%c%s",
	             is_first ? ' ' : ',', make_temp_name(value_index).c_str());
	return false;
}

int try_tupv_term(Term &term) {
	if (is_specific_special_form(term, "tupv")) {
		ASSERT(term.list.size() > 1, "tupv without parameters");
		std::list<Term>::iterator it = term.list.begin();
		std::list<int> value_indices;
		std::list<Type> types;
		while (++it != term.list.end()) {
			value_indices.push_back(handle_term(*it));
			types.push_back(it->metadata.get_type());
		}
		int result_index = temp_index++;
		Type result_type = build_poly_type("tup", types);
		std::string s = try_var_term_var_list_elem_translate
		    (result_type, make_temp_name(result_index).c_str(), false);
		cheme_printf("%s = {", s.c_str());
		foldl(print_value_index_with_comma, true, value_indices);
		cheme_printf("};");
		term.metadata.set_type(result_type);
		return result_index;
	}
	return 0;
}

int try_ptr_add_term(Term &term) {
	if (is_specific_special_form(term, "ptr_add")) {
		ASSERT(term.list.size() == 3, "ptr_add should get two parameters");
		std::list<Term>::iterator it = term.list.begin();
		int value_index = handle_term(*++it);
		Type type = it->metadata.get_type();
		ASSERT(type.name == "ptr", "ptr_add 1st parameter must be a pointer");
		int offset_index = handle_term(*++it);
		ASSERT(same_types(it->metadata.get_type(), build_base_type("int")),
		       "ptr_add 2nd parameter must be an int");
		int result_index = temp_index++;	   
		std::string s = try_var_term_var_list_elem_translate
		    (type, make_temp_name(result_index), false);
		cheme_printf("%s = %s + %s;", s.c_str(),
		             make_temp_name(value_index).c_str(),
		             make_temp_name(offset_index).c_str());
		term.metadata.set_type(type);
		return result_index;
	}
	return 0;
}

int try_is_null_term(Term &term) {
	if (is_specific_special_form(term, "is_null")) {
		ASSERT(term.list.size() == 2, "is_null should get one parameter");
		std::list<Term>::iterator it = term.list.begin();
		int value_index = handle_term(*++it);
		Type type = it->metadata.get_type();
		ASSERT(type.name == "ptr", "is_null 1st parameter must be a pointer");
		int result_index = temp_index++;
		std::string s = try_var_term_var_list_elem_translate
		    (build_base_type("bool"), make_temp_name(result_index), false);
		cheme_printf("%s = (%s == NULL);\n",
		             s.c_str(), make_temp_name(value_index).c_str());
		term.metadata.set_type(build_base_type("bool"));
		return result_index;
	}
	return 0;
}

int try_new_term(Term &term) {
	if (is_specific_special_form(term, "new")) {
		ASSERT(term.list.size() == 2, "new should get one parameter");
		std::list<Term>::iterator it = term.list.begin();
		Type type = type_from_term(*++it);
		int size = type_info(type).size;
		int result_index = temp_index++;
		Type result_type = build_uni_poly_type("ptr", type);
		std::string s = try_var_term_var_list_elem_translate
		    (result_type, make_temp_name(result_index), false);
		cheme_printf("%s = malloc(%d);\n", s.c_str(), size);
		term.metadata.set_type(result_type);
		return result_index;
	}
	return 0;
}

int try_quote_term(Term &term) {
	if (is_specific_special_form(term, "quote")) {
		ASSERT(term.list.size() == 2, "quote should get one parameter");
		int result_index = temp_index++;
		std::string s = try_var_term_var_list_elem_translate
		    (build_base_type("any"), make_temp_name(result_index), false);
		std::list<Term>::iterator it = term.list.begin(); ++it;
		cheme_printf("%s = read_term_from_str(\"%s\");\n",
		             s.c_str(), term_to_string(*it).c_str());
		term.metadata.set_type(build_base_type("any"));
		return result_index;
	}
	return 0;
}

int handle_term(Term &term) {
	Maybe<Type> type;
	int used_temp_index = 0;
#define BEGIN_TRY() if (false) {}
#define TRY(func) else if ((used_temp_index = func(term))) {}
#define ELSE else

	push_error_context(term.ec);
	BEGIN_TRY()
	TRY(try_var_term)
	TRY(try_decl_term)
	TRY(try_while_term)
	TRY(try_set_term)
	TRY(try_and_term)
	TRY(try_or_term)
	TRY(try_if_term)
	TRY(try_begin_term)
	TRY(try_lambda_term)
	TRY(try_ref_term)
	TRY(try_ind_term)
	TRY(try_type_pack_term)
	TRY(try_pack_term)
	TRY(try_unpack_term)
	TRY(try_int_term)
	TRY(try_unit_term)
	TRY(try_string_term)
	TRY(try_char_term)	
	TRY(try_any_is_term)
	TRY(try_any_to_term)
	TRY(try_to_any_term)
	TRY(try_at_term)
	TRY(try_tupv_term)
	TRY(try_ptr_add_term)
	TRY(try_is_null_term)
	TRY(try_new_term)
	TRY(try_quote_term)
	TRY(try_app_term)
	TRY(try_var_ref_term)
	ELSE
	{
		std::string str;
		if (term.type == TERM_TYPE_LIST) {
			const Term &term2 = *term.list.begin();
			if (term2.is_single_word()) {
				str = "(" + term2.single.str + " ...)";
			} else {
				str = "(<NON-WORD-TERM> ...)";
			}
		} else {
			str = "<NON-LIST-TERM>";
		}
		ABORT1("Expected special form or application form of a known callable, "
		       "instead got term %s\n", str.c_str());
	}
	ASSERT(term.metadata.has_type(), "term without type???");
	pop_error_context();
	return used_temp_index;
}

#undef printf
void print_type_desc_extern(const std::pair<Type, int> &pair) {
	// skip opaque
	if (TypePackManager::is_opaque_type_pack(pair.first.name)) return;
//	printf("hidden_cheme_type_desc_data %s_data = {%d,\"%s\"};\n",
//	       make_type_desc_data_name(pair.second).c_str(),
//	       type_info(pair.first).size,
//	       type_to_string(pair.first).c_str());
	printf("extern hidden_cheme_type_desc_data %s_data;\n",
	       make_type_desc_data_name(pair.second).c_str());
	printf("hidden_cheme_type_desc_data * const %s = &%s_data;\n",
	       make_type_desc_data_name(pair.second).c_str(),
	       make_type_desc_data_name(pair.second).c_str());
	       
}

void print_type_desc_data(FILE *f, const std::pair<Type, int> &pair) {
	if (TypePackManager::is_opaque_type_pack(pair.first.name)) return;
	char buf[32];
	putc('(', f);
	fputs(type_to_string(pair.first).c_str(), f);
	putc(' ', f);
	sprintf(buf, "%d", pair.second); fputs(buf, f);
	putc(' ', f);
	sprintf(buf, "%d", type_info(pair.first).size); fputs(buf, f);
	putc(')', f);
	putc('\n', f);
}

void print_tup_or_type_pack(const Type &t, bool *printed) {
	int id = -1;
	if (t.name == "tup") {
		if (printed[id = TypeInstanceManager::id(t)]) return;
		std::list<Type>::const_iterator it2 = t.type_params.begin();
		for (; it2 != t.type_params.end(); ++it2) {
			print_tup_or_type_pack(*it2, printed);
		}
		printf("%s {\n", make_tup_name(id).c_str());
		int index = 0;
		it2 = t.type_params.begin();
		for (; it2 != t.type_params.end(); ++it2, ++index) {
			char buf[32];
			sprintf(buf, "elem%d", index);
			printf("%s;\n",
			       try_var_term_var_list_elem_translate(*it2, buf,
			                                            false).c_str());
		}
		printf("};\n");
	} else if (TypePackManager::is_defined_type_pack(t.name)) {
		if (printed[id = TypeInstanceManager::id(t)]) return;
		Type type = TypePackManager::unpack(t.name);
		print_tup_or_type_pack(type, printed);
		printf("struct %s { %s; };\n",
		       t.name.c_str(),
		       try_var_term_var_list_elem_translate(type, "data",
		                                            false).c_str());
	}
	if (id >= 0) printed[id] = true;
}

void print_tup_structs_and_type_packs_worker(bool *printed) {
	std::list< std::pair<Type, int> > list = TypeInstanceManager::all();
	std::list< std::pair<Type, int> >::iterator it = list.begin();
	for (; it != list.end(); ++it) {
		print_tup_or_type_pack(it->first, printed);
	}
}

void print_tup_structs_and_type_packs() {
	bool *p = new bool[TypeInstanceManager::count()];
	for (int i = TypeInstanceManager::count(); i > 0; --i) p[i - 1] = false;
	print_tup_structs_and_type_packs_worker(p);
}

void print_type_descs() {
	std::list< std::pair<Type, int> > desc_pairs = TypeInstanceManager::all();
	std::list< std::pair<Type, int> >::iterator it = desc_pairs.begin();
	std::for_each(desc_pairs.begin(), desc_pairs.end(),
	              print_type_desc_extern);
	FILE *f;
	f = fopen("types", "w");
	for (it = desc_pairs.begin(); it != desc_pairs.end(); ++it) {
		print_type_desc_data(f, *it);
	}
	fclose(f);
}


void print_header_stuff() {
	puts("typedef struct { int size; char *name; } "
	     "hidden_cheme_type_desc_data;");
	puts("typedef hidden_cheme_type_desc_data *type_desc;");
	puts(CHEME_TYPE_BOOL_STR);
	puts(CHEME_TYPE_UNIT_STR);
	puts("static unit unitv() { unit v; return v; }");
	puts(CHEME_TYPE_ANY_STR);
	puts(CHEME_TYPE_SYM_STR);
	puts("static hidden_cheme_type_desc_data**"
	     HIDDEN_CHEME_ANY_TYPE_PTR"(any *a) "
	     "{return &a->t;}");
	puts("static hidden_cheme_type_desc_data *"
	     HIDDEN_CHEME_ANY_TYPE    "(any *a) "
	     "{return  a->t;}");
	puts("static char **    "HIDDEN_CHEME_ANY_DPTR_PTR "(any *a) "
	     "{return &a->d;}");
	puts("static char *     "HIDDEN_CHEME_ANY_SPTR     "(any *a) "
	     "{return a->s;}");
	puts("static char *     "HIDDEN_CHEME_ANY_DPTR     "(any *a) "
	     "{return a->d;}");
	
	puts("static int        "HIDDEN_CHEME_ANY_SIZE     "(any *a) "
	     "{return a->t->size;}");
	puts("static char *     "HIDDEN_CHEME_ANY_TYPE_NAME"(any *a) "
	     "{return a->t->name;}");
	puts("static char *type_desc_name(type_desc t) "
	     "{return t->name;}");
	puts("static int "HIDDEN_CHEME_LT"(int x, int y) "
	     "{ return x < y; }");
	puts("static int "HIDDEN_CHEME_GT"(int x, int y) "
	     "{ return x > y; }");
	puts("static int "HIDDEN_CHEME_EQ"(int x, int y) "
	     "{ return x == y; }");
	puts("static int mod(int x, int y) { return x % y; }");
	puts("static int div(int x, int y) { return x / y; }");
	puts("static int "HIDDEN_CHEME_ADD_INTS"(int x, int y) "
	     "{ return x + y; }");
	puts("static int "HIDDEN_CHEME_SUB_INTS"(int x, int y) "
	     "{ return x - y; }");
	puts("static int "HIDDEN_CHEME_MUL_INTS"(int x, int y) "
	     "{ return x * y; }");
	puts("static int "HIDDEN_CHEME_CMP_INTS"(int x, int y) "
	     "{ return x == y; }");
	puts("static bool any_eq(any *a, any *b) "
	     "{ if ("HIDDEN_CHEME_ANY_TYPE"(a) != "HIDDEN_CHEME_ANY_TYPE"(b))\n"
	     "    return false;\n"
	     "  if ("HIDDEN_CHEME_ANY_SIZE"(a) <= "ANY_SIZE_THRESHOLD_STR")\n"
	     "    return memcmp("HIDDEN_CHEME_ANY_SPTR"(a),\n"
	     "                  "HIDDEN_CHEME_ANY_SPTR"(b)) == 0;\n"
	     "}\n");
	puts("static bool char_eq(char a, char b) { return a == b; }");
	puts("static bool type_desc_eq(type_desc a, type_desc b) { return a == b; }");
	puts("static bool str_eq(char *a, char *b) { return strcmp(a,b) == 0; }");
	puts("static double int2double(int x) { return (double)x; }");
	puts("static int double2int(double x) { return (int)x; }");
	puts("static int char2int(char c) { return c; }");
	print_type_descs();
    TypeInstanceManager::add(TypePackManager::unpack("anylist"));
	print_tup_structs_and_type_packs();
	// printf("struct anylist { %s data; }; ",
	//        make_tup_name(anylist_data_id).c_str());
	puts("void abort(void); void *malloc(long size); void free(void *ptr);\n");
	puts("void *memcpy(void *dest, const void *src, long n);");
	puts("int printf(const char *, ...);");
	puts("any read_term_from_str(char *);");
	puts("char *sym_str(sym s) { return s.str; }");
	puts("bool sym_eq(sym s, sym t) { return !strcmp(s.str, t.str); }");
	puts("#define NULL 0");
}

#define printf DO NOT USE THIS

void print_main_header_stuff() {
	cheme_printf("int main() {\n");
}

void print_footer_stuff() {
	cheme_printf("}\n");
}

void add_primitives() {
	std::list<Type> iib;
	iib.push_back(build_base_type("int"));
	iib.push_back(build_base_type("int"));
	iib.push_back(build_base_type("bool"));
	FunctionManager::add("==", build_poly_type("func", iib),
	                     HIDDEN_CHEME_CMP_INTS, "", true);
	FunctionManager::add("<", build_poly_type("func", iib),
	                     HIDDEN_CHEME_LT, "", true);
	FunctionManager::add(">", build_poly_type("func", iib),
	                     HIDDEN_CHEME_GT, "", true);
	std::list<Type> iii;
	iii.push_back(build_base_type("int"));
	iii.push_back(build_base_type("int"));
	iii.push_back(build_base_type("int"));
	FunctionManager::add_simple("mod", build_poly_type("func", iii));
	FunctionManager::add_simple("div", build_poly_type("func", iii));
	FunctionManager::add("+", build_poly_type("func", iii),
	                     HIDDEN_CHEME_ADD_INTS, "", true);
	FunctionManager::add("-", build_poly_type("func", iii),
	                     HIDDEN_CHEME_SUB_INTS, "", true);
	FunctionManager::add("*", build_poly_type("func", iii),
	                     HIDDEN_CHEME_MUL_INTS, "", true);
	std::list<Type> id;
	id.push_back(build_base_type("int"));
	id.push_back(build_base_type("double"));
	FunctionManager::add_simple("int2double", build_poly_type("func", id));
	std::list<Type> di;
	di.push_back(build_base_type("double"));
	di.push_back(build_base_type("int"));
	FunctionManager::add_simple("double2int", build_poly_type("func", di));
	std::list<Type> ci;
	ci.push_back(build_base_type("char"));
	ci.push_back(build_base_type("int"));
	FunctionManager::add_simple("char2int", build_poly_type("func", ci));
	std::list<Type> ccb;
	ccb.push_back(build_base_type("char"));
	ccb.push_back(build_base_type("char"));
	ccb.push_back(build_base_type("bool"));
	FunctionManager::add_simple("char_eq", build_poly_type("func", ccb));
	std::list<Type> ttb;
	ttb.push_back(build_base_type("type_desc"));
	ttb.push_back(build_base_type("type_desc"));
	ttb.push_back(build_base_type("bool"));	
	FunctionManager::add_simple("type_desc_eq", build_poly_type("func", ttb));
	std::list<Type> ts;
	ts.push_back(build_base_type("type_desc"));
	ts.push_back(build_uni_poly_type("ptr", build_base_type("char")));
	FunctionManager::add_simple("type_desc_name", build_poly_type("func", ts));
	std::list<Type> at;
	at.push_back(build_uni_poly_type("ptr", build_base_type("any")));
	at.push_back(build_base_type("type_desc"));
	FunctionManager::add_simple("any_type", build_poly_type("func", at));
	std::list<Type> ssb;
	ssb.push_back(build_uni_poly_type("ptr", build_base_type("char")));
	ssb.push_back(build_uni_poly_type("ptr", build_base_type("char")));
	ssb.push_back(build_base_type("bool"));
	FunctionManager::add_simple("str_eq", build_poly_type("func", ssb));
	std::list<Type> aab;
	aab.push_back(build_uni_poly_type("ptr", build_base_type("any")));
	aab.push_back(build_uni_poly_type("ptr", build_base_type("any")));
	aab.push_back(build_base_type("bool"));
	FunctionManager::add_simple("any_eq", build_poly_type("func", aab));
	
	FunctionManager::add_simple("true", build_base_type("bool"));
	FunctionManager::add_simple("false", build_base_type("bool"));
	std::list<Type> sy_str;
	sy_str.push_back(build_base_type("sym"));
	sy_str.push_back(build_uni_poly_type("ptr", build_base_type("char")));
	FunctionManager::add_simple("sym_str", build_poly_type("func", sy_str));
	std::list<Type> sy_sy_bool;
	sy_sy_bool.push_back(build_base_type("sym"));
	sy_sy_bool.push_back(build_base_type("sym"));
	sy_sy_bool.push_back(build_base_type("bool"));
	FunctionManager::add_simple("sym_eq", build_poly_type("func", sy_sy_bool));
}

#ifdef MAIN

int compile(int argc, char **argv) {
	argc == 1 ||
    (argc == 5 && !strcmp(argv[1], "--index") &&
	 !strcmp(argv[3], "--first_layer")) ||
	    (fprintf(stderr, "Very limited usage\n"), abort(), false);
	if (argc == 5) {
		is_first_and_last_layer = false;
		indices_file = argv[2];
		first_layer_source = argv[4];
	}
	
	TypeManager::init();
	TypeInstanceManager::init();
	init_parsing(stdin);
	std::list<Term> all_terms;
	cheme_printf_push_body();
	cheme_printf_top_zone_stack.push(cheme_printf_context());
	FunctionManager::start_scope();
	add_primitives();
	
	for (int i = 0; peep_token().type != TOKEN_TYPE_EOF; ++i)
		all_terms.push_back(read_term(i));

	cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
	print_main_header_stuff();
	std::for_each(all_terms.begin(), all_terms.end(), set_term_parent);
	std::for_each(all_terms.begin(), all_terms.end(), handle_term);
	FunctionManager::end_scope();
	print_footer_stuff();
	cheme_printf_unset_zone();
	
	print_header_stuff();
	cheme_printf_set_zone(PRINTF_ZONE_TOP);
	std::auto_ptr<char> pre_main_text(cheme_printf_pop());
	puts(pre_main_text.get());
	cheme_printf_unset_zone();
	cheme_printf_pop_body();
	
	cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
	while (!cheme_printf_done_body_zones.empty()) {
		puts(cheme_printf_done_body_zones.front());
		cheme_printf_done_body_zones.pop();
	}
	cheme_printf_unset_zone();
	ASSERT(cheme_printf_zone_stack.empty(),
	       "cheme_printf_zone_stack NOT empty at end");
	ASSERT(cheme_printf_body_zones_stack.empty(),
	       "cheme_printf_body_zones_stack NOT empty at end");
	ASSERT(cheme_printf_top_zone_stack.empty(),
	       "cheme_printf_top_zone_stack NOT empty at end");
	return 0;
}

bool merge_types_not_all_at_end
    (const std::list< std::list<Term> > &lol,
     const std::list< std::list<Term>::const_iterator > &its)
{
	ASSERT2(lol.size() == its.size(),
	        "merge_types_not_all_at_end: sizes unequal (%d, %d)",
	        lol.size(), its.size());
	std::list< std::list<Term> >::const_iterator itl = lol.begin();
	std::list<std::list<Term>::const_iterator>::const_iterator it = its.begin();
	for (; it != its.end(); ++itl, ++it)
		if (*it != itl->end())
			return true;
	return false;
}

#undef printf
int merge_types(int argc, char **argv) {
	std::list< std::list<Term> > lol;
	std::list<FILE *> replace_tables;
	for (int j = 1; j < argc; ++j) {
		char buf[256];
		sprintf(buf, "replace_table_%08d", j - 1);
		replace_tables.push_back(fopen(buf, "w"));
	}
		
	for (int j = 1; j < argc; ++j) {
		lol.push_back(std::list<Term>());
		FILE *f = fopen(argv[j], "r");
		init_parsing(f);
		for (int i = 0; peep_token().type != TOKEN_TYPE_EOF; ++i) {
			lol.back().push_back(read_term(i));
			fprintf(stderr, "Added %s to list %d\n", term_to_string(lol.back().back()).c_str(), j);
		}
		fclose(f);
	}
	
	int non_1st_lst_ind = 0; // index to be used for types not appearing
	                         // in lol.begin(), initialied to max id of
	                         // lol.begin() plus one.
	for (std::list<Term>::const_iterator it = lol.begin()->begin();
	     it != lol.begin()->end(); ++it)
	{
		const Term &t1 = *it;
		ASSERT1(t1.type == TERM_TYPE_LIST && t1.list.size() >= 2 &&
		        list_at(t1.list, 1).is_single_int(),
		        "Invalid term %s in first types file",
		       term_to_string(t1).c_str());
		int id = list_at(t1.list, 1).single.num;
		if (non_1st_lst_ind < id) non_1st_lst_ind = id;
	}
	++non_1st_lst_ind;
		 
	     
	std::list< std::list<Term>::const_iterator > it_lst =
	    ::map(begin_func< std::list<Term> > , lol, &it_lst.back());
	while (merge_types_not_all_at_end(lol, it_lst)) {
		std::list<std::list<Term>::const_iterator>::iterator it2;
		std::list<std::list<std::list<Term>::const_iterator>::iterator> it2_lst;
		std::list<FILE *> it3_tbls_lst;
		std::list< std::list<Term> >::const_iterator itl = lol.begin();
		fprintf(stderr, "Comparing...\n");
		for (it2 = it_lst.begin(); it2 != it_lst.end(); ++it2, ++itl) {
			if (itl->end() == *it2) continue;
			const Term &t1 = **it2;
			ASSERT1(t1.type == TERM_TYPE_LIST && t1.list.size() == 3 &&
			        list_at(t1.list, 1).is_single_int() &&
			        list_at(t1.list, 2).is_single_int(),
			        "Invalid term %s", term_to_string(t1).c_str());
		}
		std::list<FILE *>::iterator tbls_it = replace_tables.begin();
		for (it2 = it_lst.begin(), itl = lol.begin(); it2 != it_lst.end();
		     ++tbls_it, ++it2, ++itl)
		{
			if (itl->end() == *it2) continue;
			fprintf(stderr, "Going over %s...\n", term_to_string(**it2).c_str());
			if (it2_lst.empty()) {
				it2_lst.push_back(it2);
				it3_tbls_lst.push_back(*tbls_it);
			} else {
				const Term &t1 = **it2, &t2 = ***it2_lst.begin();
				int cmp = term_cmp(*t1.list.begin(), *t2.list.begin());
				if (cmp <= 0) {
					if (cmp < 0) { it2_lst.clear(); it3_tbls_lst.clear(); }
					it2_lst.push_back(it2); it3_tbls_lst.push_back(*tbls_it);
				}
			}
		}
		const bool type_in_1st_lst = *it2_lst.begin() == it_lst.begin();
		fprintf(stderr, "Picked %s, type %s in first list\n",
		        term_to_string(***it2_lst.begin()).c_str(),
		        type_in_1st_lst ? "is" : "is not");
		int unified_type_index = type_in_1st_lst
		    ? list_at((***it2_lst.begin()).list, 1).single.num
		    : non_1st_lst_ind++;
		printf("hidden_cheme_type_desc_data %s_data = {%d,\"%s\"};\n",
		       make_type_desc_data_name(unified_type_index).c_str(),
		       list_at((***it2_lst.begin()).list, 2).single.num,
		       term_to_string(list_at((***it2_lst.begin()).list, 0)).c_str());
		std::list<std::list<
		  std::list<Term>::const_iterator >::iterator>::iterator it3 =
		    it2_lst.begin();
		tbls_it = it3_tbls_lst.begin();
		for (++it3, ++tbls_it ; tbls_it != it3_tbls_lst.end(); ++tbls_it) {
			int type_index = list_at((***it3).list, 1).single.num;
			if (type_index != unified_type_index)
				fprintf(*tbls_it, "(%d %d)", type_index, unified_type_index);
		}
		for (it3 = it2_lst.begin(); it3 != it2_lst.end(); ++it3)
			++(**it3);
	}
	return 0;
}

std::map<int, int> build_subst_map(char *filename) {
	FILE *f = fopen(filename, "r");
	std::map<int, int> ret;
	init_parsing(f);
	while (peep_token().type != TOKEN_TYPE_EOF) {
		Term term = read_term(0);
		Term first, second;
		ASSERT1(term.type == TERM_TYPE_LIST &&
		        term.list.size() == 2 &&
		        (first = list_at(term.list, 0)).is_single_int() &&
		        (second = list_at(term.list, 1)).is_single_int(),
		        "Malformed termed '%s' in substitution table",
		        term_to_string(term).c_str());
		ret[first.single.num] = second.single.num;
	}
	return ret;
}

int patch_obj_types(int argc, char **argv) {
	argc == 3 ||
	    (fprintf(stderr, "Very limited usage\n"), abort(), false);
	FILE *obj_file = fopen(argv[1], "r+b");
	std::map<int, int> subst_map = build_subst_map(argv[2]);
	const int prefix_len = sizeof(CHEME_TYPE_DESC_VAR_PREFIX);
	const int len = (int)make_type_desc_data_name(0).size();
	ASSERT(len - prefix_len == TYPE_DESC_NUM_LEN,
	       "patch_obj_types: length const mismatch");
	std::auto_ptr<char> ap(new char[len]);
	char *p0 = ap.get();
	char * const plim = p0 + len;
	fread(p0, len, 1, obj_file);
#define CYCP(O) ( (p0 + O >= plim) ? (p0 + O + (p0 - plim)) : (p0 + O) )
	while (1) {
		if (memcmp(CHEME_TYPE_DESC_VAR_PREFIX, p0, prefix_len) == 0) {
			int num; sscanf(CYCP(prefix_len), "%d", &num);
			int new_num;
			if (subst_map.find(num) != subst_map.end()) {
				new_num = subst_map[num];
				sprintf(CYCP(prefix_len),
				        "%0"TYPE_DESC_NUM_LEN_STR"d", new_num);
			}
		}
		fseek(obj_file, -len, SEEK_CUR);
		fputc(*p0, obj_file);
		int c = fgetc(obj_file);
		if (c == EOF)
			break;
		*p0 = c;
	}
#undef CYCP
	return 0;
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		fprintf(stderr, "Missing command\n");
		exit(1);
	}
	if (strcmp(argv[1], "compile") == 0)
		return compile(argc - 1, argv + 1);
	else if (strcmp(argv[1], "merge_types") == 0)
		return merge_types(argc - 1, argv + 1);
	else if (strcmp(argv[1], "patch_obj_types") == 0)
		return patch_obj_types(argc - 1, argv + 1);
	else {
		fprintf(stderr, "Unknown command %s\n", argv[1]);
		exit(1);
	}
	
}
#endif
