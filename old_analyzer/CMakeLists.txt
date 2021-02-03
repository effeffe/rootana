
add_executable(analyzer analyzer.cxx)
target_link_libraries(analyzer PUBLIC rootana)

add_executable(event_dump event_dump.cxx)
target_link_libraries(event_dump PUBLIC rootana)

add_executable(event_skim event_skim.cxx)
target_link_libraries(event_skim PUBLIC rootana)
