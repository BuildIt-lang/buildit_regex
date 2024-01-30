#include "compile.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <string>

const int T = 1;

int main(int argc, char* argv[]) {
    string regex(argv[1]);
	string schedule(argv[2]);
    string str;
	string str_filename(argv[3]);
	std::ifstream str_file(str_filename);
	str_file >> str;

    // default options (ignore_case = 0, interleaving_parts = 1, no special flags)
    RegexOptions default_options;
	default_options.flags = schedule;

	// Full

	double duration = 0;
	int is_match;

	for(int i = 0; i < T; i++) {
		auto t1 = std::chrono::high_resolution_clock::now();
		is_match = match(regex, str, default_options, MatchType::FULL);
		auto t2 = std::chrono::high_resolution_clock::now();
		duration += std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
	}

	std::cout << is_match << endl;
	//std::cout << duration/(T*1000) << endl;

//	// Partial
//
//	duration = 0;
//	string submatch;
//	default_options.binary = 0;
//
//	for(int i = 0; i < T; i++) {
//		auto t1 = std::chrono::high_resolution_clock::now();
//		is_match = match(regex, str, default_options, MatchType::PARTIAL_SINGLE, &submatch);
//		auto t2 = std::chrono::high_resolution_clock::now();
//		duration += std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
//	}
//
//	std::cout << is_match << endl;
//	std::cout << submatch << endl;
//	//std::cout << duration/(T*1000) << endl;
}

