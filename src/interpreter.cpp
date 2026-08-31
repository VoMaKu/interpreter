#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <map>
#include <algorithm>
#include <cctype>
#include "interpreter.hpp"
#include "Lexem.hpp"
#include "Number.hpp"
#include "Variable.hpp"
#include "Oper.hpp"
#include "Goto.hpp"
#include "Function.hpp"
#include "enum.hpp"

bool DEBUG = false;

bool INTERACTIVE = true;

// operators spelled with letters: a name may start with them, so one has to
// look at what follows before calling them an operator
static const OPERATOR WORD_OPERATORS[] = {
	FUNCTION, RETURN, ENDFUNCTION, IF, ELSE, WHILE, GOTO, OR, AND
};

std::string ERRORTEXT[] = { // one text per ERRORS value, in the same order
	"Unknown label, or a label that belongs to another function",
	"Brackets are not balanced",
	"def and fed are not balanced",
	"if, else or while is left unclosed",
	"Bad function name after def",
	"Bad parameter list in the function definition",
	"Incorrect use of if, else or while",
	"The program needs a function main",
	"Use of a variable that has no value yet",
	"Function was called with a wrong number of arguments",
	"The left side of := is not a variable",
	"No colon after the condition of if, else or while",
	"fed inside an if or while that is still open",
	"Invalid syntax",
	"A function that returns nothing was used as a value",
	"Division by zero",
	"Not enough operands for the operator",
	"The line leaves more than one value behind",
	"The result does not fit in a 32 bit integer",
	"A shift count has to be between 0 and 31"
};

// the debug dump asks each kind of lexem to print itself
void print_lexem(Lexem *lexem) {
	if (lexem == nullptr) {
		return;
	}
	if (dynamic_cast<Number *>(lexem)) {
		((Number *)lexem) -> print();
	} else if (dynamic_cast<Variable *>(lexem)) {
		((Variable *)lexem) -> print();
	} else if (dynamic_cast<Function *>(lexem)) {
		((Function *)lexem) -> print();
	} else if (dynamic_cast<Goto *>(lexem)) {
		((Goto *)lexem) -> print();
	} else {
		((Oper *)lexem) -> print();
	}
}

void print_lexems(const std::vector<Lexem *> &lexems) {
	for (auto &lexem: lexems) {
		print_lexem(lexem);
	}
}

Lexem *is_oper(std::string codeline, int &i, int size) {
	Lexem *lex = nullptr;
	char tmp = codeline[i];
	while (i < size && (tmp == ' ' || tmp == '\t')) {
		++i;
		tmp = codeline[i];
	}
	int n = sizeof(OPERTEXT) / sizeof(std::string);
	for (int op = 0; op < n; op++) {
		bool flag_not_a_oper = false;
		std::string subcodeline = codeline.substr(i, OPERTEXT[op].size());
		int words = sizeof(WORD_OPERATORS) / sizeof(OPERATOR);
		for (int j = 0; j < words; j++) {
			if ((OPERATOR)op == WORD_OPERATORS[j]) {
				int size_of_opertext = OPERTEXT[op].size();
				char next_symbol = ' ';
				if (i + size_of_opertext < size) {
					next_symbol = codeline[i + size_of_opertext];
				}
				if (isdigit(next_symbol) || isalpha(next_symbol) || next_symbol == '_')
					flag_not_a_oper = true;
				break;
			}
		}
		if (OPERTEXT[op] == subcodeline && !flag_not_a_oper) {
			if ((OPERATOR)op == IF || (OPERATOR)op == ELSE || (OPERATOR)op == WHILE
								|| (OPERATOR)op == ENDLOOP || (OPERATOR)op == RETURN) {
				lex = new Goto((OPERATOR)op);
			} else {
				lex = new Oper((OPERATOR)op);
			}
			i += OPERTEXT[op].size();
			break;
		}
	}
	return lex;
}

Lexem *is_number(std::string codeline, int &i, int size) {
	int p = i, number = 0;
	for ( ; p < size && codeline[p] >= '0' && codeline[p] <= '9'; p++) {
		int digit = codeline[p] - '0';
		if (__builtin_mul_overflow(number, 10, &number)
				|| __builtin_add_overflow(number, digit, &number)) {
			throw (ERR_INTEGER_OVERFLOW);
		}
	}
	if (p == i) {
		return nullptr;
	}
	i = p;
	Lexem *lex = new Number(number);
	return lex;
}

Lexem *is_variable(std::string codeline, int &i, int size) {
	std::string variable;
	while (i < size && ((codeline[i] >= 'A' && codeline[i] <= 'Z') || (codeline[i] >= 'a' && codeline[i] <= 'z')
		|| (codeline[i] >= '0' && codeline[i] <= '9') || codeline[i] == '_' )) {
		variable += codeline[i];
		i++;
	}
	if (variable.empty()) {
		return nullptr;
	}
	Lexem *lex = new Variable(variable);
	return lex;
}

std::vector<Lexem *> parse_lexem(std::string codeline) {
	std::string::size_type comment = codeline.find('#');
	if (comment != std::string::npos) {
		codeline = codeline.substr(0, comment); // the rest of the line is a note to the reader
	}
	codeline = codeline + ' ';
	int size = codeline.size();
	std::vector<Lexem *> infix;
	Lexem *lex;
	int i = 0;
	try {
		while (i < size) {
			lex = is_oper(codeline, i, size);
			if (lex != nullptr) {
				// a minus with nothing on its left is a sign, not a subtraction
				if (((Oper *)lex) -> get_type() == MINUS && (infix.empty()
						|| (dynamic_cast<Oper *>(infix.back())
							&& ((Oper *)infix.back()) -> get_type() != RBRACKET))) {
					((Oper *)lex) -> set_type(UNARY_MINUS);
				}
				infix.push_back(lex);
				continue;
			}
			if (i == size) {
				break;
			}
			lex = is_number(codeline, i, size);
			if (lex != nullptr) {
				infix.push_back(lex);
				continue;
			}
			lex = is_variable(codeline, i, size);
			if (lex != nullptr) {
				infix.push_back(lex);
				continue;
			} else {
				throw (INVALID_SYNTAX);
			}
		}
	} catch (ERRORS e) {
		for (auto &parsed: infix) { // nothing of a broken line is kept
			delete parsed;
		}
		throw (e);
	}
	return infix;
}

void init_labels(std::vector<std::vector<Lexem *>> &infix) {
	std::string current_function;
	for (int row = 0; row < (int)infix.size(); ++row) {
		int size = infix[row].size();
		for (int i = 0; i < size; ++i) {
			if (infix[row][i] == nullptr || !(dynamic_cast<Oper *>(infix[row][i]))) {
				continue;
			}
			OPERATOR type = ((Oper *)infix[row][i]) -> get_type();
			if (type == FUNCTION && i + 1 < size && (dynamic_cast<Variable *>(infix[row][i + 1]))) {
				current_function = ((Variable *)infix[row][i + 1]) -> get_name();
			} else if (type == ENDFUNCTION) {
				current_function.clear();
			}
		}
		for (int i = 1; i < size; ++i) {
			if (infix[row][i - 1] == nullptr || infix[row][i] == nullptr) {
				continue;
			}
			if (!(dynamic_cast<Variable *>(infix[row][i - 1])) || !(dynamic_cast<Oper *>(infix[row][i]))) {
				continue;
			}
			if (((Oper *)infix[row][i]) -> get_type() != COLON) {
				continue;
			}
			// a label lives in the function it stands in: two functions may reuse
			// a name, and a goto can no longer land in a body that is not running
			Variable *lexemvar = (Variable *)infix[row][i - 1];
			Goto::ltable[current_function][lexemvar -> get_name()] = row;
			delete infix[row][i - 1];
			delete infix[row][i];
			infix[row][i - 1] = nullptr;
			infix[row][i] = nullptr;
			i++;
		}
	}
}

void init_loops(std::vector<std::vector<Lexem *>> &infix) {
	std::stack<bool> is_it_if_condition;
	int size = infix.size();
	std::stack<Goto *> loop_stack;
	for (int row = 0; row < size; row++) {
		int length = infix[row].size();
		for (int i = 0; i < length; i++) {
			if (infix[row][i] == nullptr) {
				continue;
			}
			if (dynamic_cast<Oper *>(infix [row][i])) {
				Oper *lexemoper = (Oper *)infix[row][i];
				if (lexemoper -> get_type() == IF) {
					if (!(dynamic_cast<Oper *>(infix[row].back())) || 
					(((Oper *)infix[row].back()) -> get_type() != COLON)) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_NO_COLONS_IN_CONDITION);
					}
					delete infix[row].back();
					infix[row].pop_back();
					length--;
					loop_stack.push((Goto *)lexemoper);
					is_it_if_condition.push(true);
				}
				if (lexemoper -> get_type() == WHILE) {
					if (!(dynamic_cast<Oper *>(infix[row].back())) || 
					(((Oper *)infix[row].back()) -> get_type() != COLON)) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_NO_COLONS_IN_CONDITION);
					}
					delete infix[row].back();
					infix[row].pop_back();
					length--;
					Goto *lexemgoto = (Goto *)lexemoper;
					lexemgoto -> set_row(row);
					loop_stack.push(lexemgoto);
					is_it_if_condition.push(false);
				}
				if (lexemoper -> get_type() == ELSE) {
					if (is_it_if_condition.empty() || !(is_it_if_condition.top())) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_WITH_CONDITIONAL_OPERATORS);
					}
					if (!(dynamic_cast<Oper *>(infix[row].back())) || 
					(((Oper *)infix[row].back()) -> get_type() != COLON)) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_NO_COLONS_IN_CONDITION);
					}
					delete infix[row].back();
					infix[row].pop_back();
					length--;
					loop_stack.top() -> set_row(row + 1);
					loop_stack.pop();
					loop_stack.push((Goto *)lexemoper);
				}
				if (lexemoper -> get_type() == ENDLOOP) {
					if (is_it_if_condition.empty()) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_WITH_CONDITIONAL_OPERATORS);
					}
					if (!(is_it_if_condition.top())) {
						Goto *lexemgoto = (Goto*)lexemoper;
						lexemgoto -> set_row(loop_stack.top() -> get_row());
					} else {
						Goto *lexemgoto = (Goto*)lexemoper;
						lexemgoto -> set_row(row + 1);
					}
					loop_stack.top() -> set_row(row + 1);
					loop_stack.pop();
					is_it_if_condition.pop();
				}
				if (lexemoper -> get_type() == ENDFUNCTION && !(is_it_if_condition.empty())) {
					std::cerr << "problems in " << row + 1 << " string:\n";
					throw (ERR_WRONG_POSITION_FUNCTION);
				}
			}
		}
	}
	if (!loop_stack.empty()) {
		throw (ERR_NOT_BALANCED_CONDITIONAL_OPERATORS);
	}
}

void init_function_start_vars(std::vector<Lexem *> &infix, int &i, std::map<std::string, std::vector<std::string>> &func_var_table) {
	int length = infix.size();
	Function *function = (Function *)infix[i];
	i += 2;
	bool correct_func = false;
	int start_var_counter = 0, point = i;
	while (i < length) {
		if (infix[i] && dynamic_cast<Oper *>(infix[i]) 
			&& (((Oper *)infix[i]) -> get_type() == RBRACKET)) {
			correct_func = true;
			break;
		}
		if (dynamic_cast<Function *>(infix[i])) {
			init_function_start_vars(infix, i, func_var_table);
			continue;
		}
		if (dynamic_cast<Oper *>(infix[i])
		 	&& (((Oper *)infix[i]) -> get_type() == COMMA)) {
			start_var_counter++;
		}
		i++;
	}
	if (start_var_counter || i > point) {
		start_var_counter++;
	}
	i++;
	if (!correct_func || start_var_counter != function -> get_num_of_start_vars()) {
		throw (ERR_UNDEFINED_FUNCTION);
	}
	for (auto &j: func_var_table[function -> get_name()]) {
		function -> add_var(j);
	}
}

void init_functions(std::vector<std::vector<Lexem *>> &infix) {
	std::stack<int> err_function_stack;
	std::map<std::string, int> f_st_var;
	std::map<std::string, std::vector<std::string>> func_var_table;
	for (int row = 0; row < (int)infix.size(); row++) {
		int length = infix[row].size();
		for (int i = 0; i < length; i++) {
			if (infix[row][i] == nullptr) {
				continue;
			}
			if (dynamic_cast<Oper *>(infix[row][i])) { 
				if (((Oper *)infix[row][i]) -> get_type() == FUNCTION) {
					if ((i + 1 >= length) || !(dynamic_cast<Variable *>(infix[row][i + 1]))) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_NAME_FUNCTION);
					}
					std::string function_name = ((Variable *)infix[row][i + 1]) -> get_name();
					Function::ftable[function_name] = row + 1;
					delete infix[row][i];
					delete infix[row][i + 1];
					infix[row][i] = nullptr;
					infix[row][i + 1] = nullptr;
					i += 2;
					while (i < length && !infix[row][i]) {
						i++;
					}
					if (i >= length || !dynamic_cast<Oper *>(infix[row][i]) || 
							((Oper *)infix[row][i]) -> get_type() != LBRACKET || 
											!dynamic_cast<Oper *>(infix[row].back())
					 							|| ((Oper *)infix[row].back()) -> get_type() != RBRACKET) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_FUNCTION_START_VARS);						
					}
					int start_point_to_del = i, start_var_counter = 0;
					i++;
					length--;
					bool oper0_var1 = true;
					while (i < length) {
						if (oper0_var1 && dynamic_cast<Variable *>(infix[row][i])) {
							std::string param = ((Variable *)infix[row][i]) -> get_name();
							std::vector<std::string> &params = func_var_table[function_name];
							if (std::find(params.begin(), params.end(), param) != params.end()) {
								std::cerr << "problems in " << row + 1 << " string:\n";
								throw (ERR_FUNCTION_START_VARS);
							}
							params.push_back(param);
							start_var_counter++;
							oper0_var1 = false;
						} else if (!oper0_var1 && dynamic_cast<Oper *>(infix[row][i]) 
							 && (((Oper *)infix[row][i]) -> get_type() == COMMA)) {
							oper0_var1 = true;
						} else {
							std::cerr << "problems in " << row + 1 << " string:\n";
							throw (ERR_FUNCTION_START_VARS);
						}
						i++;
					}
					f_st_var[function_name] = start_var_counter;
					length++;
					for (int j = start_point_to_del; j < length; j++) {
						delete infix[row].back();
						infix[row].pop_back();
					}
					if (!err_function_stack.empty()) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_NOT_BALANCED_FUNCTION);
					}
					err_function_stack.push(row);
				} else if (((Oper *)infix[row][i]) -> get_type() == ENDFUNCTION) {
					for (int j = i; j < length; j++) {
						delete infix[row].back();
						infix[row].pop_back();
					}
					length = infix[row].size(); // the tail is gone, do not scan it again
					if (err_function_stack.empty()) {
						std::cerr << "problems in " << row + 1 << " string:\n";
						throw (ERR_NOT_BALANCED_FUNCTION);
					}
					err_function_stack.pop();
				} else if (((Oper *)infix[row][i]) -> get_type() == RETURN) {
					((Goto *)infix[row][i]) -> set_row(FUNCTION_RETURN);
				}
			}
		}
	}
	if (!err_function_stack.empty()) {
    	throw (ERR_NOT_BALANCED_FUNCTION);
	}
	// calls are bound only once every definition is known, so a function may be
	// called from a row above the one that defines it
	for (int row = 0; row < (int)infix.size(); row++) {
		int length = infix[row].size();
		for (int i = 0; i + 1 < length; i++) {
			if (!infix[row][i] || !dynamic_cast<Variable *>(infix[row][i])) {
				continue;
			}
			if (!dynamic_cast<Oper *>(infix[row][i + 1])
					|| ((Oper *)infix[row][i + 1]) -> get_type() != LBRACKET) {
				continue;
			}
			std::string var_name = ((Variable *)infix[row][i]) -> get_name();
			if (Function::ftable.find(var_name) == Function::ftable.end()) {
				continue;
			}
			Function *function = new Function(var_name, Function::ftable[var_name], f_st_var[var_name]);
			delete infix[row][i];
			infix[row][i] = function;
			// the name is stepped over, the bracket is not: a call may stand
			// in the first argument of this one
		}
	}
	for (int row = 0; row < (int)infix.size(); row++) {
		int length = infix[row].size();
		for (int i = 0 ; i < length; ) {
			if (infix[row][i] == nullptr) {
				i++;
				continue;
			}
			if (dynamic_cast<Function *>(infix[row][i])) {	
				try {
					init_function_start_vars(infix[row], i, func_var_table);
				} catch (ERRORS e) {
					std::cerr << "problems in " << row + 1 << " string:\n";
					throw (e);
				}
				continue;
			} else {
				i++;
			}
		}
	}
}

std::vector<Lexem *> build_postfix(std::vector<Lexem *> infix) {
	std::vector<Lexem *> lexem_stack;
	std::vector<Oper *> operator_stack;
	for (auto &element: infix) {
		if (element == nullptr) {
			continue;
		}
		if (Number *ptr = dynamic_cast<Number *>(element)) {
			lexem_stack.push_back(element);
			continue;
		}
		if (Variable *ptr = dynamic_cast<Variable *>(element)) {
			lexem_stack.push_back(element);
			continue;
		}
		OPERATOR operatortype = ((Oper *)element) -> get_type();
		if (operator_stack.empty() || operatortype == LBRACKET) {
			operator_stack.push_back((Oper *)element);
			continue;
		}
		if (operatortype == RBRACKET) {
			if (operator_stack.empty()) {
				throw (ERR_NOT_BALANCED_BRACKETS);
			}
			while ((operator_stack.back()) -> get_type() != LBRACKET ) {
				lexem_stack.push_back(operator_stack.back());
				operator_stack.pop_back();
				if (operator_stack.empty()) {
					throw (ERR_NOT_BALANCED_BRACKETS);
				}
			}
			operator_stack.pop_back();
			if (!operator_stack.empty() &&(operator_stack.back()) -> get_type() == FUNCTION) {
				lexem_stack.push_back(operator_stack.back());
				operator_stack.pop_back();
			}
			continue;
		}
		if (operatortype == COMMA) {
			while (!operator_stack.empty() && (operator_stack.back()) -> get_type() != LBRACKET) {
				lexem_stack.push_back(operator_stack.back());
				operator_stack.pop_back();
			}
			if (operator_stack.empty()) {
				throw (ERR_NOT_BALANCED_BRACKETS);
			}
			continue;
		}
		if (operatortype == UNARY_MINUS) {
			operator_stack.push_back((Oper *)element); // right associative, so - - x works
			continue;
		}
		if (operatortype == ASSIGN && ASSIGN == (operator_stack.back()) -> get_type()) {
			operator_stack.push_back((Oper *)element);
			continue;
		}
		if (operatortype != FUNCTION && ((Oper *)element) -> get_priority() <= (operator_stack.back()) -> get_priority()) {
			// only operators that really bind tighter may leave before this one,
			// otherwise 2 + 3 * 4 * 5 would fold the plus away too
			while (!(operator_stack.empty()) && ((operator_stack.back()) -> get_priority() > 0)
					&& ((operator_stack.back()) -> get_priority() >= ((Oper *)element) -> get_priority())) {
				lexem_stack.push_back(operator_stack.back());
				operator_stack.pop_back();
			}
			operator_stack.push_back((Oper *)element);
		} else {
			operator_stack.push_back((Oper *)element);
		}
	}
	while (!operator_stack.empty()) {
		if ((operator_stack.back()) -> get_type() == LBRACKET) {
			throw (ERR_NOT_BALANCED_BRACKETS); // an opening bracket nobody closed
		}
		lexem_stack.push_back(operator_stack.back());
		operator_stack.pop_back();
	}
	return lexem_stack;
}

// every temporary Number an expression produced, released in one place
void clear_numbers(std::vector<Number *> &numbers) {
	for (auto &n: numbers) {
		if (n) {
			delete n;
		}
	}
	numbers.clear();
}

int read_operand(Lexem *lex, Function *function) {
	if (dynamic_cast<Number *>(lex)) {
		return ((Number *)lex) -> get_value();
	}
	Variable *var = (Variable *)lex;
	if (!(var -> has_value(function))) {
		throw (ERR_UNDEFINED_VARIABLE);
	}
	return var -> get_value(function);
}

int get_value(Lexem *leftlex, Lexem *rightlex, Function **function, OPERATOR opertype) {
	if (opertype == ASSIGN) {
		if (!dynamic_cast<Variable *>(leftlex)) {
			throw (ERR_ASSIGN_PROBLEMS);
		}
		int tmp = read_operand(rightlex, *function); // the left name is written, so it is not read
		((Variable *)leftlex) -> set_value(*function, tmp);
		return tmp;
	}
	int left = read_operand(leftlex, *function);
	int right = read_operand(rightlex, *function);
	int answer;
	switch (opertype) {
			case PLUS:
				if (__builtin_add_overflow(left, right, &answer)) {
					throw (ERR_INTEGER_OVERFLOW);
				}
				return answer;
			case MINUS:
				if (__builtin_sub_overflow(left, right, &answer)) {
					throw (ERR_INTEGER_OVERFLOW);
				}
				return answer;
			case MULT:
				if (__builtin_mul_overflow(left, right, &answer)) {
					throw (ERR_INTEGER_OVERFLOW);
				}
				return answer;
			case DIV:
				if (right == 0) {
					throw (ERR_DIVISION_BY_ZERO);
				}
				if (left == INT32_MIN && right == -1) {
					throw (ERR_INTEGER_OVERFLOW);
				}
				return left / right;
			case MOD:
				if (right == 0) {
					throw (ERR_DIVISION_BY_ZERO);
				}
				if (left == INT32_MIN && right == -1) {
					throw (ERR_INTEGER_OVERFLOW);
				}
				return left % right;
			case OR:
				return left || right;
			case AND:
				return left && right;
			case BITOR:
				return left | right;
			case XOR:
				return left ^ right;
			case BITAND:
				return left & right;
			case EQ:
				return left == right;
			case NEQ:
				return left != right;
			case LEQ:
				return left <= right;
			case LT:
				return left < right;
			case GEQ:
				return left >= right;
			case GT:
				return left > right;
			case SHL: {
				if (right < 0 || right >= 32) {
					throw (ERR_BAD_SHIFT_COUNT);
				}
				// a multiplication instead of a shift: shifting a negative
				// number, or past the width of the type, is undefined
				long long wide = (long long)left * (1LL << right);
				if (wide > INT32_MAX || wide < INT32_MIN) {
					throw (ERR_INTEGER_OVERFLOW);
				}
				return (int)wide;
			}
			case SHR:
				if (right < 0 || right >= 32) {
					throw (ERR_BAD_SHIFT_COUNT);
				}
				return left >> right;
			default:
				break;
	}
	return 0;
}

int evaluate_postfix(std::vector<std::vector<Lexem *>> &postfix, int row, Function **function_field, Number **result) {
	std::vector<Lexem *> stack;
	std::vector<Number *> need_to_clear;
	Lexem *res;
	int i = 0;
	bool has_lexems = false;
	for (auto &element: postfix[row]) {
		if (element != nullptr) {
			has_lexems = true;
			break;
		}
	}
	if (!has_lexems) { // blank lines and lines holding only a label carry no code
		*result = nullptr;
		return row + 1;
	}
	for (auto &element: postfix[row]) {
		if (DEBUG) {
			std::cout << i++ << '\n';
			std::cout << "row = " << row + 1 << std::endl;
			std::cout << "\n**********************************\n" << "Stack<" << stack.size() << ">:\n";
			print_lexems(stack);
			std::cout << "\n----------------------------------\n";
		}
		if (element == nullptr) {
			continue;
		}
		if (dynamic_cast<Number *>(element)) {
			stack.push_back(element);
			continue;
		}
		if (dynamic_cast<Variable *>(element)) {
			stack.push_back(element); // a name comes into being by assignment, not by being mentioned
			continue;
		}
		if (dynamic_cast<Function *>(element)) {
			Function call_frame = *((Function *)element); // the lexem is shared, the frame must not be
			Function *function_elem = &call_frame;
			int num = function_elem -> get_num_of_start_vars();
			while (num--) {
				if (stack.empty()) {
					clear_numbers(need_to_clear);
					throw (ERR_UNDEFINED_FUNCTION);
				}
				if (stack.back() == nullptr) {
					clear_numbers(need_to_clear);
					throw (ERR_VOID_FUNCTION_IN_EVALUATION);
				}
				function_elem -> set_start_var(read_operand(stack.back(), *function_field), num);
				stack.pop_back();
			}
			int func_row = ((Goto *)element) -> get_row();
			Number *func_result = nullptr;
			if (DEBUG) {
				std::cout << "function " << function_elem -> get_name() << ":\n";
			}
			while (0 <= func_row && func_row < (int)postfix.size()) {
				delete func_result; // only the value of the last line is kept
				func_result = nullptr;
				try {
					func_row = evaluate_postfix(postfix, func_row, &function_elem, &func_result);
				} catch (ERRORS e) {
					delete func_result;
					clear_numbers(need_to_clear);
					std::cerr << "function " << function_elem -> get_name() << " problems in " << func_row + 1 << " string:\n";
					throw (e);
				}
			}
			if (DEBUG) {
				std::cout << "function " << function_elem -> get_name() << " finished\n";
			}
			stack.push_back(func_result);
			if (func_result) {
				need_to_clear.push_back(func_result);
			}
		} else {
			OPERATOR operatortype = ((Oper *)element) -> get_type();
			if (operatortype == GOTO) {
				if (stack.empty() || !dynamic_cast<Variable *>(stack.back())) {
					clear_numbers(need_to_clear);
					throw (ERR_WITH_GOTO_OR_LABELS);
				}
				Variable *label = (Variable *)stack.back();
				stack.pop_back();
				clear_numbers(need_to_clear);
				if (!stack.empty()) {
					throw (ERR_EXTRA_VALUES_IN_LINE);
				}
				std::map<std::string, int> &labels = Goto::ltable[(*function_field) -> get_name()];
				if (labels.find(label -> get_name()) == labels.end()) {
					// a label of another function is not reachable, and an unknown
					// one would otherwise send the jump to row 0 forever
					throw (ERR_WITH_GOTO_OR_LABELS);
				}
				*result = nullptr;
				return labels[label -> get_name()];
			}
			if (operatortype == ELSE || operatortype == ENDLOOP) {
				clear_numbers(need_to_clear);
				if (!stack.empty()) {
					throw (ERR_EXTRA_VALUES_IN_LINE);
				}
				*result = nullptr;
				return ((Goto *)element) -> get_row();
			}
			if (operatortype == IF || operatortype == WHILE) {
				if (stack.empty()) {
					clear_numbers(need_to_clear);
					throw (ERR_NOT_ENOUGH_OPERANDS);
				}
				int jump;
				if (dynamic_cast<Number *>(stack.back())) {
					jump = ((Number *)stack.back()) -> get_value();
				} else if (dynamic_cast<Variable *>(stack.back())){
					jump = read_operand(stack.back(), *function_field);
				} else {
					ERRORS problem = (stack.back() == nullptr)
						? ERR_VOID_FUNCTION_IN_EVALUATION : ERR_NOT_ENOUGH_OPERANDS;
					clear_numbers(need_to_clear);
					throw (problem);
				}
				stack.pop_back();
				clear_numbers(need_to_clear);
				if (!stack.empty()) {
					throw (ERR_EXTRA_VALUES_IN_LINE);
				}
				*result = nullptr; // the caller has already deleted the previous one
				if (!jump) {
					return ((Goto *)element) -> get_row();
				}
				return row + 1;
			}
			if (operatortype == RETURN) {
				if (stack.empty()) {
					(*result) = nullptr;
				} else {
					int tmp;
					if (dynamic_cast<Number *>(stack.back())) {
						tmp = ((Number *)stack.back()) -> get_value();
					} else if (dynamic_cast<Variable *>(stack.back())){
						tmp = read_operand(stack.back(), *function_field);
					} else {
						ERRORS problem = (stack.back() == nullptr)
							? ERR_VOID_FUNCTION_IN_EVALUATION : ERR_NOT_ENOUGH_OPERANDS;
						clear_numbers(need_to_clear);
						throw (problem);
					}
					*result = new Number(tmp);
					stack.pop_back();
					if (!stack.empty()) {
						clear_numbers(need_to_clear);
						throw (ERR_EXTRA_VALUES_IN_LINE);
					}
				}
				clear_numbers(need_to_clear); // the result is a fresh Number, not one of these
				return ((Goto *)element) -> get_row();
			}
			if (operatortype == UNARY_MINUS) {
				if (stack.empty()) {
					clear_numbers(need_to_clear);
					throw (ERR_NOT_ENOUGH_OPERANDS);
				}
				res = stack.back();
				stack.pop_back();
				if (res == nullptr) {
					clear_numbers(need_to_clear);
					throw (ERR_VOID_FUNCTION_IN_EVALUATION);
				}
				int negated = read_operand(res, *function_field);
				if (negated == INT32_MIN) {
					clear_numbers(need_to_clear);
					throw (ERR_INTEGER_OVERFLOW);
				}
				res = new Number(-negated);
				stack.push_back(res);
				need_to_clear.push_back((Number *)res);
				continue;
			}
			if (stack.empty()) {
				clear_numbers(need_to_clear);
				throw (ERR_NOT_ENOUGH_OPERANDS);
			}
			res = stack.back();
			stack.pop_back();
			if (stack.empty()) {
				clear_numbers(need_to_clear);
				throw (ERR_NOT_ENOUGH_OPERANDS);
			}
			if (res == nullptr || stack.back() == nullptr) {
				clear_numbers(need_to_clear);
				throw(ERR_VOID_FUNCTION_IN_EVALUATION);
			}
			res = new Number(get_value(stack.back(), res, function_field, operatortype));
			stack.pop_back();
			stack.push_back(res);
			need_to_clear.push_back((Number *)res);
		}
	}
	if (DEBUG) {
		std::cout << "\n**********************************\n" << "Stack<" << stack.size() << ">:\n";
		print_lexems(stack);
		std::cout << "\n----------------------------------\n";
	}
	if (!stack.empty()) {
		int tmp;
		if (!stack.back()) {
			*result = nullptr;
		} else {
			if (dynamic_cast<Number *>(stack.back())) {
				tmp = ((Number *)stack.back()) -> get_value();
			} else if (dynamic_cast<Variable *>(stack.back())){
				tmp = read_operand(stack.back(), *function_field);
			} else {
				ERRORS problem = (stack.back() == nullptr)
					? ERR_VOID_FUNCTION_IN_EVALUATION : ERR_NOT_ENOUGH_OPERANDS;
				clear_numbers(need_to_clear);
				throw (problem);
			}
			*result = new Number(tmp);
		}
		stack.pop_back();
	} else {
		throw (ERR_NOT_ENOUGH_OPERANDS);
	}
	clear_numbers(need_to_clear);
	if (!stack.empty()) {
		throw (ERR_EXTRA_VALUES_IN_LINE);
	}
	return row + 1;
}