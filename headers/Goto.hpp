#ifndef GOTO_HPP
#define GOTO_HPP

#include <map>
#include <string>
#include "Oper.hpp"
#include "enum.hpp"

class Goto: public Oper {
	int row;
  public:
	Goto();
	Goto(OPERATOR optype);
	virtual ~Goto();
	void set_row(int row);
	int get_row();
	virtual void print();
	static std::map<std::string, std::map<std::string, int>> ltable;
};

#endif