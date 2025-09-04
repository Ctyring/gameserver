# ========== 编译器选项 ==========
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++20 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Wno-unknown-pragmas")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -rdynamic -O3 -fPIC -ggdb -std=c20 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Wno-unknown-pragmas")

# FetchContent
set(FETCHCONTENT_VERBOSE ON)
include(FetchContent)

# ---------- spdlog ----------
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_HEADER_ONLY OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(spdlog)

# ---------- yaml-cpp ----------
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_SHARED ON CACHE BOOL "" FORCE)
FetchContent_Declare(yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)

# ---------- asio ----------
FetchContent_Declare(asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# ---------- mysql ----------
find_package(OpenSSL REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)

# ---------- 源文件 ----------
set(LIB_SRC
        cfl/config.cc
        cfl/shm/shmpage.cc
        cfl/shm/shmpool.cc
)

add_library(cfl SHARED ${LIB_SRC})

target_link_libraries(cfl
        PUBLIC
        spdlog::spdlog
        yaml-cpp
        OpenSSL::SSL
        unofficial::mysql-connector-cpp::connector
)

target_include_directories(cfl PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${asio_SOURCE_DIR}/asio/include
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_BINARY_DIR}/include/mysqlx
)

# ---------- 测试 ----------
set(TEST_TARGETS
        test_log test_config test_asio test_asio_tcp
        test_asio_udp test_asio_async
        test_ssm_creator test_ssm_attacher test_mysql
)

foreach (target_name IN LISTS TEST_TARGETS)
    add_executable(${target_name} tests/${target_name}.cc)
    target_link_libraries(${target_name} PRIVATE cfl)
endforeach ()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 复制 configs
foreach (EXE_TARGET IN LISTS TEST_TARGETS)
    add_custom_command(TARGET ${EXE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/configs
            $<TARGET_FILE_DIR:${EXE_TARGET}>/configs
    )
endforeach ()
