all: example_all_urls example_all_types

debug:
	g++ -Wall -ggdb -g -DDEBUG -std=c++11 examples/snss-parse-lib.cpp -o parse-util

example_all_urls:
	g++ -Wall -o parse-all-urls examples/parse_print_urls.cpp

example_all_types:
	g++ -Wall -o parse-all-types examples/parse_print_types.cpp

