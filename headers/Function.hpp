#ifndef FUNCTION_HPP
#define FUNCTION_HPP

#include <string>
#include <map>
#include <vector>
#include "Goto.hpp"

class Function: public Goto {
	std::string f_name;
	std::map<std::string, int> vtable;
	std::vector<std::string> params;
	int num_of_start_vars = 0;
public:
	Function();
	Function(std::string name, int row, int n);
	~Function();
	int get_num_of_start_vars();
	std::string get_name();
	void add_var(std::string var_name);
	void set_start_var(int answer, int i);
	void print_function_vars();
	int check_var(std::string var_name);
	void set_value(std::string var_name, int value);
	int get_value(std::string var_name);
	virtual void print();
	static std::map<std::string, int> ftable;
};

#endif