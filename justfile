_default:
    @just --list

build_dir := "build"
src_dir := "src"
test_dir := "tests"
munit_dir := test_dir / "munit"

run some_arg='':
    @mkdir -p {{ build_dir }}
    @clang {{ src_dir }}/main.c -o {{ build_dir }}/main
    @./{{ build_dir }}/main {{ some_arg }}

fmt_justfile:
    just --unstable --fmt
    just --unstable --fmt --check

download_munit:
    @mkdir -p {{ munit_dir }}
    curl -L https://raw.githubusercontent.com/nemequ/munit/refs/heads/master/munit.h -o {{ munit_dir }}/munit.h
    curl -L https://raw.githubusercontent.com/nemequ/munit/refs/heads/master/munit.c -o {{ munit_dir }}/munit.c


test:
    clang {{test_dir}}/*.c {{munit_dir}}/*.c -o build/test_runner
    ./build/test_runner
