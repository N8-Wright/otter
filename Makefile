bootstrap:
	cc -o otter_bootstrap otter_make.c otter_target.c otter_allocator.c otter_logger.c otter_cstring.c otter_filesystem.c otter_file.c -lcrypto
	./otter_bootstrap
	rm ./otter_bootstrap

otter:
	./otter_make
