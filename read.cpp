
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

#define HIDDEN_CHEME_LT            "hidden_cheme_less_than"
#define HIDDEN_CHEME_GT            "hidden_cheme_greater_than"
#define HIDDEN_CHEME_EQ            "hidden_cheme_equals"
#define HIDDEN_CHEME_INT_TO_DOUBLE "hidden_cheme_int_to_double"
#define HIDDEN_CHEME_DOUBLE_TO_INT "hidden_cheme_double_to_int"
#define HIDDEN_CHEME_ADD_INTS      "hidden_cheme_add_ints"
#define HIDDEN_CHEME_SUB_INTS      "hidden_cheme_sub_ints"
#define HIDDEN_CHEME_CMP_INTS      "hidden_cheme_cmp_ints"
#define HIDDEN_CHEME_ANY_SIZE      "hidden_cheme_any_size"
#define HIDDEN_CHEME_ANY_DPTR_PTR  "hidden_cheme_any_dptr_ptr"
#define HIDDEN_CHEME_ANY_DPTR      "hidden_cheme_any_dptr"
#define HIDDEN_CHEME_ANY_SPTR      "hidden_cheme_any_sptr"
#define HIDDEN_CHEME_ANY_TYPE_PTR  "hidden_cheme_any_type_ptr"
#define HIDDEN_CHEME_ANY_TYPE      "hidden_cheme_any_type"


#define ANY_SIZE_THRESHOLD     16
#define ANY_SIZE_THRESHOLD_STR "16"

#define CHEME_TYPE_BOOL_STR \
    "typedef enum {false = 0      , true = 1      } bool;"
     typedef enum {cheme_false = 0, cheme_true = 1} cheme_bool;
#define CHEME_TYPE_UNIT_STR \
    "typedef struct {} unit;"
     typedef struct {} cheme_unit;
#define SS1 ANY_SIZE_THRESHOLD_STR
#define SS2 ANY_SIZE_THRESHOLD
struct cheme_type_desc;
#define CHEME_TYPE_ANY_STR \
    "typedef struct { type_desc *t;       char s["SS1"]; char *d; } any;"
     typedef struct { cheme_type_desc *t; char s[ SS2 ]; char *d; } cheme_any;
	 

struct ErrorContext {
	long pos;
};

ErrorContext current_error_context;

std::list<ErrorContext> error_contexts;

void push_error_context(ErrorContext ec) {
	error_contexts.push_back(ec);
}

void pop_error_context() {
	error_contexts.pop_back();
}

std::string error_context_to_string() {
	if (error_contexts.empty())
		return "Empty error context stack";
	char buf[256];
	sprintf(buf, "char %ld", error_contexts.back().pos);
	return buf;
}

int temp_index = 1;

std::string make_temp_name(int temp_index) {
	char buf[80];
	sprintf(buf, "hidden_cheme_temp%08d", temp_index);
	return std::string(buf);
}

std::string make_type_desc_name(int index) {
	char buf[80];
	sprintf(buf, "hidden_cheme_type_desc%08d", index);
	return std::string(buf);
}

std::string get_next_temp() {
	return make_temp_name(temp_index++);
}



int num;
char word[1000];


void cheme_printf_empty_stacks_to_stderr();

#define ABORT(x) ASSERT(false,x)
#define ASSERT(x,y) ((x) || (fputs((y),stderr), fprintf(stderr, "\nAt %s\n", error_context_to_string().c_str()), cheme_printf_empty_stacks_to_stderr(), fprintf(stderr, "At compiler source %s:%d\n", __FILE__, __LINE__) , abort(), true))

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
		       "cheme_printf_get_current_stack: there is no top body zone");
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
		if (written == max_write) {
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
	return cur.buf_origin;
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

#define TOKEN_TYPE_EOF    0
#define TOKEN_TYPE_OPEN   1
#define TOKEN_TYPE_CLOSE  2
#define TOKEN_TYPE_INT    3
#define TOKEN_TYPE_WORD   4
	
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
};



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

Term parse_word_to_term(const std::string &str) {
	Term term;
	term.type = TERM_TYPE_SINGLE;
	term.single = parse_word_to_token(str);
	return term;
}

Token read_token_for_real(FILE *f) {
	start:
	extern int yyoffset;
	current_error_context.pos = yyoffset;
	switch (yylex()) {
	case 0: return make_eof_token();
	case OPEN: return make_open_token();
	case CLOSE: return make_close_token();
	case NUMBER: return parse_int_to_token(num);
	case WORD: return parse_word_to_token(word);
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

void load_cur_token(FILE *f) { hidden_cur_token = read_token_for_real(f); }

Token peep_token(FILE *f) {
	return hidden_cur_token;
}

Token read_token(FILE *f) {
	Token t = hidden_cur_token;
	load_cur_token(f);
	return t;
}

Term read_term(FILE *f, int index) {
	Term ret;
	ret.ec = current_error_context;
	const Token tok = read_token(f);
//	printf("read_term: got token with type %d\n", tok.type);
	switch (tok.type) {
	case TOKEN_TYPE_INT:
		ret.type = TERM_TYPE_SINGLE;
		ret.single = tok;
		break;
	case TOKEN_TYPE_WORD:
		ret.type = TERM_TYPE_SINGLE;
		ret.single = tok;
		break;
	case TOKEN_TYPE_OPEN:
	{
		ret.type = TERM_TYPE_LIST;
//		printf("read_term: recursing\n");
		int index2 = 0;
		while (peep_token(f).type != TOKEN_TYPE_CLOSE &&
		       peep_token(f).type != TOKEN_TYPE_EOF) {
//			printf("read_term: calling read_term\n");
			ret.list.push_back(read_term(f, index2++));
		}
		if (read_token(f).type != TOKEN_TYPE_CLOSE) {
			ABORT("read_term: expected )");
		}
		break;
	}
	default:
		fprintf(stderr,
		        "read_term: expected ( or a simple term, "
		        "instead got token type %d\n", tok.type); ABORT("");
	}
	ret.index = index;
	return ret;
//	printf("read_term: done\n");
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


class FunctionManager {
private:
	class FunctionData {
	public:
		Type t;
		int scope_depth;
		int lambda_depth;
		std::string true_name;
	};
	typedef std::map<std::string, std::list<FunctionData> > map_t;
	typedef std::list< std::pair< std::string, std::list<map_t::iterator> > >
	    scope_t;
public:
	static bool accessible(const std::string &name)
	{
		FunctionData *p = get(name);
		return p != NULL && (p->lambda_depth == 0 ||
		                     p->lambda_depth == lambda_depth);
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
		add(name, newtype, name, "");
	}
	static void add(const std::string &name, const Type &newtype,
	                const std::string &true_name,
	                const std::string &dtor_str)
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
	static void start_lambda() { ++lambda_depth; }
	static void end_lambda() {
		--lambda_depth;
		ASSERT(lambda_depth >= 0, "lambda depth cannot be negative");
	}	
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
};

FunctionManager::map_t FunctionManager::map;
FunctionManager::scope_t FunctionManager::scope;
int FunctionManager::lambda_depth = 0;

class TypeInfo {
public:
	int size;
};

typedef TypeInfo (*TypeInfoFunc_t)(const std::list<Type> &type_params); 

#define SIMPLE_TYPEINFO_GETTER(type)									\
TypeInfo get_##type##_type_info(const std::list<Type> &ignored)         \
{ TypeInfo t; t.size = sizeof(type); return t;}


SIMPLE_TYPEINFO_GETTER(int)
SIMPLE_TYPEINFO_GETTER(char)
SIMPLE_TYPEINFO_GETTER(cheme_bool)
SIMPLE_TYPEINFO_GETTER(cheme_unit)
SIMPLE_TYPEINFO_GETTER(double)
SIMPLE_TYPEINFO_GETTER(cheme_any)
TypeInfo get_func_type_info(const std::list<Type> &ignored)
{ TypeInfo t; t.size = sizeof(void(*)()); return t; }

class TypeManager {
public:
	static void init() {
		typedef std::list<Type> L;
		base_types.insert(std::make_pair("int",    get_int_type_info));
		base_types.insert(std::make_pair("char",   get_char_type_info));
		base_types.insert(std::make_pair("bool",   get_cheme_bool_type_info));
		base_types.insert(std::make_pair("unit",   get_cheme_unit_type_info));
		base_types.insert(std::make_pair("double", get_double_type_info));
		base_types.insert(std::make_pair("any",    get_cheme_any_type_info));
		// we hope all funcptrs have same size
		poly_types.insert(std::make_pair
		                      ("func", std::make_pair(-1, get_func_type_info)));
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
	static TypeInfoFunc_t type_info_getter(const std::string &str) {
		ASSERT((is_poly_type(str) ? 1 : 0) + (is_base_type(str) ? 1 : 0) == 1,
		       "type_info_getter: unknown type");
		return is_poly_type(str) ?
		    poly_types[str].second : base_types[str];
	}
private:
	static std::map<std::string, TypeInfoFunc_t> base_types;
	static std::map<std::string, std::pair<int, TypeInfoFunc_t> > poly_types;
};
std::map<std::string, TypeInfoFunc_t> TypeManager::base_types;
std::map<std::string, std::pair<int, TypeInfoFunc_t> > TypeManager::poly_types;

class TypeInstanceManager {
private:
	class Compare {
	public:
		bool operator()(const Type &t1, const Type &t2)
		{ return strcmp(type_to_string(t1).c_str(),
		                type_to_string(t2).c_str()) < 0; }
	};
	typedef std::map<Type, int, Compare> map_t;
public:
	static int add(const Type &type) {
		std::pair<map_t::iterator, bool> pair =
		    map.insert(std::make_pair(type, map.size()));
		return pair.first->second;
	}
	static std::list< std::pair<Type, int> > all() {
		std::list< std::pair<Type, int> > ret;
		for (map_t::iterator it = map.begin(); it != map.end(); ++it)
			ret.push_back(*it);
		return ret;
	}
	static map_t map;
};
TypeInstanceManager::map_t TypeInstanceManager::map;

int type_size(const Type &t) {
	return (TypeManager::type_info_getter(t.name))(t.type_params).size;
}

bool is_type_term(const Term &term) {
	return ( (term.type == TERM_TYPE_SINGLE &&
	          term.single.type == TOKEN_TYPE_WORD &&
	          TypeManager::is_base_type(term.single.str)) ||
	         (term.type == TERM_TYPE_LIST &&
	          !term.list.empty() &&
	          term.list.begin()->is_single_word() &&
	          TypeManager::is_poly_type(term.list.begin()->single.str)));
}

bool is_var_name_term(const Term &term) {
	return !is_type_term(term) && term.is_single_word();
}

bool is_not_type_term(const Term &term) {
	return !is_type_term(term);
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


Type type_from_term(const Term &term) {
	Type ret;
	if (term.type == TERM_TYPE_SINGLE) {
		ASSERT(term.single.type == TOKEN_TYPE_WORD,
		       "type_from_term: base type name must be a word");
		ret.name = term.single.str;
	} else {
		ASSERT(!term.list.empty(),
		        "type_from_term: Found empty list where type should be");
		ASSERT(term.list.begin()->is_single_word(),
		       "type_from_Term: poly type name must be a word");
		ret.name = term.list.begin()->single.str;
		std::list<Term>::const_iterator it = term.list.begin(); ++it;
		for ( ; it != term.list.end(); ++it)
			ret.type_params.push_back(type_from_term(*it));
	}
	return ret;
}

int handle_term(Term &term);
int handle_term_for_fold(int ignored, Term &term) {
	return handle_term(term);
}


Type try_var_term_var_list_elem_type(const std::list<Term> &list) {
	ASSERT(!list.empty(), "try_var_term_var_list_elem_type: missing type");
	ASSERT(is_type_term(*list.begin()),
	       "try_var_term_var_list_elem_type: type should be 1st element");
	return type_from_term(*list.begin());
}

std::string try_var_term_var_list_elem_name(const std::list<Term> &list) {
	ASSERT(list.size() > 1, "try_var_term_var_list_elem_name: missing name");
	std::list<Term>::const_iterator it = list.begin(); ++it;
	ASSERT(is_var_name_term(*it),
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




std::string try_var_term_var_list_elem_translate
    (const Type &type,
     const std::string &name,
     const bool is_decl)
{
	std::string first_half;
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
		first_half = ret + params + "";
	} else {
		ASSERT(type.type_params.empty(),
		       "try_var_term_var_list_elem_translate: "
		       "can only handle base_types");
		first_half =
		    std::string(is_decl ? "extern " : "") + type.name + " " + name;
	}
	return first_half;
}

void try_var_term_var_list(std::list<Term>::iterator it,
                           const std::list<Term>::const_iterator itEnd,
                           const bool is_decl) {
begin:
	if (it == itEnd)
		return;
	else {
		Term &term = *it;
		ASSERT(term.type == TERM_TYPE_LIST &&
		       2 <= term.list.size() && term.list.size() <= (is_decl ? 2 : 3),
		       "Invalid variable format");
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
		if (type.name != "func" || init_value_temp_index == 0)
		{ // inital lambdas are a special case, they write themselves
			cheme_printf_set_zone(FunctionManager::in_global_scope() ?
			                      PRINTF_ZONE_TOP : PRINTF_ZONE_CURRENT_BODY);
			cheme_printf("%s;\n",translation.c_str());
			cheme_printf_unset_zone();
			if (init_value_temp_index != 0) {
				cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
				cheme_printf("%s = %s;\n", name.c_str(),
				             make_temp_name(init_value_temp_index).c_str());
				cheme_printf_unset_zone();
			}
		}
		if (!same_types(type, build_base_type("any"))) {
			FunctionManager::add_simple(name, type);
		} else {
			cheme_printf_push();
			cheme_printf("if (%s(&%s) > %d) free(*%s(&%s));\n",
			             HIDDEN_CHEME_ANY_SIZE, name.c_str(),
			             ANY_SIZE_THRESHOLD,
			             HIDDEN_CHEME_ANY_DPTR_PTR, name.c_str());
			std::auto_ptr<char> p(cheme_printf_pop());
			FunctionManager::add(name, type, name, std::string(p.get()));
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
		ret.push_back(handle_term(*itTerm));
		ASSERT(!same_types(itTerm->metadata.get_type(), build_base_type("any")),
		       "Any type in function calls is not yet supported (by-value)");
		ASSERT(same_types(itTerm->metadata.get_type(), *itType),
		       "Expected type of application argument does not match inferred");
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

int try_set_term(Term &term) {
	// TODO: this will soon change with addition of = and <-
	if (is_specific_special_form(term, "=")) {
		ASSERT(term.list.size() == 3, "set special form should get 2 params");
		std::list<Term>::iterator it = term.list.begin(); 
		const Term &var = *++it;
		ASSERT(var.is_single_word(), "variable name should be a single word");
		ASSERT(FunctionManager::accessible(var.single.str),
		       "Attempt to do set on non existent/accessible variable");
		Term &value = *++it;
		const int value_temp_index = handle_term(value);
		ASSERT(same_types(FunctionManager::type(var.single.str),
		                  value.metadata.get_type()),
		                  "assignment violates types");
		cheme_printf_set_zone(PRINTF_ZONE_CURRENT_BODY);
#define LEFT_STR (var.single.str.c_str())
#define RIGHT_STR (make_temp_name(value_temp_index).c_str())
		if (same_types(FunctionManager::type(var.single.str),
		               build_base_type("any")))
		{
#define MAX_SIZE ANY_SIZE_THRESHOLD		
			cheme_printf("{\n");
			cheme_printf("size_t size1 = %s(&%s);\n",
			             HIDDEN_CHEME_ANY_SIZE, LEFT_STR);
			cheme_printf("size_t size2 = %s(&%s);\n",
			             HIDDEN_CHEME_ANY_SIZE, RIGHT_STR);
			cheme_printf("if      (size1 <= %d && size2 >  %d) "
			             "*%s(&%s) = malloc(size2);\n",
			             MAX_SIZE, MAX_SIZE,
			             HIDDEN_CHEME_ANY_DPTR_PTR, LEFT_STR);
			cheme_printf("else if (size1 >  %d && size2 <= %d) "
			             "free(%s(%s));\n",
			             MAX_SIZE, MAX_SIZE,
			             HIDDEN_CHEME_ANY_DPTR, LEFT_STR);
			cheme_printf("if      (size2 <= %d) "
			             "%s = %s;\n",
			             MAX_SIZE, MAX_SIZE, LEFT_STR, RIGHT_STR);
			cheme_printf("else if (size2 >  %d) {"
			             "*%s(&%s) = *%s(&%s);\n"
			             "memcpy(%s(&%s),%s(&%s));\n}\n",
			             MAX_SIZE, MAX_SIZE,
			             HIDDEN_CHEME_ANY_TYPE_PTR, LEFT_STR,
			             HIDDEN_CHEME_ANY_TYPE_PTR, RIGHT_STR,
			             HIDDEN_CHEME_ANY_DPTR_PTR, LEFT_STR,
			             HIDDEN_CHEME_ANY_DPTR_PTR, RIGHT_STR);
#undef MAX_SIZE 
		} else {
			cheme_printf("%s = %s;\n", LEFT_STR, RIGHT_STR);
		}
#undef RIGHT_STR
#undef LEFT_STR
		cheme_printf_unset_zone();
		term.metadata.set_type(build_base_type("unit"));
		return try_unit_term_unitv_var();
	}
	return 0;
}

int try_and_term(Term &term) {
	if (is_specific_special_form(term, "and")) {
		ASSERT(term.list.size() > 2, "and"
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
			cheme_printf("if (!(%s)) goto %s;\n",
			             make_temp_name(value_temp_index).c_str(),
			             make_temp_name(false_label_index).c_str());
		}
		const std::string result_var = get_next_temp();
#define TVTVLET try_var_term_var_list_elem_translate
		cheme_printf("%s;\n",
		             TVTVLET(build_base_type("bool"), result_var,
		                     false).c_str());
#undef TVTVLET											
		cheme_printf("%s = true;\ngoto %s;\n",result_var.c_str(),
		             make_temp_name(finish_label_index).c_str());
		cheme_printf("%s: %s = false;\n",
		             make_temp_name(false_label_index).c_str(),
		             result_var.c_str());
		cheme_printf("%s:;\n", make_temp_name(finish_label_index).c_str());
		term.metadata.set_type(build_base_type("bool"));
		return temp_index - 1;
	}
	return 0;
}

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
				ASSERT(same_types(false_exp->metadata.get_type(),
				                  true_exp->metadata.get_type()),
				       "if true expression and false expression must "
				       "have same types");
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
		ASSERT(term.list.size() > 2,
		       "begin should wrap more than one expression");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		cheme_printf_push();
		FunctionManager::start_scope();
		const int result_index = temp_index++;
		cheme_printf("{\n");
		const int final_value_index = foldl(handle_term_for_fold, 0, it,
		                                    term.list.end());
		cheme_printf("%s = %s;\n",make_temp_name(result_index).c_str(),
		             make_temp_name(final_value_index).c_str());
		cheme_printf("}\n");
		FunctionManager::end_scope();
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
		std::string name = is_const_lambda ?
	        try_var_term_var_list_elem_name(term.parent->list) :
		    get_next_temp();
		std::list<Term>::iterator it = term.list.begin(); ++it;

		FunctionManager::start_scope(); FunctionManager::start_lambda();
		std::list<Type> type_params = ::map(try_lambda_term_add_param_var,
		                                    it->list, &*type_params.begin());
		Type type;
		if (is_const_lambda) {
			const Type declared_type =
			    try_var_term_var_list_elem_type(term.parent->list);
			std::list<Type>::const_iterator it =
			    declared_type.type_params.end(); --it;
			type_params.push_back(*it);
			ASSERT(same_type_lists(type_params, declared_type.type_params),
			       "Declared lambda type does not match actual lambda");
			type = build_poly_type("func", type_params);
			FunctionManager::add_simple(name, type); // allow recursion
		}
		if (is_var_init_term(term)) // if this is const lambda, allow recursion
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
		try_lambda_term_header(term, name, it->metadata.get_type());
		cheme_printf("%s",body.get());
		try_lambda_term_footer();
		if (!is_const_lambda) { // otherwise, this was already done above
			type_params.push_back(it->metadata.get_type());
			type = build_poly_type("func", type_params);
		} else { // make sure declared return type matches actual return type
			std::list<Type>::iterator it2 = type_params.end(); --it2;
			ASSERT(same_types(it->metadata.get_type(), *it2),
			       "Declared return type is not the same as "
			       "inferred return type");
		}

		cheme_printf_unset_zone();
		cheme_printf_pop_body();

		FunctionManager::end_scope(); FunctionManager::end_lambda();
		term.metadata.set_type(type);
		return -1;
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
		     make_type_desc_name(TypeInstanceManager::add(type)).c_str());
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
		const std::string type_desc_str = make_type_desc_name
		    (TypeInstanceManager::add(type));

		cheme_printf("if (%s(&%s) != %s) abort(); /* 'any' type-safety */ \n",
		             HIDDEN_CHEME_ANY_TYPE, value_str.c_str(),
		             type_desc_str.c_str());
		cheme_printf("%s;\n",
		             try_var_term_var_list_elem_translate(type, result_str,
					                                      false).c_str());
		try_to_any_term_assignment(value_str, result_str, true,
		                           type_size(type));
		term.metadata.set_type(type);
		return result_index;
	}
	return 0;
}


int try_to_any_term(Term &term) {
	if (is_specific_special_form(term, "to_any")) {
		ASSERT(term.list.size() == 2,
		       "to_any should accept a single expression");
		std::list<Term>::iterator it = term.list.begin(); ++it;
		const int value_index = handle_term(*it);
		const std::string value_str(make_temp_name(value_index));
		const Type &type = it->metadata.get_type();
		const int size = type_size(type);
		const int result_index = temp_index++;
		const std::string result_str(make_temp_name(result_index));
		const std::string type_desc_str = make_type_desc_name
		    (TypeInstanceManager::add(type));
		cheme_printf("any %s; *%s(&%s) = %s;\n",
		             result_str.c_str(),
		             HIDDEN_CHEME_ANY_TYPE_PTR,
		             result_str.c_str(), type_desc_str.c_str());
		if (size > ANY_SIZE_THRESHOLD)
			cheme_printf("*%s(&%s) = malloc(%d);\n",
			             HIDDEN_CHEME_ANY_DPTR_PTR, result_str.c_str(), size);
		try_to_any_term_assignment(result_str, value_str, false,
		                           type_size(it->metadata.get_type()));
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
	TRY(try_if_term)
	TRY(try_begin_term)
	TRY(try_lambda_term)
	TRY(try_int_term)
	TRY(try_unit_term)
	TRY(try_any_is_term)
	TRY(try_any_to_term)
	TRY(try_to_any_term)
	TRY(try_app_term)
	TRY(try_var_ref_term)
	ELSE
	{
		ABORT("Expected: special form or application form of a known callable");
	}
	ASSERT(term.metadata.has_type(), "term without type???");
	pop_error_context();
	return used_temp_index;
}

void print_type_desc(const std::pair<Type, int> &pair) {
// we need too access to printf for this one
#undef printf
	printf("type_desc %s_data = {%d};\n",
	       make_type_desc_name(pair.second).c_str(), type_size(pair.first));
	printf("type_desc * const %s = &%s_data;\n",
	       make_type_desc_name(pair.second).c_str(),
	       make_type_desc_name(pair.second).c_str());
	       
#define printf DO NOT USE THIS
}

void print_type_descs() {
	std::list< std::pair<Type, int> > desc_pairs = TypeInstanceManager::all();
	std::for_each(desc_pairs.begin(), desc_pairs.end(),
	              print_type_desc);
}

void print_header_stuff() {
	puts("void abort(void); void *malloc(long size); void free(void *ptr);\n");
	puts("void *memcpy(void *dest, const void *src, long n);");
	puts("typedef struct { int size; } type_desc;");
	puts(CHEME_TYPE_BOOL_STR);
	puts(CHEME_TYPE_UNIT_STR);
	puts("static unit unitv() { unit v; return v; }");
	puts(CHEME_TYPE_ANY_STR);
	puts("static type_desc**"HIDDEN_CHEME_ANY_TYPE_PTR"(any *a) "
	     "{return &a->t;}");
	puts("static type_desc *"HIDDEN_CHEME_ANY_TYPE    "(any *a) "
	     "{return  a->t;}");
	puts("static char **    "HIDDEN_CHEME_ANY_DPTR_PTR"(any *a) "
	     "{return &a->d;}");
	puts("static char *     "HIDDEN_CHEME_ANY_SPTR    "(any *a) "
	     "{return a->s;}");
	
	puts("static int hidden_cheme_any_size(any *a) {return a->t->size;}");
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
	puts("static int "HIDDEN_CHEME_CMP_INTS"(int x, int y) "
	     "{ return x == y; }");
	puts("static double int2double(int x) { return (double)x; }");
	puts("static int double2int(double x) { return (int)x; }");
	print_type_descs();
}

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
	                     HIDDEN_CHEME_CMP_INTS, "");
	FunctionManager::add("<", build_poly_type("func", iib),
	                     HIDDEN_CHEME_LT, "");
	FunctionManager::add(">", build_poly_type("func", iib),
	                     HIDDEN_CHEME_GT, "");
	std::list<Type> iii;
	iii.push_back(build_base_type("int"));
	iii.push_back(build_base_type("int"));
	iii.push_back(build_base_type("int"));
	FunctionManager::add_simple("mod", build_poly_type("func", iii));
	FunctionManager::add_simple("div", build_poly_type("func", iii));
	FunctionManager::add("+", build_poly_type("func", iii),
	                     HIDDEN_CHEME_ADD_INTS, "");
	FunctionManager::add("-", build_poly_type("func", iii),
	                     HIDDEN_CHEME_SUB_INTS, "");
	std::list<Type> id;
	id.push_back(build_base_type("int"));
	id.push_back(build_base_type("double"));
	FunctionManager::add_simple("int2double", build_poly_type("func", id));
	std::list<Type> di;
	di.push_back(build_base_type("double"));
	di.push_back(build_base_type("int"));
	FunctionManager::add_simple("double2int", build_poly_type("func", di));
}

int main() {
	TypeManager::init();
	load_cur_token(stdin);
	std::list<Term> all_terms;
	cheme_printf_push_body();
	cheme_printf_top_zone_stack.push(cheme_printf_context());
	FunctionManager::start_scope();
	add_primitives();
	
	for (int i = 0; peep_token(stdin).type != TOKEN_TYPE_EOF; ++i)
		all_terms.push_back(read_term(stdin, i));

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
}
