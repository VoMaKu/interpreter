#include <string>
#include <map>
#include <iostream>
#include "Variable.hpp"
#include "Function.hpp"

Variable::Variable() {}

Variable::Variable(std::string str) {
	v_name = str; 
}

Variable::~Variable() {}

std::string Variable::get_name() {
	return v_name;
}

bool Variable::has_value(Function *function) {
	return function -> has_var(v_name);
}

void Variable::set_value(Function *function, int num) {
	function -> set_value(v_name, num);
}

int Variable::get_value(Function *function) {
	return (function -> get_value(v_name));
}

void Variable::print() {
	std::cout << '[' << v_name << "] ";
}