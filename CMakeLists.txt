cmake_minimum_required(VERSION 3.13)
project(HMDA)
include(ctest)
set(CMAKE_VERBOSE_MAKEFILE ON)

# clang++ -O3 -S -emit-llvm -I/Users/jray/CLionProjects/HMDA-BuildIt/hmda -I/Users/jray/CLionProjects/HMDA-BuildIt/hmda/unstaged -I/Users/jray/CLionProjects/HMDA-BuildIt/hmda/staged -I/Users/jray/CLionProjects/HMDA-BuildIt/buildit/include -std=c++17 ../examples/example1.cpp -o example.ll
# other flags

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

if (${MACOSX})
set(CMAKE_CXX_FLAGS "-std=c++17 -Wno-sign-compare -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor")
else()
set(CMAKE_CXX_FLAGS "-std=c++17 -Wno-sign-compare -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -fopenmp")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DBOUNDS_CHECK")

include_directories(cola ${CMAKE_SOURCE_DIR}/buildit/include ${CMAKE_SOURCE_DIR}/buildit/build/gen_headers)
link_directories(${CMAKE_SOURCE_DIR}/buildit/build)

# staged jpeg
function (staged_jpeg ver)
 # build the staged code
 add_executable(sjpegc_v${ver} ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/sjpeg.cpp)
 target_link_libraries(sjpegc_v${ver} buildit)
 
 target_compile_definitions(sjpegc_v${ver} PUBLIC VERSION=${ver})
 # now run the staged code
 add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/sjpeg_v${ver}.cpp ${CMAKE_BINARY_DIR}/sjpeg_v${ver}.h
                    COMMAND ${CMAKE_BINARY_DIR}/sjpegc_v${ver} ARGS sjpeg_v${ver}
                    # make it depend on the binary since cmake is dump and won't let me just depend on the target...
                    DEPENDS ${CMAKE_BINARY_DIR}/sjpegc_v${ver})

 # and build it with the unstaged part
 add_executable(jpeg_v${ver} ${CMAKE_BINARY_DIR}/sjpeg_v${ver}.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/jpeg.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/huffman.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/syntax.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/bits.cpp )
 target_include_directories(jpeg_v${ver} PUBLIC ${CMAKE_SOURCE_DIR}/hmda
                                                ${CMAKE_SOURCE_DIR}/apps/jpeg/staged
                                                ${CMAKE_BINARY_DIR})
 target_compile_definitions(jpeg_v${ver} PUBLIC VERSION=${ver})
endfunction()

staged_jpeg(1)
staged_jpeg(2)

function (CoLaJM)
 set(name CoLaJM)
 add_executable(${name}_generator ${CMAKE_SOURCE_DIR}/apps/JM/${name}.cpp)
 target_link_libraries(${name}_generator buildit)
 target_include_directories(${name}_generator PUBLIC ${CMAKE_SOURCE_DIR}/JM/lencod/inc ${CMAKE_SOURCE_DIR}/JM/lcommon/inc)
 
 # now run the staged code
 add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${name}_generated.c ${CMAKE_BINARY_DIR}/${name}_generated.h
                    COMMAND ${CMAKE_BINARY_DIR}/${name}_generator
                    DEPENDS ${CMAKE_BINARY_DIR}/${name}_generator)

 # build the generate cpp code 		    	    
 add_library(${name} STATIC ${CMAKE_BINARY_DIR}/${name}_generated.c)

endfunction()

CoLaJM()

# run "make test" to build all the tests
add_custom_target(tests)
function (tester name useDefaultDriver)
 # build the staged code
 add_executable(${name}_generator ${CMAKE_SOURCE_DIR}/tests/${name}.cpp)
 target_link_libraries(${name}_generator buildit)
 
 # now run the staged code
 add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${name}_generated.cpp ${CMAKE_BINARY_DIR}/${name}_generated.h
                    COMMAND ${CMAKE_BINARY_DIR}/${name}_generator
                    DEPENDS ${CMAKE_BINARY_DIR}/${name}_generator)

 # and build it with the unstaged part
 if (${useDefaultDriver})
   add_executable(${name} ${CMAKE_SOURCE_DIR}/tests/test_driver.cpp ${CMAKE_BINARY_DIR}/${name}_generated.cpp)                        
 else()
   add_executable(${name} ${CMAKE_SOURCE_DIR}/tests/${name}_driver.cpp ${CMAKE_BINARY_DIR}/${name}_generated.cpp)                        
 endif()
 target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/hmda ${CMAKE_BINARY_DIR})
 target_compile_definitions(${name} PUBLIC WHICH_TEST=${name})
 add_test(NAME ${name} COMMAND ${name})
 add_dependencies(tests ${name})
endfunction()

tester(test0 true)
tester(test1 true)
tester(test2 false)
tester(test3 false)
tester(test4 true)
tester(test5 true)
