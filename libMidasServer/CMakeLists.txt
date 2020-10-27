
if(${HAVE_ROOT})
add_executable(test_midasServer test_midasServer.cxx)
target_link_libraries(test_midasServer PUBLIC rootana)
endif()
