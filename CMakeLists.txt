cmake_minimum_required(VERSION 3.13)
project(HMDA)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -Werror")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

include_directories(hmda ${CMAKE_SOURCE_DIR}/buildit/include)
set(STAGED
		hmda/staged/format.h
	    hmda/staged/fwd.h
	    hmda/staged/hmda.h
	    hmda/staged/traits.h
	    hmda/staged/utils.h)

link_directories(${CMAKE_SOURCE_DIR}/buildit/build)
add_compile_definitions(HMDA_VERBOSE)

#add_executable(driver1 ${STAGED} drivers/driver1.cpp)
#target_link_libraries(driver1 buildit)

add_executable(simple simple.cpp)
target_link_libraries(simple buildit)
