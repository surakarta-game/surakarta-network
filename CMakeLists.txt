cmake_minimum_required(VERSION 3.16)

project(surakarta-network)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# add_compile_options(-fsanitize=address)
# add_compile_options(-g)
# add_link_options(-fsanitize=address)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Do not enable -Wall -Wextra due to the third-party code
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # To disable a false positive warning in libsockpp
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-stringop-truncation")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Do not enable -Wall -Wextra due to the third-party code
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /w14640")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
endif()

add_subdirectory(third-party/network-framework)
add_subdirectory(third-party/surakarta-core)

if(NOT TARGET surakarta-network)
    SET(SOURCE
        src/surakarta_agent_remote.cpp
        src/surakarta_network_service.cpp
        src/message.cpp
        src/socket_log_wrapper.cpp
    )
    if(WIN32)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
    add_library(surakarta-network SHARED ${SOURCE})
    target_include_directories(surakarta-network
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/private-include
    )
    target_link_libraries(surakarta-network PUBLIC network-framework)
    target_link_libraries(surakarta-network PRIVATE surakarta)
    install(TARGETS surakarta-network)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(surakarta-network PRIVATE -Wall -Wextra)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(surakarta-network PRIVATE /W4 /w14640)
    endif()
endif()

if(NOT TARGET surakarta-server)
    add_executable(surakarta-server src/server.cpp)
    target_link_libraries(surakarta-server PRIVATE surakarta-network)
    target_link_libraries(surakarta-server PRIVATE surakarta)
    install(TARGETS surakarta-server)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(surakarta-server PRIVATE -Wall -Wextra)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(surakarta-server PRIVATE /W4 /w14640)
    endif()
endif()

if(NOT TARGET surakarta-network-client)
    add_executable(surakarta-network-client src/client.cpp)
    target_link_libraries(surakarta-network-client PRIVATE surakarta-network)
    target_link_libraries(surakarta-network-client PRIVATE surakarta)
    install(TARGETS surakarta-network-client)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(surakarta-network-client PRIVATE -Wall -Wextra)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(surakarta-network-client PRIVATE /W4 /w14640)
    endif()
endif()

if(NOT TARGET surakarta-network-test)
    add_executable(surakarta-network-test src/test.cpp)
    target_link_libraries(surakarta-network-test PRIVATE surakarta-network)
    target_link_libraries(surakarta-network-test PRIVATE surakarta)
    add_test(NAME surakarta-network-test COMMAND surakarta-network-test)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(surakarta-network-test PRIVATE -Wall -Wextra)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(surakarta-network-test PRIVATE /W4 /w14640)
    endif()
endif()

if(NOT TARGET surakarta-submission-server)
    add_executable(surakarta-submission-server submission/Server_Team_3.cpp)
    target_link_libraries(surakarta-submission-server PRIVATE surakarta-network)
    target_link_libraries(surakarta-submission-server PRIVATE surakarta)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(surakarta-submission-server PRIVATE -Wall -Wextra)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(surakarta-submission-server PRIVATE /W4 /w14640)
    endif()
endif()

if(NOT TARGET surakarta-submission-client)
    add_executable(surakarta-submission-client submission/Client_Team_3.cpp)
    target_link_libraries(surakarta-submission-client PRIVATE surakarta-network)
    target_link_libraries(surakarta-submission-client PRIVATE surakarta)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(surakarta-submission-client PRIVATE -Wall -Wextra)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(surakarta-submission-client PRIVATE /W4 /w14640)
    endif()
endif()
