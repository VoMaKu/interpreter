#include "Number.hpp"
#include <iostream>

Number::Number(): value(0) {}

Number::Number(int smthg) {
	value = smthg; 
}

Number::~Number () {}

void Number::set_value(int smthg) {
	value = smthg;
}

int Number::get_value() {
	return value;
}

void Number::print() {
		std::cout << '[' << value << "] ";
}