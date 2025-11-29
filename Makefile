bootstrap:
	cc -g -o otter_bootstrap otter_make.c otter_target.c otter_allocator.c otter_logger.c otter_cstring.c otter_filesystem.c otter_file.c -lgnutls
	./otter_bootstrap
	rm ./otter_bootstrap

otter:
	./otter_make

format:
	clang-format *.c *.h -i

clean:
	rm *.o

clean-all: clean
	rm otter_make
