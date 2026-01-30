bootstrap:
	mkdir -p release
	mkdir -p debug
	cc -g -fsanitize=address -o otter_make src/make.c src/target.c src/build.c src/allocator.c src/logger.c src/cstring.c src/filesystem.c src/file.c src/array.c src/string.c src/process_manager.c -lgnutls -I ./include

.PHONY: otter

otter:
	./otter_make --debug

otter_coverage:
	./otter_make --debug-coverage

cstring_coverage_tests: otter_coverage
	./debug/test_driver ./debug/cstring_tests_coverage.so

cstring_tests: otter
	./debug/test_driver ./debug/cstring_tests.so

array_coverage_tests: otter_coverage
	./debug/test_driver ./debug/array_tests_coverage.so

array_tests: otter
	./debug/test_driver ./debug/array_tests.so

string_coverage_tests: otter_coverage
	./debug/test_driver ./debug/string_tests_coverage.so

string_tests: otter
	./debug/test_driver ./debug/string_tests.so

lexer_coverage_tests: otter_coverage
	./debug/test_driver ./debug/lexer_tests_coverage.so

lexer_tests: otter
	./debug/test_driver ./debug/lexer_tests.so

parser_coverage_tests: otter_coverage
	./debug/test_driver ./debug/parser_tests_coverage.so
	./debug/test_driver ./debug/parser_integration_tests_coverage.so

parser_tests: otter
	./debug/test_driver ./debug/parser_tests.so
	./debug/test_driver ./debug/parser_integration_tests.so

build_coverage_tests: otter_coverage
	./debug/test_driver ./debug/build_tests_coverage.so
	./debug/test_driver ./debug/build_tests_extended_coverage.so
	./debug/test_driver ./debug/build_integration_tests_coverage.so

build_tests: otter
	./debug/test_driver ./debug/build_tests.so
	./debug/test_driver ./debug/build_tests_extended.so
	./debug/test_driver ./debug/build_integration_tests.so

target_coverage_tests: otter_coverage
	./debug/test_driver_coverage ./debug/target_tests_coverage.so
	./debug/test_driver_coverage ./debug/target_integration_tests_coverage.so

target_tests: otter
	./debug/test_driver ./debug/target_tests.so
	./debug/test_driver ./debug/target_integration_tests.so

coverage: coverage_tests
	@echo "Generating HTML coverage report with gcovr..."
	mkdir -p coverage
	gcovr --html --html-details -o ./coverage/coverage-report.html ./debug
	@echo "HTML coverage report generated: coverage-report.html"

coverage_tests: cstring_coverage_tests string_coverage_tests array_coverage_tests lexer_coverage_tests parser_coverage_tests build_coverage_tests target_coverage_tests
tests: cstring_tests string_tests array_tests lexer_tests parser_tests build_tests target_tests

format:
	clang-format ./src/*.c ./include/otter/*.h -i

clean:
	rm -rf ./debug/* ./release/* ./coverage

