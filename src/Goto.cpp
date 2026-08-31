#include <iostream>
#include <map>
#include <string>
#include "interpreter.hpp"
#include "Goto.hpp"
#include "Variable.hpp"
#include "Oper.hpp"

Goto::Goto(): row(UNDEFINED) {}
    
Goto::Goto(OPERATOR optype): Oper (optype) { 
	row = UNDEFINED; 
}

Goto::~Goto() {}

void Goto::set_row(int row) {
	Goto::row = row;
}

int Goto::get_row() {
	return row;
}

void Goto::print() {
	std::cout << "<row " << row << '>';
	Oper::print(); 
}

std::map<std::string, std::map<std::string, int>> Goto::ltable;