bootstrap:
	mkdir -p release
	mkdir -p debug
	cc -g -fsanitize=address -o otter_make src/make.c src/target.c src/allocator.c src/logger.c src/cstring.c src/filesystem.c src/file.c src/array.c src/string.c -lgnutls -I ./include

.PHONY: otter

otter:
	./otter_make --debug

cstring_tests: otter
	./debug/test ./debug/cstring_tests.so
	./debug/test ./debug/cstring_tests_coverage.so

array_tests: otter
	./debug/test ./debug/array_tests.so
	./debug/test ./debug/array_tests_coverage.so

lexer_tests: otter
	./debug/test ./debug/lexer_tests.so
	./debug/test ./debug/lexer_tests_coverage.so

parser_tests: otter
	./debug/test ./debug/parser_tests.so
	./debug/test ./debug/parser_integration_tests.so
	./debug/test ./debug/parser_tests_coverage.so
	./debug/test ./debug/parser_integration_tests_coverage.so

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

