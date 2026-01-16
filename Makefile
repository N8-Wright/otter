bootstrap:
	cc -g -fsanitize=address -o otter_bootstrap otter_make.c otter_target.c otter_allocator.c otter_logger.c otter_cstring.c otter_filesystem.c otter_file.c otter_array.c -lgnutls
	./otter_bootstrap

.PHONY: otter

otter:
	./otter_make --debug

cstring_tests: otter
	./otter_test ./otter_cstring_tests.so
	./otter_test ./otter_cstring_tests_coverage.so

array_tests: otter
	./otter_test ./otter_array_tests.so
	./otter_test ./otter_array_tests_coverage.so

lexer_tests: otter
	./otter_test ./otter_lexer_tests.so
	./otter_test ./otter_lexer_tests_coverage.so

parser_tests: otter
	./otter_test ./otter_parser_tests.so
	./otter_test ./otter_parser_integration_tests.so
	./otter_test ./otter_parser_tests_coverage.so
	./otter_test ./otter_parser_integration_tests_coverage.so

coverage: test
	@echo "Generating HTML coverage report with gcovr..."
	gcovr --html --html-details -o coverage-report.html .
	@echo "HTML coverage report generated: coverage-report.html"

test: cstring_tests array_tests lexer_tests parser_tests coverage

format:
	clang-format *.c *.h -i

clean:
	rm -rf *.o *.html *.css *.gcov *.gcda *.gcno *.gz *.out *.so

clean-all: clean
	rm -rf otter_make otter_test
