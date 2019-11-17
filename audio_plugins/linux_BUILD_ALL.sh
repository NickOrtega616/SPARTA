#!/bin/sh

# set aliases for prefered compilers and linker
alias gcc=/opt/AMD/aocc-compiler-2.0.0/bin/clang
alias g++=/opt/AMD/aocc-compiler-2.0.0/bin/clang++
alias ld=/opt/AMD/aocc-compiler-2.0.0/bin/ld64.lld

find -type f \( -name 'makefile' -o -name 'makefile' -o -name 'Makefile' \) \
-exec bash -c 'cd "$(dirname "{}")" && make CONFIG=Release -j4' \;

