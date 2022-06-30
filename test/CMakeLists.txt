add_executable(test_browse)
target_sources(test_browse PRIVATE
    "test_browse.cpp"
)

target_link_libraries(test_browse NetworkModule)
target_include_directories(test_browse PRIVATE 
    "." ".."
    "../third_party" 

    #pupnp
    "../third_party/pupnp/ixml/inc" 
    "../third_party/pupnp/upnp/inc"
    "../third_party/pupnp/upnp/src/inc"
    "${PUPNP_BINARY_DIR}"
    "${PUPNP_BINARY_DIR}/upnp/inc"
    "../third_party/pupnp/upnp/src/threadutil"
    "${PTHREADS4W_SOURCE_DIR}"

    #libdsm
    "../third_party/libdsm/include"
    "../third_party/libdsm/include/bdsm"
    "$<$<BOOL:${MSVC}>:${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libdsm/include/msvc>"

    #libsmb2
    "../third_party/libsmb2/include"
    "../third_party/libsmb2/include/smb2"
    "$<$<BOOL:${MSVC}>:${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libsmb2/include/msvc>"
)

add_executable(test_log)
target_sources(test_log PRIVATE
    "test_log.cpp"
)
target_link_libraries(test_log NetworkModule)
target_include_directories(test_log PRIVATE
    ".."
)