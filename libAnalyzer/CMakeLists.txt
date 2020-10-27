
if(${ROOT_FOUND})
    add_executable(analyzer_example analyzer_example.cxx)
    target_link_libraries(analyzer_example PUBLIC rootana)
endif()
