cmake_minimum_required(VERSION 3.13)
project(HMDA)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -Werror")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

include_directories(core ${CMAKE_SOURCE_DIR}/buildit/include)
set(HDR core/format.h
	core/fwd.h
	core/hmda.h
	core/traits.h
	core/utils.h)

link_directories(${CMAKE_SOURCE_DIR}/buildit/build)


add_executable(driver1 ${HDR} drivers/driver1.cpp)
target_link_libraries(driver1 buildit)
