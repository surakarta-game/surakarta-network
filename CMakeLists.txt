cmake_minimum_required(VERSION 3.16)

project(surakarta-network)

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
endif()

if(NOT TARGET surakarta-server)
    add_executable(surakarta-server src/server.cpp)
    target_link_libraries(surakarta-server PRIVATE surakarta-network)
    target_link_libraries(surakarta-server PRIVATE surakarta)
endif()
