bootstrap:
	mkdir -p release
	mkdir -p debug
	cc -g -fsanitize=address -o otter_make src/otter_make.c src/otter_target.c src/otter_allocator.c src/otter_logger.c src/otter_cstring.c src/otter_filesystem.c src/otter_file.c src/otter_array.c -lgnutls -I ./include/otter

.PHONY: otter

otter:
	./otter_make --debug

cstring_tests: otter
	./debug/otter_test ./debug/otter_cstring_tests.so
	./debug/otter_test ./debug/otter_cstring_tests_coverage.so

array_tests: otter
	./debug/otter_test ./debug/otter_array_tests.so
	./debug/otter_test ./debug/otter_array_tests_coverage.so

lexer_tests: otter
	./debug/otter_test ./debug/otter_lexer_tests.so
	./debug/otter_test ./debug/otter_lexer_tests_coverage.so

parser_tests: otter
	./debug/otter_test ./debug/otter_parser_tests.so
	./debug/otter_test ./debug/otter_parser_integration_tests.so
	./debug/otter_test ./debug/otter_parser_tests_coverage.so
	./debug/otter_test ./debug/otter_parser_integration_tests_coverage.so

coverage: test
	@echo "Generating HTML coverage report with gcovr..."
	mkdir -p coverage
	gcovr --html --html-details -o ./coverage/coverage-report.html ./debug
	@echo "HTML coverage report generated: coverage-report.html"

test: cstring_tests array_tests lexer_tests parser_tests coverage

format:
	clang-format ./src/*.c ./include/otter/*.h -i

clean:
	rm -rf ./debug/* ./release/* ./coverage

