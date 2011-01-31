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
#include <memory>

#include "lexer.h"

#define HIDDEN_CHEME_LT "hidden_cheme_less_than"
#define HIDDEN_CHEME_GT "hidden_cheme_greater_than"
#define HIDDEN_CHEME_EQ "hidden_cheme_equals"
#define HIDDEN_CHEME_INT_TO_DOUBLE "hidden_cheme_int_to_double"
#define HIDDEN_CHEME_DOUBLE_TO_INT "hidden_cheme_double_to_int"
#define HIDDEN_CHEME_ADD_INTS "hidden_cheme_add_ints"
#define HIDDEN_CHEME_SUB_INTS "hidden_cheme_sub_ints"
#define HIDDEN_CHEME_CMP_INTS "hidden_cheme_cmp_ints"

typedef struct {
	char *buf_origin;
	char *buf_ptr;
	size_t buf_size;
} cheme_printf_context;
std::stack<cheme_printf_context> cheme_printf_stack;

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
	if (cheme_printf_stack.empty()) {
		va_start(argp, format);
		int ret = vprintf(format, argp);
		va_end(argp);
		return ret;
	} else {
		cheme_printf_context &cur = cheme_printf_stack.top();
		va_start(argp, format);
		int written = cheme_vprintf_to_context(cur, format, argp);
		va_end(argp);
		return written;
	}
}

void cheme_printf_push() {
	cheme_printf_stack.push(cheme_printf_context());
	cheme_printf_context &cur = cheme_printf_stack.top();
	cur.buf_origin = cur.buf_ptr = NULL;
	cur.buf_size = 0;
}

char *cheme_printf_pop() {
	if (!cheme_printf_stack.empty()) {
		cheme_printf_context cur = cheme_printf_stack.top();
		cheme_printf_stack.pop();
		return cur.buf_origin;
	} else
		return NULL;
}

#define printf DO NOT USE


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

std::string get_next_temp() {
	return make_temp_name(temp_index++);
}



int num;
char word[1000];

#define ABORT(x) ASSERT(false,x)
#define ASSERT(x,y) ((x) || (fputs((y),stderr), fprintf(stderr, "\nAt %s\n", error_context_to_string().c_str()), fprintf(stderr, "At compiler source %s:%d\n", __FILE__, __LINE__) , abort(), true))


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


Term read_term(FILE *f) {
	Term ret;
	ret.ec = current_error_context;
	const Token tok = read_token(f);
//	printf("read_term: got token with type %d\n", tok.type);
	switch (tok.type) {
	case TOKEN_TYPE_INT:
		ret.type = TERM_TYPE_SINGLE;
		ret.single = tok;
		return ret;
	case TOKEN_TYPE_WORD:
		ret.type = TERM_TYPE_SINGLE;
		ret.single = tok;
		return ret;
	case TOKEN_TYPE_OPEN:
	{
		ret.type = TERM_TYPE_LIST;
//		printf("read_term: recursing\n");
		while (peep_token(f).type != TOKEN_TYPE_CLOSE &&
		       peep_token(f).type != TOKEN_TYPE_EOF) {
//			printf("read_term: calling read_term\n");
			ret.list.push_back(read_term(f));
		}
		if (read_token(f).type != TOKEN_TYPE_CLOSE) {
			ABORT("read_term: expected )");
		}
		return ret;
	}
	default:
		fprintf(stderr, "read_term: expected ( or a simple term, instead got token type %d\n", tok.type); ABORT("");
	}
//	printf("read_term: done\n");
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
		ret.push_back(func(*it));
	}
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
	for (B it = begin; it != end; ++it) { acc = func(acc, *it); }
	return acc;
}

template <class A, class B, class FUNC>
A foldl(const FUNC &func, A acc, const B &list) {
	foldl(func, acc, list.begin(), list.end());
}

template <class A, class B>
const A &pair_first(const std::pair<A,B> &pair) { return pair.first; }


class FunctionManager {
private:
	class FunctionData { public: Type t; int scope_depth; std::string true_name; };
	typedef std::map<std::string, std::list<FunctionData> > map_t;
public:
	static bool exists(const std::string &str)
	{
		return get(str) != NULL;
	}
	
	static const Type &type(const std::string &name) {
		ASSERT(exists(name), "FunctionManager::type: unknown name");
		return get(name)->t;
	}
	static const std::string &true_name(const std::string &name) {
		ASSERT(exists(name), "FunctionManager::true_name: unknown name");
		return get(name)->true_name;
	}
	static void add(const std::string &name, const Type &newtype)
	{
		add_with_true_name(name, newtype, name);
	}
	static void add_with_true_name(const std::string &name, const Type &newtype,
	                               const std::string &true_name)
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
		list.back().true_name = true_name;
		scope.back().push_back(it);

	}
	static void start_scope() {
		scope.push_back(std::list<map_t::iterator>());
	}
	static void list_pop_and_maybe_delete(map_t::iterator it) {
		it->second.pop_back();
		if (it->second.empty()) map.erase(it);
	}
	static void end_scope() {
		std::for_each(scope.back().begin(), scope.back().end(),
		              list_pop_and_maybe_delete);
		scope.pop_back();
	}
	static int cur_scope_depth() { return scope.size() - 1; }
private:
	static FunctionData *get(const std::string &name) {
		map_t::iterator it = map.find(name);
		return it == map.end() ? NULL : &map.find(name)->second.back();
	}
	static map_t map;
	static std::list< std::list<map_t::iterator> > scope;
};

FunctionManager::map_t FunctionManager::map;
std::list< std::list<FunctionManager::map_t::iterator> > FunctionManager::scope;

class TypeManager {
public:
	static void init() {
		base_types.insert("int");
		base_types.insert("char");
		base_types.insert("bool");
		base_types.insert("unit");
		base_types.insert("double");
		poly_types.insert(std::make_pair("func", -1));
	}
	static bool is_base_type(const std::string &str) {
		return base_types.find(str) != base_types.end();
	}
	static bool is_poly_type(const std::string &str) {
		return poly_types.find(str) != poly_types.end();
	}
	static int poly_type_arity(const std::string &str) {
		ASSERT(is_poly_type(str), "poly_type_arity: called with nonexistent type");
		return poly_types[str];
	}
private:
	static std::set<std::string> base_types;
	static std::map<std::string, int> poly_types;
};
std::set<std::string> TypeManager::base_types;
std::map<std::string, int> TypeManager::poly_types;
	
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
	ASSERT(TypeManager::is_base_type(str), "build_base_type: called with unknown type");
	t.name = str;
	return t;
}

Type build_poly_type(const std::string &str, const std::list<Type> &type_params) {
	Type t;
	ASSERT(TypeManager::is_poly_type(str), "build_poly_type: called with unknown type");
	ASSERT(TypeManager::poly_type_arity(str), "build_poly_type: called with bad arity");
	t.name = str;
	t.type_params = type_params;
	return t;
}


Type type_from_term(const Term &term) {
	Type ret;
	if (term.type == TERM_TYPE_SINGLE) {
		ASSERT(term.single.type == TOKEN_TYPE_WORD, "type_from_term: base type name must be a word");
		ret.name = term.single.str;
	} else {
		ASSERT(!term.list.empty(), "type_from_term: Found empty list where type should be");
		ASSERT(term.list.begin()->is_single_word(), "type_from_Term: poly type name must be a word");
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

int try_var_term_var_list_elem_init(std::list<Term> &list) {
	if (list.size() > 2) {
		std::list<Term>::iterator it = list.begin(); ++++it;
		return handle_term(*it);
	} else
		return 0;
}

std::string word_from_term(const Term &term) {
	ASSERT(term.type == TERM_TYPE_SINGLE &&
	       term.single.type == TOKEN_TYPE_WORD,
	       "word_from_term: unsupported input");
	return term.single.str;
}




std::string try_var_term_var_list_elem_translate(const Type &type,
                                                 const std::string &name,
                                                 const bool is_decl,
                                                 const int init_value_temp_index) {
	std::string init_value = " = " + make_temp_name(init_value_temp_index);
	std::string first_half;
	if (type.name == "func") {
		ASSERT(TypeManager::is_poly_type(type.name),
		       "func is not really func?");
		ASSERT(!type.type_params.empty(), "try_var_term_var_list_elem_translate: func without type_params???");
		std::list<Type>::const_iterator it = type.type_params.end();
		const std::list<Type>::const_iterator itRet = --it;
		std::string ret, params;
		params += "(";
		for (it = type.type_params.begin(); it != itRet; ++it) {
			if (it != type.type_params.begin()) params += ",";
			params += try_var_term_var_list_elem_translate(*it, "", false, 0);
		}
		params += ")";
		ret = (try_var_term_var_list_elem_translate(*itRet, "", false, 0) + " " +
		       (is_decl ? "" : "(*") + name + (is_decl ? "" : ")"));
		first_half = ret + params + "";
	} else {
		ASSERT(type.type_params.empty(), "try_var_term_var_list_elem_translate: can only handle base_types");
		first_half = std::string(is_decl ? "extern " : "") + type.name + " " + name;
	}
	return first_half + (init_value_temp_index ? init_value : "");
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
		int init_value_temp_index = try_var_term_var_list_elem_init(term.list);
		std::string translation = try_var_term_var_list_elem_translate
		    (type, name, is_decl, init_value_temp_index);
		cheme_printf("%s;\n",translation.c_str());
		       
//		printf("Var %s Type %s\n", name.c_str(), type_to_string(type).c_str());
		FunctionManager::add(name, type);
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
	        FunctionManager::exists(term.list.begin()->single.str)) ?
	    term.list.begin()->single.str : "";
}

std::list<int> try_app_term_params(std::list<Type>::const_iterator itType,
                                   const std::list<Type>::const_iterator itTypeEnd,
                                   std::list<Term>::iterator itTerm,
                                   const std::list<Term>::const_iterator itTermEnd)
{
	std::list<int> ret;
begin:
	int at_end = ((itType == itTypeEnd) ? 1 : 0) + ((itTerm == itTermEnd) ? 1 : 0);
	ASSERT(at_end % 2 == 0, "try_app_term_params: arity mismatch");
	if (!at_end) {
		ret.push_back(handle_term(*itTerm));
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

	int result_index = temp_index++;
#define TVTVLET try_var_term_var_list_elem_translate
	cheme_printf("%s = %s(",
	       TVTVLET(*itTypeRet, make_temp_name(result_index).c_str(),
	               false, 0).c_str(), FunctionManager::true_name(str).c_str());
#undef TVTVLET
	std::list<int>::const_iterator it = temp_indices.begin();
	for (; it != temp_indices.end(); ++it)
		cheme_printf("%s%s", it == temp_indices.begin() ? "" : ",",
		             make_temp_name(*it).c_str());
	cheme_printf(");\n");
	term.metadata.set_type(*itTypeRet);
	return result_index;
}

int try_int_term(Term &term) {

	if (term.type == TERM_TYPE_SINGLE  &&
	    term.single.type == TOKEN_TYPE_INT)
	{
		const Type int_type = build_base_type("int");
		term.metadata.set_type(int_type);
		cheme_printf("%s = %d;\n",
		             try_var_term_var_list_elem_translate
		             (int_type, get_next_temp(), false, 0).c_str(),
		             term.single.num);
		return temp_index - 1;
	}
	else
		return 0;
}

int try_unit_term_unitv_var() {
#define TVTVLAW try_var_term_var_list_elem_translate
	cheme_printf("%s;\n", TVTVLAW(build_base_type("unit"), get_next_temp(),
	                              false, 0).c_str());
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
		ASSERT(FunctionManager::exists(var.single.str),
		       "Attempt to do set on non existent variable");
		Term &value = *++it;
		const int value_temp_index = handle_term(value);
		ASSERT(same_types(FunctionManager::type(var.single.str),
		                  value.metadata.get_type()),
		                  "assignment violates types");
		cheme_printf("%s = %s;\n",
		             var.single.str.c_str(),
		             make_temp_name(value_temp_index).c_str());
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
		             TVTVLET(build_base_type("bool"), result_var, false,
		                     0).c_str());
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
		const Type &type = FunctionManager::type(term.single.str);
		std::string next_temp = get_next_temp();
		cheme_printf("%s = %s;\n",
		             try_var_term_var_list_elem_translate(type, next_temp,
		                                                  false, 0).c_str(),
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
	cheme_printf("if (!(%s)) break;\n", make_temp_name(cond_temp_index).c_str());
}

void try_while_term_translate_footer() {
	cheme_printf("}\n");
}

int try_while_term(Term &term) {
	if (is_specific_special_form(term, "while")) {
		ASSERT(term.list.size() >= 2, "while should contain condition");
		std::list<Term>::iterator it1 = term.list.begin(); ++it1;
		std::list<Term>::iterator it2 = term.list.begin(); ++++it2;
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
		                              result_var, false, 0).c_str());
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
		                            false, 0).c_str());
#undef TVTVLET
		cheme_printf("%s", p.get());
		term.metadata.set_type(it->metadata.get_type());
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
	TRY(try_int_term)
	TRY(try_unit_term)
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

void print_header_stuff() {
	cheme_printf("typedef struct {} unit;\n"
	             "static unit unitv() { unit v; return v; }\n");
	cheme_printf("static int "HIDDEN_CHEME_LT"(int x, int y) "
	             "{ return x < y; }\n");
	cheme_printf("static int "HIDDEN_CHEME_GT"(int x, int y) "
	             "{ return x > y; }\n");
	cheme_printf("static int "HIDDEN_CHEME_EQ"(int x, int y) "
	             "{ return x == y; }\n");
	cheme_printf("static int mod(int x, int y) { return x %% y; }\n");
	cheme_printf("static int div(int x, int y) { return x / y; }\n");
	cheme_printf("static int "HIDDEN_CHEME_ADD_INTS"(int x, int y) "
	             "{ return x + y; }\n");
	cheme_printf("static int "HIDDEN_CHEME_SUB_INTS"(int x, int y) "
	             "{ return x - y; }\n");
	cheme_printf("static int "HIDDEN_CHEME_CMP_INTS"(int x, int y) "
	             "{ return x == y; }\n");
	cheme_printf("static double int2double(int x) { return (double)x; }\n");
	cheme_printf("static int double2int(double x) { return (int)x; }\n");
	cheme_printf("typedef enum { false = 0, true = 1} bool;\n"); 
	cheme_printf("int main() {");
}

void print_footer_stuff() {
	cheme_printf("}");
}

void add_primitives() {
	std::list<Type> iib;
	iib.push_back(build_base_type("int"));
	iib.push_back(build_base_type("int"));
	iib.push_back(build_base_type("bool"));
	FunctionManager::add_with_true_name("==", build_poly_type("func", iib),
	                                    HIDDEN_CHEME_CMP_INTS);
	FunctionManager::add_with_true_name("<", build_poly_type("func", iib),
	                                    HIDDEN_CHEME_LT);
	FunctionManager::add_with_true_name(">", build_poly_type("func", iib),
	                                    HIDDEN_CHEME_GT);
	std::list<Type> iii;
	iii.push_back(build_base_type("int"));
	iii.push_back(build_base_type("int"));
	iii.push_back(build_base_type("int"));
	FunctionManager::add("mod", build_poly_type("func", iii));
	FunctionManager::add("div", build_poly_type("func", iii));
	FunctionManager::add_with_true_name("+", build_poly_type("func", iii),
	                                    HIDDEN_CHEME_ADD_INTS);
	FunctionManager::add_with_true_name("-", build_poly_type("func", iii),
	                                    HIDDEN_CHEME_SUB_INTS);
	std::list<Type> id;
	id.push_back(build_base_type("int"));
	id.push_back(build_base_type("double"));
	FunctionManager::add("int2double", build_poly_type("func", id));
	std::list<Type> di;
	di.push_back(build_base_type("double"));
	di.push_back(build_base_type("int"));
	FunctionManager::add("double2int", build_poly_type("func", di));
}

int main() {
	TypeManager::init();
	load_cur_token(stdin);
	std::list<Term> all_terms;
	print_header_stuff();
	FunctionManager::start_scope();
	add_primitives();
	while (peep_token(stdin).type != TOKEN_TYPE_EOF)
		all_terms.push_back(read_term(stdin));
	std::for_each(all_terms.begin(), all_terms.end(), handle_term);
	FunctionManager::end_scope();
	print_footer_stuff();
	ASSERT(cheme_printf_stack.empty(), "cheme_printf_stack NOT empty");
}
