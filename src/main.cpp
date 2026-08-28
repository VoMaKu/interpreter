#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include "interpreter.hpp"
#include "Number.hpp"
#include "Variable.hpp"
#include "Goto.hpp"
#include "Function.hpp"
#include "enum.hpp"

int main(int argc, char** argv) {
	for (int i = 1, flag = 0; i < argc; i++){
		if (!DEBUG && strcmp(argv[i], "--verbose") == 0) {
			DEBUG = true;
		}
		if (strstr(argv[i], ".txt") != nullptr) {
			if (strcmp(argv[i - 1], "<") == 0 || strcmp(argv[i - 1], "<<") == 0) 
				INTERACTIVE = false;
		}
	}
	std::string codeline;
	std::vector<std::vector<Lexem *>> infixlines, postfixlines;
	try {
		while (getline(std::cin, codeline)) {
			try {
				infixlines.push_back(parse_lexem(codeline));
			} catch (ERRORS e) {
				std::cout << "problems in " << infixlines.size() + 1 << " string:\n";
				throw (e);
			}
		}
		init_loops(infixlines);
		for (int row = 0; row < infixlines.size(); ++row) {
			try {
				init_labels(infixlines[row], row);
			} catch (ERRORS e) {
				std::cerr << "problems in " << row + 1 << " string:\n";
				throw (e);
			}
		}
		init_functions(infixlines);
		if (DEBUG) {
			for (int i = 0; i < infixlines.size(); ++i) {
				std::cout << std::endl << i + 1 << ": size of infix = " << infixlines[i].size() << std::endl;
				for (auto &debug: infixlines[i]) {
					if (debug == nullptr) {
						continue;
					}
					if (Number *ptr = dynamic_cast<Number *>(debug)) {
						((Number *)debug) -> print();
						continue;
					}
					if (Variable *ptr = dynamic_cast<Variable *>(debug)) {
						((Variable *)debug) -> print();
						continue;
					}
					if (Function *ptr = dynamic_cast<Function *>(debug)) {
						((Function *)debug) -> print();
						continue;
					}
					if (Goto *ptr = dynamic_cast<Goto *>(debug)) {
						((Goto *)debug) -> print();
					} else {
						((Oper *)debug) -> print();
					}
				}
				std::cout << "\n----------------------------------\n";
			}
			std::cout << "##################################\n";
			std::cout << "##################################\n";
		}
		for (const auto &infix: infixlines) {
			try {
				postfixlines.push_back(build_postfix(infix));
			} catch (ERRORS e) {
				std::cerr << "problems in " << postfixlines.size() + 1 << " string:\n";
				throw (e);	
			}
		}
		if (DEBUG) {
			for (int i = 0; i < postfixlines.size(); ++i) {
				std::cout << std::endl << i + 1 << ": size of postfix = " << postfixlines[i].size() << std::endl;
				for (auto &debug: postfixlines[i]) {
					if (debug == nullptr) {
						continue;
					}
					if (dynamic_cast<Number *>(debug)) {
						((Number *)debug) -> print();
						continue;
					}
					if (dynamic_cast<Variable *>(debug)) {
						((Variable *)debug) -> print();
						continue;
					}
					if (dynamic_cast<Function *>(debug)) {
						((Function *)debug) -> print();
						continue;
					}
					if (dynamic_cast<Goto *>(debug)) {
						((Goto *)debug) -> print();
					} else {
						((Oper *)debug) -> print();
					}
				}
				std::cout << "\n----------------------------------\n";
			}
			std::cout << "##################################\n";
			std::cout << "##################################\n";
		}
		Number *res = nullptr;
		if (Function::ftable.find("main") == Function::ftable.end()) {
            throw (ERR_UNDEFINED_MAIN);
        }
        int row = Function::ftable["main"];
        Function *main = new Function("main", row, 0);
		while (0 <= row && row < (int)postfixlines.size()) {
			try {
				row = evaluate_postfix(postfixlines, row, &main, &res);
				if (res != nullptr) {
					if (DEBUG || INTERACTIVE) {
						std::cout << ">>>>>>" <<(res -> get_value());
					}
					delete res;
					res = nullptr; // the next line must not see a freed number
				}
				std::cout << std::endl;
			} catch (ERRORS e) {
				std::cerr << "problems in " << row + 1 << " string:\n";
				throw (e);	
			}
		}
	} catch (ERRORS e) {
		std::cerr << ERRORTEXT[int(e)] << std::endl;
	}
	for (int i = 0; i < infixlines.size(); i++) {
		for (auto &cl: infixlines[i]) {
			if (cl) {
				delete cl;
			}
		}
	}
	return 0;
}

// ./interpreter filename.txt
// ./interpreter filename.txt --verbose
// ./interpreter --verbose