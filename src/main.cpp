#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include "interpreter.hpp"
#include "Number.hpp"
#include "Variable.hpp"
#include "Goto.hpp"
#include "Function.hpp"
#include "enum.hpp"

enum {EXIT_BAD_USAGE = 2};

void usage(const char *program) {
	std::cerr << "usage: " << program << " [--verbose] [program.txt]\n"
		<< "       " << program << " [--verbose] < program.txt\n";
}

void dump_lines(std::vector<std::vector<Lexem *>> &lines, const char *what) {
	for (int i = 0; i < (int)lines.size(); ++i) {
		std::cout << std::endl << i + 1 << ": size of " << what << " = " << lines[i].size() << std::endl;
		print_lexems(lines[i]);
		std::cout << "\n----------------------------------\n";
	}
	std::cout << "##################################\n";
	std::cout << "##################################\n";
}

int main(int argc, char** argv) {
	const char *path = nullptr;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--verbose") == 0) {
			DEBUG = true;
		} else if (argv[i][0] == '-' && argv[i][1] != '\0') {
			std::cerr << "unknown option " << argv[i] << std::endl;
			usage(argv[0]);
			return EXIT_BAD_USAGE;
		} else if (path == nullptr) {
			path = argv[i];
		} else {
			std::cerr << "only one program can be given, got " << path
				<< " and " << argv[i] << std::endl;
			usage(argv[0]);
			return EXIT_BAD_USAGE;
		}
	}
	std::ifstream program;
	if (path != nullptr) {
		program.open(path);
		if (!program) {
			std::cerr << "cannot open " << path << std::endl;
			return EXIT_BAD_USAGE;
		}
	}
	std::istream &source = (path != nullptr) ? program : std::cin;
	// a prompt only makes sense when a person is typing the program in
	INTERACTIVE = (path == nullptr) && isatty(STDIN_FILENO);

	std::string codeline;
	std::vector<std::vector<Lexem *>> infixlines, postfixlines;
	int exit_code = 0;
	try {
		while (true) {
			if (INTERACTIVE) {
				std::cout << "> " << std::flush;
			}
			if (!getline(source, codeline)) {
				break;
			}
			try {
				infixlines.push_back(parse_lexem(codeline));
			} catch (ERRORS e) {
				std::cerr << "problems in " << infixlines.size() + 1 << " string:\n";
				throw (e);
			}
		}
		if (INTERACTIVE) {
			std::cout << std::endl;
		}
		init_loops(infixlines);
		init_labels(infixlines);
		init_functions(infixlines);
		if (DEBUG) {
			dump_lines(infixlines, "infix");
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
			dump_lines(postfixlines, "postfix");
		}
		Number *res = nullptr;
		if (Function::ftable.find("main") == Function::ftable.end()) {
			throw (ERR_UNDEFINED_MAIN);
		}
		int row = Function::ftable["main"];
		Function main_frame("main", row, 0); // the frame outlives the loop, nothing to free
		Function *frame = &main_frame;
		while (0 <= row && row < (int)postfixlines.size()) {
			try {
				row = evaluate_postfix(postfixlines, row, &frame, &res);
				if (res != nullptr) {
					std::cout << ">>>>>>" << (res -> get_value());
					delete res;
					res = nullptr; // the next line must not see a freed number
				}
				std::cout << std::endl;
			} catch (ERRORS e) {
				delete res;
				std::cerr << "problems in " << row + 1 << " string:\n";
				throw (e);	
			}
		}
	} catch (ERRORS e) {
		std::cerr << ERRORTEXT[int(e)] << std::endl;
		exit_code = 1; // a failed program must not look successful to the shell
	}
	for (int i = 0; i < (int)infixlines.size(); i++) {
		for (auto &cl: infixlines[i]) {
			if (cl) {
				delete cl;
			}
		}
	}
	return exit_code;
}
