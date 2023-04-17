#!/bin/bash

set -e

echo clang++ -Wno-tautological-compare -O3 -std=c++17 -I../ -I. -I../../buildit/include -I../../buildit/build/gen_headers/ -L../../buildit/build -lbuildit test.cpp -o test
clang++ -O3 -std=c++17 -I../ -I. -I../../buildit/include -I../../buildit/build/gen_headers/ -L../../buildit/build -lbuildit test.cpp -o test

echo ./test
./test

echo clang -O3 -std=c++17 -I../ -I. my_func.cpp -o driver
clang -O3 -std=c++17 -I../ -I. driver.cpp my_func.cpp -o driver

echo ./driver
./driver
