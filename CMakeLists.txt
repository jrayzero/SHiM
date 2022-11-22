cmake_minimum_required(VERSION 3.13)
project(HMDA)
set(CMAKE_VERBOSE_MAKEFILE ON)

# clang++ -O3 -S -emit-llvm -I/Users/jray/CLionProjects/HMDA-BuildIt/hmda -I/Users/jray/CLionProjects/HMDA-BuildIt/hmda/unstaged -I/Users/jray/CLionProjects/HMDA-BuildIt/hmda/staged -I/Users/jray/CLionProjects/HMDA-BuildIt/buildit/include -std=c++17 ../examples/example1.cpp -o example.ll
# other flags
# TODO use the build options that Ajay pointed me to

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -ferror-limit=5")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DBOUNDS_CHECK")

include_directories(hmda hmda/unstaged hmda/staged ${CMAKE_SOURCE_DIR}/buildit/include ${CMAKE_SOURCE_DIR}/buildit/build/gen_headers)
link_directories(${CMAKE_SOURCE_DIR}/buildit/build)

function(example name)
    add_executable(${name} examples/${name}.cpp)
    target_link_libraries(${name} buildit)
endfunction()

#add_executable(simple simple.cpp)
#target_link_libraries(simple buildit)
example(example0)
example(example1)
example(example2)
example(example3)


add_executable(jpegc apps/jpeg/jpeg.cpp)
