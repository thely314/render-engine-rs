cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")

project(Render)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(DEPS ${CMAKE_SOURCE_DIR}/dep)

file(TO_CMAKE_PATH "$ENV{MKLROOT}" MKL_ROOT)
include_directories(${MKL_ROOT}/include)
set(MKL_LIBS
    ${MKL_ROOT}/lib/mkl_intel_lp64.lib
    ${MKL_ROOT}/lib/mkl_sequential.lib
    ${MKL_ROOT}/lib/mkl_core.lib
)
add_definitions(-DEIGEN_USE_MKL_ALL)
# set(ASSIMP_STATIC_LIBRARY ${CMAKE_SOURCE_DIR}/lib/gcc/libassimp.dll.a)
# set(ASSIMP_SHARED_LIBRARY ${CMAKE_SOURCE_DIR}/lib/gcc/libassimp-5.dll)  
set(ASSIMP_STATIC_LIBRARY ${CMAKE_SOURCE_DIR}/lib/clang/assimp.lib)  

set(HEADER ${CMAKE_SOURCE_DIR}/Header)
file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/Source/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCE})
target_compile_options(${PROJECT_NAME} PUBLIC -O3 -march=native -fno-math-errno -fno-trapping-math -flto -D_ITERATOR_DEBUG_LEVEL=0 -D_HAS_ITERATOR_DEBUGGING=0)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/exec)
target_link_libraries(${PROJECT_NAME} PUBLIC ${ASSIMP_STATIC_LIBRARY} PUBLIC ${MKL_LIBS})
target_include_directories(${PROJECT_NAME} PUBLIC ${DEPS} PUBLIC ${HEADER})