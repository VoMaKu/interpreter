#include <string>
#include "enum.hpp"

int PRIORITY[] = { 
	-1, -1,
	0,
	0, 0, 0,
	0, 0,
	0, 0,
	0, 0, 0,
	1,
	2,
	3,
	4,
	5,
	6, 6,
	8, 8,
	7, 7,
	7, 7,
	9, 9,
	10, 10, 10,
	11
};

std::string OPERTEXT[] = {
	"(", ")",
	",",
	"def", "return", "fed",
	"if", "else",
	"while", "@",
	"goto", ":=", ":",
	"or",
	"and",
	"|",
	"^",
	"&",
	"==", "!=",
	"<<", ">>",
	"<=", "<",
	">=", ">",
	"+", "-",
	"*", "/", "%",
	"-" // must stay after MINUS so that is_oper never matches it
};