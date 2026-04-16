_default:
    @just --list

build_dir := "build"

run s_arg='':
    @mkdir -p {{ build_dir }}
    @clang main.c -o {{ build_dir }}/main
    @./{{ build_dir }}/main {{ s_arg }}

fmt_justfile:
    just --unstable --fmt
    just --unstable --fmt --check

download_munit:
    curl -L https://raw.githubusercontent.com/nemequ/munit/refs/heads/master/munit.h -o munit.h
