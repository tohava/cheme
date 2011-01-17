#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <list>
#include <string>
#include <map>
#include <set>
#include <algorithm>

#include "lexer.h"

int temp_index = 1;

std::string make_temp_name(int temp_index) {
	char buf[80];
	sprintf(buf, "temp%08d", temp_index);
	return std::string(buf);
}

std::string get_next_temp() {
	return make_temp_name(temp_index++);
}



int num;
char word[1000];

#define ABORT(x) (fputs(x,stderr), fputs("\n", stderr), abort())
#define ASSERT(x,y) ((x) || (fputs((y),stderr), fprintf(stderr, " At %s:%d\n", __FILE__, __LINE__) , abort(), true))


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

bool same_types(const Type &t1, const Type &t2) {
#define CASE(X,Y) case X: return t2.type == X && t1.Y == t2.Y;

	return t1.name == t2.name &&
	    t1.type_params.size() == t2.type_params.size() &&
	    std::equal(t1.type_params.begin(), t1.type_params.end(),
	               t2.type_params.begin(), same_types);
#undef CASE
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
	Maybe<Type> type;
	void set_type(const Type &typ) { type = Maybe<Type>(typ); }
	const Type &get_type() {
		ASSERT(type.is_valid(),
		       "TermMetaData::get_type - there is no type");
		return type.get_data();
	}
};

#define TERM_TYPE_LIST 0
#define TERM_TYPE_SINGLE 1
struct Term {
	int type;
	std::list<Term> list;
	Token single;
	TermMetaData metadata;
	bool is_single_specific_word(const char *word) const {
		return type == TERM_TYPE_SINGLE && single.is_specific_word(word);
	}
	bool is_single_word() const {
		return type == TERM_TYPE_SINGLE && single.type == TOKEN_TYPE_WORD;
	}
};



const Token hidden_eof_token = {TOKEN_TYPE_EOF};
#define EOF_TOKEN hidden_eof_token
const Token hidden_open_token = {TOKEN_TYPE_OPEN};
#define OPEN_TOKEN hidden_open_token
const Token hidden_close_token = {TOKEN_TYPE_CLOSE};
#define CLOSE_TOKEN hidden_close_token

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
	switch (yylex()) {
	case 0: return EOF_TOKEN;
	case OPEN: return OPEN_TOKEN;
	case CLOSE: return CLOSE_TOKEN;
	case NUMBER: return parse_int_to_token(num);
	case WORD: return parse_word_to_token(word);
	case SPACE: goto start;
	case UNKNOWN: fprintf(stderr, "Unknown token %s\n", word); abort();
	default: abort();
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
		fprintf(stderr, "read_term: expected ( or a simple term, instead got token type %d\n", tok.type); abort();
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

class WriteTerm {
public:
	WriteTerm(FILE *f) : f(f) {}
	void operator()(const Term &term) {
		void write_term(FILE *f, const Term &term);
		write_term(f, term);
	}
private:
	FILE *f;
};

void write_term(FILE *f, const Term &term) {
	switch (term.type) {
	case TERM_TYPE_SINGLE:
		write_token(f, term.single);
		break;
	case TERM_TYPE_LIST:
		write_token(f, OPEN_TOKEN);
		std::for_each(term.list.begin(), term.list.end(), WriteTerm(f));
		write_token(f, CLOSE_TOKEN);
		break;
	default:
		ABORT("write_term: Unknown term");
	}
}

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
		abort();
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


class FunctionManager {
public:
	static bool exists(const std::string &str)
	{return map.find(str) != map.end();}

	static Type type(const std::string &name) {
		ASSERT(exists(name), "FunctionManager::type: unknown name");
		return map[name].t;
	}
	static void add(const std::string &name, const Type &newtype)
	{
		if (exists(name) && !same_types(type(name), newtype)) {
			fprintf(stderr,
			        "Attempt to add variable '%s' with two "
			        "different types\n", name.c_str());
		}
		map[name].t = newtype;
	}
private:
	class FunctionData { public: Type t; };
private:
	static std::map<std::string, FunctionData> map;
};
std::map< std::string, FunctionManager::FunctionData> FunctionManager::map;

class TypeManager {
public:
	static void init() {
		base_types.insert("int");
		base_types.insert("char");
		base_types.insert("unit");
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

bool is_not_type_term(const Term &term) {
	return !is_type_term(term);
}

bool is_not_list(const Term &term) {
	return term.type != TERM_TYPE_LIST;
}



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


std::string try_var_term_var_list_elem_name(const std::list<Term> &list) {
	std::list<Term> filter_result = filter(list, is_not_type_term);
	switch (filter_result.size()) {
	case 0:
		ABORT("Variable definition without a name");
	case 1:
		ASSERT(filter_result.begin()->type == TERM_TYPE_SINGLE ||
		       filter_result.begin()->single.type == TOKEN_TYPE_WORD,
		       "Variable name should be a single word");
		return filter_result.begin()->single.str;
	default:
		ABORT("Variable can only have one name");
	}
}

Type build_base_type(const std::string &str) {
	Type t;
	ASSERT(TypeManager::is_base_type(str), "build_type: called with unknown type");
	t.name = str;
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

Type try_var_term_var_list_elem_type(const std::list<Term> &list) {
	std::list<Term> filter_result = filter(list, is_type_term);
	switch (filter_result.size()) {
	case 1:
		return type_from_term(*filter_result.begin());
	default:
		ABORT("Variable can only have one type");
	}
}



std::string word_from_term(const Term &term) {
	ASSERT(term.type == TERM_TYPE_SINGLE &&
	       term.single.type == TOKEN_TYPE_WORD,
	       "word_from_term: unsupported input");
	return term.single.str;
}

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



std::string try_var_term_var_list_elem_translate(const Type &type,
                                                 const std::string &name,
                                                 const bool is_decl) {
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
			params += try_var_term_var_list_elem_translate(*it, "", false);
		}
		params += ")";
		ret = (try_var_term_var_list_elem_translate(*itRet, "", false) + " " +
		       (is_decl ? "" : "(*") + name + (is_decl ? "" : ")"));
		return ret + params + "";
	} else {
		ASSERT(type.type_params.empty(), "try_var_term_var_list_elem_translate: can only handle base_types");
		return std::string(is_decl ? "extern " : "") + type.name + " " + name;
	}
}

void try_var_term_var_list(std::list<Term>::const_iterator it,
                           const std::list<Term>::const_iterator itEnd,
                           const bool is_decl) {
begin:
	if (it == itEnd)
		return;
	else {
		Term term = force_to_list(*it);
		std::string name = try_var_term_var_list_elem_name(term.list);
		Type type = try_var_term_var_list_elem_type(term.list);
		printf("%s;\n", try_var_term_var_list_elem_translate(type, name,
		                                                     is_decl).c_str());
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

	std::list<Term>::const_iterator it = term.list.begin(); ++it;
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

	std::list<Term>::const_iterator it = term.list.begin(); ++it;
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
                                   std::list<Term>::iterator itTerm)
{
	int handle_term(Term &term);
	std::list<int> ret;
	if (!itTerm->metadata.type.is_valid())
		ret.push_back(handle_term(*itTerm));
	ASSERT(same_types(itTerm->metadata.type.get_data(), *itType),
	       "Expected type of application argument does not match inferred");
	return ret;
}

int try_app_term(Term &term) {
	const std::string str = is_app_form(term);
	if (str.empty()) return 0;
	const Type t = FunctionManager::type(str);
	ASSERT(is_callable_type(t), "Attempt to do application on non-callable");
	std::list<Type>::const_iterator itType = t.type_params.begin(); ++itType;
	std::list<Term>::iterator itTerm = term.list.begin(); ++itTerm;
	const std::list<int> temp_indices = try_app_term_params(itType, itTerm);
//	printf("Application of %s\n", str.c_str());
	printf("%s(", str.c_str());
	std::list<int>::const_iterator it = temp_indices.begin();
	for (; it != temp_indices.end(); ++it)
		printf("%s%s", it == temp_indices.begin() ? "" : ",",
		       make_temp_name(*it).c_str());
	printf(");\n");
	term.metadata.set_type(*t.type_params.begin());
	return true;
}

int try_int_term(Term &term) {

	if (term.type == TERM_TYPE_SINGLE  &&
	    term.single.type == TOKEN_TYPE_INT)
	{
		const Type int_type = build_base_type("int");
		term.metadata.set_type(int_type);
		printf("%s = %d;\n",
		       try_var_term_var_list_elem_translate
		       (int_type, get_next_temp(), false).c_str(),
		       term.single.num);
		return temp_index - 1;
	}
	else
		return 0;
}

int try_unit_term(Term &term) {
	if (term.is_single_specific_word("unitv")) {
		term.metadata.set_type(build_base_type("unit"));
		return temp_index++;
	}
	return 0;
}

int try_set_term(Term &term) {
	int handle_term(Term &term);
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
		                  value.metadata.type.get_data()),
		                  "assignment violates types");
		printf("%s = %s;\n",
		       var.single.str.c_str(),
		       make_temp_name(value_temp_index).c_str());
		term.metadata.set_type(build_base_type("unit"));
		return temp_index++;
	}
	return 0;
}

int handle_term(Term &term) {
	Maybe<Type> type;
	int used_temp_index = 0;
#define BEGIN_TRY() if (false) {}
#define TRY(func) else if ((used_temp_index = func(term))) {}
#define ELSE else

	BEGIN_TRY()
	TRY(try_var_term)
	TRY(try_decl_term)
	TRY(try_app_term)
	TRY(try_int_term)
	TRY(try_unit_term)
	TRY(try_set_term)
	ELSE
	{
		ABORT("Expected: special form or application form of a known callable");
	}
	ASSERT(term.metadata.type.is_valid(), "term without type???");
	return used_temp_index;
}

void print_header_stuff() {
	printf("typedef struct {} unit;\n"
	       "unit internal_unitv() { unit v; return v; }\n");
	printf("int main() {");
}

void print_footer_stuff() {
	printf("}");
}

int main() {
	TypeManager::init();
	load_cur_token(stdin);
	std::list<Term> all_terms;
	print_header_stuff();

	while (peep_token(stdin).type != TOKEN_TYPE_EOF)
		all_terms.push_back(read_term(stdin));
	std::for_each(all_terms.begin(), all_terms.end(), handle_term);

	print_footer_stuff();
}
