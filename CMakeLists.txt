cmake_minimum_required(VERSION 3.13)
project(loader_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 允许手动指定 binutils 位置（可选）
if (BFD_ROOT)
  set(BFD_HINTS ${BFD_ROOT} ${BFD_ROOT}/lib ${BFD_ROOT}/include)
endif()

# 找库
find_library(BFD_LIB bfd HINTS ${BFD_HINTS})
find_library(DL_LIB dl)
find_library(Z_LIB z)
if (NOT BFD_LIB)
  message(FATAL_ERROR "libbfd not found. Install 'binutils-dev' or set -DBFD_ROOT.")
endif()

# 找头文件
find_path(BFD_INCLUDE bfd.h
  HINTS ${BFD_HINTS}
  PATH_SUFFIXES include include/x86_64-linux-gnu)
if (NOT BFD_INCLUDE)
  message(FATAL_ERROR "bfd.h not found. Install 'binutils-dev'.")
endif()

include_directories(${BFD_INCLUDE} inc)

# 用绝对路径列出源码，避免相对路径歧义
set(SRC_LOADER ./src/loader_bfd.cpp)
set(SRC_DEMO   ./tools/loader_demo.cpp)

foreach(SRC ${SRC_LOADER} ${SRC_DEMO})
  if(NOT EXISTS ${SRC})
    message(FATAL_ERROR "Missing source: ${SRC}")
  endif()
endforeach()

add_executable(loader_demo
  ${SRC_LOADER}
  ${SRC_DEMO}
)

target_link_libraries(loader_demo
  ${BFD_LIB}
  ${DL_LIB}
  ${Z_LIB}
)