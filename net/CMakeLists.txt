cmake_minimum_required(VERSION 3.14)
project(cfl)

# 在构建时显示完整编译命令，方便调试
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 编译器选项 ==========
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /std:c++20")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /std:c20")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++20 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -rdynamic -O3 -fPIC -ggdb -std=c20 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ggdb -std=c++20 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Wno-unknown-pragmas")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -ggdb -std=c20 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Wno-unknown-pragmas")
endif ()

if (WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif ()

# 启用 FetchContent（下载第三方依赖）
include(FetchContent)

# ---------- spdlog 配置 ----------
# 关闭外部 fmt（防止冲突）
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "Use external fmt library" FORCE)
# 禁用安装目标
set(SPDLOG_INSTALL OFF CACHE BOOL "Disable spdlog install" FORCE)
# DLL 模式下必须关闭 header-only
set(SPDLOG_HEADER_ONLY OFF CACHE BOOL "Disable header-only mode" FORCE)
# 开启 spdlog DLL 构建
set(SPDLOG_BUILD_SHARED ON CACHE BOOL "Build spdlog as shared library" FORCE)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.0
)

# ---------- yaml-cpp 配置 ----------
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Disable yaml-cpp tools" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "Disable yaml-cpp install" FORCE)
# 开启 yaml-cpp DLL 构建
set(YAML_CPP_BUILD_SHARED ON CACHE BOOL "Build yaml-cpp as shared library" FORCE)

FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
)

# ---------- asio 配置 ----------
FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2   # 最新稳定版
)

# ---------- Abseil 配置 ----------
FetchContent_Declare(
        abseil
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG 20230802.2  # 或指定某个版本号
)

# 下载并添加
FetchContent_MakeAvailable(spdlog yaml-cpp asio abseil)
# ---------- 源文件 ----------
set(LIB_SRC
        cfl/config.cc
)

# ---------- 构建 cfl 共享库 ----------
# 这里用 SHARED（DLL），运行时需要 cfl.dll
add_library(cfl SHARED ${LIB_SRC})

# 链接依赖库
target_link_libraries(cfl
        PUBLIC
        spdlog::spdlog
        yaml-cpp
        absl::flat_hash_map # 哈希表
)

if (WIN32)
    target_link_libraries(cfl PUBLIC ws2_32 mswsock)
endif ()

# 暴露 include 路径
target_include_directories(cfl
        PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${asio_SOURCE_DIR}/asio/include
)

# ---------- 测试程序 ----------
# 定义要构建的测试可执行文件列表
set(TEST_TARGETS
        test_log
        test_config
        test_asio
        test_asio_tcp
        test_asio_udp
        test_asio_async
        test_ssm_creator
        test_ssm_attacher
        test_abseil
)

# 循环添加可执行文件与链接库
foreach (target_name IN LISTS TEST_TARGETS)
    add_executable(${target_name} tests/${target_name}.cc)
    target_link_libraries(${target_name} PRIVATE cfl)
endforeach ()
# ---------- 输出目录 ----------
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# ---------- 自动复制依赖 DLL 到可执行文件目录（Windows 专用） ----------
# $<TARGET_RUNTIME_DLLS:target> 会收集 target 的所有运行时 DLL
# COMMAND_EXPAND_LISTS 让 cmake -E copy_if_different 能接受 DLL 列表
if (WIN32)
    foreach (EXE_TARGET IN LISTS TEST_TARGETS)
        add_custom_command(TARGET ${EXE_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${EXE_TARGET}> $<TARGET_FILE_DIR:${EXE_TARGET}>
                COMMAND_EXPAND_LISTS
        )
    endforeach ()
endif ()

# ------------------- 复制配置文件 ---------------------------------
foreach (EXE_TARGET IN LISTS TEST_TARGETS)
    add_custom_command(TARGET ${EXE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/configs
            $<TARGET_FILE_DIR:${EXE_TARGET}>/configs
    )
endforeach ()
