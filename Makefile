bootstrap:
	cc -g -o otter_bootstrap otter_make.c otter_target.c otter_allocator.c otter_logger.c otter_cstring.c otter_filesystem.c otter_file.c -lgnutls
	./otter_bootstrap
	rm ./otter_bootstrap

otter:
	./otter_make

cstring_tests: otter
	./otter_test ./otter_cstring_tests.so

lexer_tests: otter
	./otter_test ./otter_lexer_tests.so

test: cstring_tests lexer_tests

coverage: test
	@echo "Generating HTML coverage report with gcovr..."
	gcovr --html --html-details -o coverage-report.html .
	@echo "HTML coverage report generated: coverage-report.html"

format:
	clang-format *.c *.h -i

clean:
	rm -rf *.o *.html *.css *.gcov *.gcda *.gcno *.out

clean-all: clean
	rm -rf otter_make otter_test
