# ========== 编译器选项 ==========
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /std:c++20")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /std:c20")
add_compile_definitions(_WIN32_WINNT=0x0A00)

# FetchContent
set(FETCHCONTENT_VERBOSE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
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

# ---------- abseil ----------
FetchContent_Declare(abseil
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG 20230802.2
)
FetchContent_MakeAvailable(abseil)

# ---------- sqlite3 ----------
find_package(SQLite3 REQUIRED)

# ---------- pugixml ----------
find_package(pugixml CONFIG REQUIRED)
# ---------- odb ----------
#find_package(odb CONFIG REQUIRED)
#find_package(libodb-sqlite CONFIG REQUIRED)
## 找到odb的编译器
#find_program(ODB_EXECUTABLE NAMES odb)
#message(STATUS "ODB_EXECUTABLE = ${ODB_EXECUTABLE}")
## 定义持久化对象头文件
#set(ODB_HEADERS
#        ${CMAKE_CURRENT_SOURCE_DIR}/cfl/db/person.h
#)
## 定义输出目录
#set(ODB_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/odb_gen)
#file(MAKE_DIRECTORY ${ODB_GEN_DIR})
#set(ODB_GEN_SOURCES
#        ${ODB_GEN_DIR}/person-odb.cxx
#)
#set(ODB_GEN_HEADERSA3
#        ${ODB_GEN_DIR}/person-odb.hxx
#)
#add_custom_command(
#        OUTPUT ${ODB_GEN_SOURCES} ${ODB_GEN_HEADERS}
#        COMMAND ${CMAKE_COMMAND} -E echo ">>> Running ODB compiler on ${ODB_HEADERS}"
#        COMMAND ${ODB_EXECUTABLE}
#        -d sqlite
#        --generate-query
#        --generate-schema
#        --output-dir ${ODB_GEN_DIR}
#        ${ODB_HEADERS}
#        DEPENDS ${ODB_HEADERS}
#        VERBATIM
#)
#
# ---------- protobuf ----------
find_package(protobuf CONFIG REQUIRED)
set(PROTO_FILES
        ${CMAKE_SOURCE_DIR}/cfl/protos/base.proto
        ${CMAKE_SOURCE_DIR}/cfl/protos/login_db.proto
        ${CMAKE_SOURCE_DIR}/cfl/protos/login.proto
        ${CMAKE_SOURCE_DIR}/cfl/protos/game.proto
        ${CMAKE_SOURCE_DIR}/cfl/protos/define.proto
)
set(PROTO_OUT_PUT_PATH ${CMAKE_SOURCE_DIR}/cfl/protos/gen_proto)
# 自动调用 protoc 生成
set(GENERATED_SRC)
set(GENERATED_HDR)

foreach (PROTO ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO} NAME_WE)

    set(SRC ${PROTO_OUT_PUT_PATH}/${PROTO_NAME}.pb.cc)
    set(HDR ${PROTO_OUT_PUT_PATH}/${PROTO_NAME}.pb.h)

    add_custom_command(
            OUTPUT ${SRC} ${HDR}
            COMMAND protobuf::protoc
            #            -I=${CMAKE_SOURCE_DIR}/cfl/protos
            --proto_path=${CMAKE_SOURCE_DIR}/cfl/protos
            --cpp_out=${PROTO_OUT_PUT_PATH}
            ${PROTO}
            DEPENDS ${PROTO}
    )

    list(APPEND GENERATED_SRC ${SRC})
    list(APPEND GENERATED_HDR ${HDR})
endforeach ()
# ---------- magic_enum ----------
#FetchContent_Declare(
#        magic_enum
#        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
#        GIT_TAG        v0.9.5   # 版本号，可以改成最新的 release
#)
#FetchContent_MakeAvailable(magic_enum)
# ---------- 源文件 ----------
set(LIB_SRC
        cfl/config.cc
        cfl/shm/shmpage.cc
        cfl/shm/shmpool.cc
        cfl/db/db_mysql.cc
        cfl/db/db_sqlite.cc
        cfl/playerobj.cc
        cfl/modules/module_base.cc
        cfl/connection.cc
        cfl/net_engine.cc
        cfl/buffer.cc
        cfl/modules/role_module.cc
        cfl/static_data.cc
        ${GENERATED_SRC}
)

add_library(cfl SHARED ${LIB_SRC})
target_compile_definitions(cfl PUBLIC CFL_EXPORTS)

target_link_libraries(cfl
        PUBLIC
        spdlog::spdlog
        yaml-cpp
        OpenSSL::SSL
        unofficial::mysql-connector-cpp::connector
        ws2_32
        mswsock
        absl::flat_hash_map
        SQLite::SQLite3
        #        odb
        #        odb-sqlite
        protobuf::libprotobuf
        #        protos
        #        magic_enum::magic_enum
        pugixml::shared
        pugixml::pugixml
)

target_include_directories(cfl PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${asio_SOURCE_DIR}/asio/include
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_BINARY_DIR}/include/mysqlx
        ${ODB_INCLUDE_DIRS}
        ${GENERATED_HDR}
)

# ---------- 测试 ----------
set(TEST_TARGETS
        test_log test_config test_asio test_asio_tcp
        test_asio_udp test_asio_async
        test_ssm_creator test_ssm_attacher test_mysql
        test_abseil test_role test_role2 test_role_sqlite
        test_role_creator test_role_attacher
        test_sqlite3 test_handler test_proto test_connection
        test_net_engine test_role_module test_static_data
)

foreach (target_name IN LISTS TEST_TARGETS)
    add_executable(${target_name}
            tests/${target_name}.cc
#            ${GENERATED_SRC}
    )
    target_link_libraries(${target_name} PRIVATE cfl)
    add_dependencies(${target_name} cfl)
endforeach ()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 自动复制 DLL
foreach (EXE_TARGET IN LISTS TEST_TARGETS)
    add_custom_command(TARGET ${EXE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${EXE_TARGET}> $<TARGET_FILE_DIR:${EXE_TARGET}>
            COMMAND_EXPAND_LISTS
    )
endforeach ()

# 复制 configs
foreach (EXE_TARGET IN LISTS TEST_TARGETS)
    add_custom_command(TARGET ${EXE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/configs
            $<TARGET_FILE_DIR:${EXE_TARGET}>/configs
    )
endforeach ()
