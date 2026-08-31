#ifndef OPER_HPP
#define OPER_HPP

#include "Lexem.hpp"
#include "enum.hpp"

class Oper: public Lexem {
	OPERATOR opertype;
  public:
	Oper();
	Oper(OPERATOR opertype);
	virtual ~Oper();
	void set_type(OPERATOR optype);
	OPERATOR get_type();
	int get_priority();
	virtual void print();
};

#endif