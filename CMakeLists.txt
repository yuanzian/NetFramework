cmake_minimum_required(VERSION 3.16)

project(NetworkModule CXX)

include(CheckCXXCompilerFlag)
if(MSVC)
    CHECK_CXX_COMPILER_FLAG("/std:c++20" COMPILER_SUPPORTS_CXX20)
    CHECK_CXX_COMPILER_FLAG("/utf-8" COMPILER_SUPPORTS_UTF8)
else()
    CHECK_CXX_COMPILER_FLAG("-std=c++20" COMPILER_SUPPORTS_CXX20)
endif()


if(COMPILER_SUPPORTS_CXX20)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CLANG_DEFAULT_CXX_STDLIB "libc++")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++20 support. Please use a different C++ compiler.")
endif()

if(COMPILER_SUPPORTS_UTF8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no utf-8 support. Please use a different C++ compiler.")
endif()

add_subdirectory("third_party/pupnp")
add_subdirectory("third_party/libdsm")
add_subdirectory("third_party/libsmb2")

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE 
    "source/protocol/protocol.cpp"
    "source/protocol/protocol_dlna.cpp"
    "source/protocol/protocol_smb.cpp"
    "source/NetworkModule.cpp"
    "source/protocol/rules.cpp"
)

target_link_libraries(${PROJECT_NAME} UPNP::Static)
target_link_libraries(${PROJECT_NAME} dsm smb2)
target_compile_definitions (${PROJECT_NAME}
    PUBLIC $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
    PUBLIC $<$<BOOL:${MSVC}>:UPNP_USE_MSVCPP>
    PUBLIC $<$<BOOL:${MSVC}>:LIBUPNP_EXPORTS>
    PUBLIC $<$<BOOL:${MSVC}>:UNICODE _UNICODE>
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    "." 
    "third_party" 

    #pupnp
    "third_party/pupnp/ixml/inc" 
    "third_party/pupnp/upnp/inc"
    "third_party/pupnp/upnp/src/inc"
    "${PUPNP_BINARY_DIR}"
    "${PUPNP_BINARY_DIR}/upnp/inc"
    "third_party/pupnp/upnp/src/threadutil"
    "${PTHREADS4W_SOURCE_DIR}"

    #libdsm
    "third_party/libdsm/include"
    "third_party/libdsm/include/bdsm"
    "$<$<BOOL:${MSVC}>:${CMAKE_CURRENT_SOURCE_DIR}/third_party/libdsm/include/msvc>"

    #libsmb2
    "third_party/libsmb2/include"
    "third_party/libsmb2/include/smb2"
    "$<$<BOOL:${MSVC}>:${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsmb2/include/msvc>"
)

add_subdirectory (test)
enable_testing ()
add_test (NAME MyTest COMMAND Test)
