#ifndef VARIABLE_HPP
#define VARIABLE_HPP

#include <string>
#include <map>
#include "Lexem.hpp"
#include "Function.hpp"

class Variable: public Lexem {
	std::string v_name;
  public:
	Variable();
	Variable(std::string str);
	~Variable();
	std::string get_name();
	bool has_value(Function *function);
	void set_value(Function *function, int num);
	int get_value(Function *function);
	void print();
};

#endif