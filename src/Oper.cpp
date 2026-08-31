#include <iostream>
#include <string>
#include "Lexem.hpp"
#include "Oper.hpp"
#include "Number.hpp"
#include "Variable.hpp"
#include "enum.hpp"

Oper::Oper(): opertype(LBRACKET) {}

Oper::Oper(OPERATOR opertype) {
	Oper::opertype = opertype;
}

Oper::~Oper() {}

void Oper::set_type(OPERATOR optype) {
	opertype = optype;
}

OPERATOR Oper::get_type() {
	return opertype;
};

int Oper::get_priority() {
	return PRIORITY[(int)opertype];
}

void Oper::print() {
	std::cout << '|' << OPERTEXT[(int)opertype] << "| ";
}