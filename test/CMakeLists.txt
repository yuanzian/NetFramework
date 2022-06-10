
add_executable(test_browse)
target_sources(test_browse PRIVATE
    "browse.cpp"
)

target_link_libraries(test_browse NetworkModule)
target_include_directories(test_browse PRIVATE 
    "." ".."
)