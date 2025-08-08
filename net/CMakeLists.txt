cmake_minimum_required(VERSION 3.14)
project(cfl)
include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /std:c++17")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /std:c17")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -rdynamic -O3 -fPIC -ggdb -std=c17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
endif()
include(FetchContent)
include_directories(.)
# ---------- spdlog 配置 ----------
set(SPDLOG_INSTALL ON)          # 确保安装时包含 spdlog
set(SPDLOG_BUILD_SHARED ON)     # 构建动态库（若需静态库则设为 OFF）
set(SPDLOG_FMT_EXTERNAL OFF)     # 使用外部 fmt 库（避免内嵌 fmt 冲突）

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.0              # 指定稳定版本（如 v1.14.0）
)
FetchContent_MakeAvailable(spdlog)  # 自动下载并包含子目录
# ----------------------------------

set(LIB_SRC 
  cfl/main.cc)

add_library(cfl SHARED ${LIB_SRC})

set(LIBS 
    cfl
    spdlog::spdlog
)
cfl_add_executable(test_log "tests/test_log.cc" cfl "${LIBS}")

# target_compile_definitions(cfl PRIVATE CFL_EXPORTS)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)