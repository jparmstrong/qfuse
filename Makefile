FUSE_CFLAGS := $(shell pkg-config --cflags fuse3)
FUSE_LIBS := $(shell pkg-config --libs fuse3)

CFLAGS_DEV := -g -O0 -Wall -Wextra -DDISABLE_DEBUG
CFLAGS_REL := -O3 -march=native -flto -fomit-frame-pointer -Wall -Wextra -DDISABLE_DEBUG

.PHONY: all
all: clean build
	@echo "FUSE CFLAGS: $(FUSE_CFLAGS)"
	@echo "FUSE LIBS: $(FUSE_LIBS)"
	cp -r $(QHOME) build/

clean:
	rm -rf build

build:
	mkdir -p build
	cc -std=c23 $(CFLAGS_DEV) $(FUSE_CFLAGS) -lpthread -o build/qfuse src/qfuse.c $(FUSE_LIBS)

build-release:
	mkdir -p build
	cc -std=c23 $(CFLAGS_REL) $(FUSE_CFLAGS) -lpthread -o build/qfuse src/qfuse.c $(FUSE_LIBS)


run: build
	mkdir -p vdb
	./build/qfuse config.csv -f vdb

# -- Debugging and Profiling --

valgrind: build
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--verbose \
		--log-file=./build/valgrind-out.txt ./build/qfuse config.csv -f mount

perf:
	perf record --call-graph dwarf -F 4000 -g ./build/qfuse config.csv -s -f mount
	perf script > perf.out
	../github/FlameGraph/stackcollapse-perf.pl perf.out > out.folded
	../github/FlameGraph/flamegraph.pl out.folded > flamegraph.svg