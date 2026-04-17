default:
    @just --list

# run some_arg='':
#     @mkdir -p build
#     @clang src/main.c -o build/main
#     @./build/main {{ some_arg }}
# test:
#     @mkdir -p build/test
#     clang test/*.c test/munit/*.c -o build/test/main-clang
#     ./build/test/main-clang
# Auto-detect C/C++ compiler commands

CC := `if [ -f src/main.cpp ]; then echo 'clang++ -std=c++20'; else echo 'clang -std=c11'; fi`
GCC := `if [ -f src/main.cpp ]; then echo 'g++ -std=c++20'; else echo 'gcc -std=c11'; fi`

# Common flags

FLAGS := "-Wall -Wextra -Wpedantic -O2 -I src"

# Easy run for clang. Same as run-clang task
clang: build-clang run-clang

# Easy run for gcc. Same as run-gcc task
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

debug-clang:
    @mkdir -p build/debug
    {{ CC }} {{ FLAGS }} -g -O0 -fsanitize=address src/main.* -o build/debug/main-clang

debug-gcc:
    @mkdir -p build/debug
    {{ GCC }} {{ FLAGS }} -g -O0 src/main.* -o build/debug/main-gcc

# diff:
#     @mkdir -p build
#     {{ CC }} {{ FLAGS }} src/main.* -o build/tmp-clang && ./build/tmp-clang > build/clang.out
#     {{ GCC }} {{ FLAGS }} src/main.* -o build/tmp-gcc && ./build/tmp-gcc > build/gcc.out
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
    @mkdir -p test/munit
    curl -L https://raw.githubusercontent.com/nemequ/munit/refs/heads/master/munit.h -o test/munit/munit.h
    curl -L https://raw.githubusercontent.com/nemequ/munit/refs/heads/master/munit.c -o test/munit/munit.c

self-fmt:
    just --unstable --fmt
    just --unstable --fmt --check
