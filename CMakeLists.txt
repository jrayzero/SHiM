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
set(CMAKE_CXX_FLAGS "-std=c++17 -Wno-sign-compare -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -Wno-c++11-narrowing")
else()
set(CMAKE_CXX_FLAGS "-std=c++17 -Wno-sign-compare -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -Wno-c++11-narrowing -fopenmp")
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
 add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/sjpeg_v${ver}.cpp ${CMAKE_BINARY_DIR}/sjpeg_v${ver}.hpp
                    COMMAND ${CMAKE_BINARY_DIR}/sjpegc_v${ver} ARGS sjpeg_v${ver}
                    # make it depend on the binary since cmake is dump and won't let me just depend on the target...
                    DEPENDS ${CMAKE_BINARY_DIR}/sjpegc_v${ver})

 # and build it with the unstaged part
 add_executable(jpeg_v${ver} ${CMAKE_BINARY_DIR}/sjpeg_v${ver}.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/jpeg.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/huffman.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/syntax.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/staged/bits.cpp)
 target_include_directories(jpeg_v${ver} PUBLIC ${CMAKE_SOURCE_DIR}/hmda
                                                ${CMAKE_SOURCE_DIR}/apps/jpeg/staged
                                                ${CMAKE_BINARY_DIR})
 target_compile_definitions(jpeg_v${ver} PUBLIC VERSION=${ver})
endfunction()

staged_jpeg(1)

#unstaged JPEG
add_executable(unstaged_jpeg ${CMAKE_SOURCE_DIR}/apps/jpeg/unstaged/ujpeg.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/unstaged/huffman.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/unstaged/syntax.cpp
                             ${CMAKE_SOURCE_DIR}/apps/jpeg/unstaged/bits.cpp)
target_compile_definitions(unstaged_jpeg PUBLIC -DUNSTAGED)
 target_link_libraries(unstaged_jpeg buildit) # still link this because I haven't #ifdef'd all buildit things
target_include_directories(unstaged_jpeg PUBLIC ${CMAKE_SOURCE_DIR}/cola
  ${CMAKE_SOURCE_DIR}/apps/jpeg/unstaged
  ${CMAKE_BINARY_DIR})


function (CoLaJM IS_CPP)
  if (${IS_CPP} EQUAL "0")
    set(SUFFIX "c")
    set(HSUFFIX "h")
    set(GEN_TARGET_NAME CoLaJMC_generator)
    set(TARGET_NAME CoLaJMC)
  else() 
    set(SUFFIX "cpp")
    set(HSUFFIX "hpp")
    set(GEN_TARGET_NAME CoLaJMCPP_generator)
    set(TARGET_NAME CoLaJMCPP)
  endif()
  add_executable(${GEN_TARGET_NAME}
    ${CMAKE_SOURCE_DIR}/apps/JM/CoLaJM.cpp
    ${CMAKE_SOURCE_DIR}/apps/JM/utils.cpp)
  target_link_libraries(${GEN_TARGET_NAME} buildit)
  target_include_directories(${GEN_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/JM/lencod/inc ${CMAKE_SOURCE_DIR}/JM/lcommon/inc)
  target_compile_definitions(${GEN_TARGET_NAME} PUBLIC IS_CPP=${IS_CPP})
  # now run the staged code
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/CoLaJM_generated.${SUFFIX} ${CMAKE_BINARY_DIR}/CoLaJM_generated.${HSUFFIX}
    COMMAND ${CMAKE_BINARY_DIR}/${GEN_TARGET_NAME}
    DEPENDS ${CMAKE_BINARY_DIR}/${GEN_TARGET_NAME})

  # build the generate cpp code   
  add_library(${TARGET_NAME} STATIC ${CMAKE_BINARY_DIR}/CoLaJM_generated.${SUFFIX})
  target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/JM/lencod/inc ${CMAKE_SOURCE_DIR}/JM/lcommon/inc)

endfunction()

CoLaJM(0)
CoLAJM(1)

# run "make test" to build all the tests
add_custom_target(tests)
function (tester name useDefaultDriver)
 # build the staged code
 add_executable(${name}_generator ${CMAKE_SOURCE_DIR}/tests/${name}.cpp)
 target_link_libraries(${name}_generator buildit)
 
 # now run the staged code
 add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${name}_generated.cpp ${CMAKE_BINARY_DIR}/${name}_generated.hpp
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
tester(test6 true)
tester(test7 true)
