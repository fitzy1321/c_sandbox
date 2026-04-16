_default:
    @just --list

top-dir := justfile_directory()
build-dir := top-dir / "build"
src-dir := top-dir / "src"
test-dir := top-dir / "tests"
munit-dir := test-dir / "munit"

# run some-arg='':
#     @mkdir -p {{ build_dir }}
#     @clang {{ src_dir }}/main.c -o {{ build_dir }}/main
#     @./{{ build_dir }}/main {{ some-arg }}
# test:
#     clang {{ test_dir }}/*.c {{ munit_dir }}/*.c -o build/test_runner
#     ./build/test_runner
# Auto-detect C/C++ compiler commands

CC := `if [ -f src/main.cpp ]; then echo 'clang++ -std=c++20'; else echo 'clang -std=c11'; fi`
GCC := `if [ -f src/main.cpp ]; then echo 'g++ -std=c++20'; else echo 'gcc -std=c11'; fi`

# Common flags

FLAGS := "-Wall -Wextra -Wpedantic -O2 -I src"

clang: build-clang run-clang

gcc: build-gcc run-gcc

build-clang:
    @mkdir -p build
    {{ CC }} {{ FLAGS }} src/main.* -o build/main-clang

build-gcc:
    @mkdir -p build
    {{ GCC }} {{ FLAGS }} src/main.* -o build/main-gcc

run-clang: build-clang
    ./build/main-clang

run-gcc: build-gcc
    ./build/main-gcc

# Debug builds
debug-clang:
    @mkdir -p build
    {{ CC }} {{ FLAGS }} -g -O0 -fsanitize=address src/main.* -o build/main-clang-dbg

debug-gcc:
    @mkdir -p build
    {{ GCC }} {{ FLAGS }} -g -O0 src/main.* -o build/main-gcc-dbg

# diff:
#     @mkdir -p build
#     {{CC}} {{FLAGS}} src/main.* -o build/tmp-clang && ./build/tmp-clang > build/clang.out
#     {{GCC}} {{FLAGS}} src/main.* -o build/tmp-gcc && ./build/tmp-gcc > build/gcc.out
#     diff build/clang.out build/gcc.out || echo "⚠️ Outputs differ!"
#     rm -f build/tmp-* build/*.out

clean:
    rm -rf build
    mkdir -p build

# # Watch mode (requires entr or similar)
# # Not working right now
# watch:
#     ls src/*.c src/*.cpp | entr -c just test

test: build-clang build-gcc
    ./build/main-clang
    ./build/main-gcc
    echo "✅ Both compilers passed"

download-munit:
    @mkdir -p {{ munit-dir }}
    curl -L https://raw.githubusercontent.com/nemequ/munit/refs/heads/master/munit.h -o {{ munit-dir }}/munit.h
    curl -L https://raw.githubusercontent.com/nemequ/munit/refs/heads/master/munit.c -o {{ munit-dir }}/munit.c

self-fmt:
    just --unstable --fmt
    just --unstable --fmt --check
