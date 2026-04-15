_default:
    @just --list

build_dir := "build"

run:
    @mkdir -p {{ build_dir }}
    @clang main.c -o {{ build_dir }}/main
    @./{{ build_dir }}/main

fmt_justfile:
    just --unstable --fmt
    just --unstable --fmt --check
