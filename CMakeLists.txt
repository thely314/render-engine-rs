cmake_minimum_required(VERSION 3.10)
#set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
#set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")

project(Render)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(EIGEN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/eigen)

set(ASSIMP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/assimp-5.3.1/include)
set(ASSIMP_STATIC_LIBRARY ${CMAKE_SOURCE_DIR}/assimp_build/lib/libassimp.dll.a)
set(ASSIMP_SHARED_LIBRARY ${CMAKE_SOURCE_DIR}/assimp_build/bin/libassimp-5.dll)  

set(HEADER ${CMAKE_SOURCE_DIR}/Header)
file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/Source/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCE})
target_compile_options(${PROJECT_NAME} PUBLIC -O3 -march=native)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/exec)
target_link_libraries(${PROJECT_NAME} PUBLIC ${ASSIMP_STATIC_LIBRARY} PUBLIC ${ASSIMP_SHARED_LIBRARY})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN_INCLUDE_DIR} PUBLIC ${ASSIMP_INCLUDE_DIR} PUBLIC ${HEADER})