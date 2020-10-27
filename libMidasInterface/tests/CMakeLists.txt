
add_executable(test_mvodb test_mvodb.cxx)
target_link_libraries(test_mvodb PUBLIC rootana)

if(${ROOT_FOUND} AND ${HAVE_MIDAS})
    add_executable(testODB testODB.cxx)
    target_link_libraries(testODB PUBLIC rootana)
endif()
