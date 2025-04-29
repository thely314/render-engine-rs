cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")

project(Render)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(EIGEN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/eigen)
set(EIGEN_STATIC_LIBRARY ${CMAKE_SOURCE_DIR}/eigen_build/blas/libeigen_blas.dll.a)
set(EIGEN_SHARED_LIBRARY ${CMAKE_SOURCE_DIR}/eigen_build/blas/libeigen_blas.dll)

set(ASSIMP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/assimp-5.3.1/include )
set(ASSIMP_STATIC_LIBRARY ${CMAKE_SOURCE_DIR}/assimp_build/lib/libassimp.dll.a)
set(ASSIMP_SHARED_LIBRARY ${CMAKE_SOURCE_DIR}/assimp_build/bin/libassimp-5.dll)  

set(HEADER ${CMAKE_SOURCE_DIR}/Header)
file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/Source/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCE})
target_compile_options(${PROJECT_NAME} PUBLIC -O3)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/exec)
target_link_libraries(${PROJECT_NAME} PUBLIC ${EIGEN_STATIC_LIBRARY} PUBLIC ${EIGEN_SHARED_LIBRARY} PUBLIC ${ASSIMP_STATIC_LIBRARY} PUBLIC ${ASSIMP_SHARED_LIBRARY})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN_INCLUDE_DIR} PUBLIC ${ASSIMP_INCLUDE_DIR} PUBLIC ${HEADER})
# 在 Windows 上，如果使用动态库，确保能找到 DLL 文件
# 设置 DLL 的路径，这样运行时可以找到 GLFW32.dll
#if(WIN32)
#    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        "${CMAKE_SOURCE_DIR}/GLFW-2.2.0/bin/GLFW32.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
#endif()

