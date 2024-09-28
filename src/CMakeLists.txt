set(TARGET github_hosts)

file(GLOB_RECURSE HEADER 
       "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE SOURCE 
       "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
set(ALL_FILES ${HEADER} ${SOURCE})
source_group_by_dir(ALL_FILES)
add_executable(${TARGET} ${ALL_FILES})

target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

link_and_install_libcurl()
INSTALL(TARGETS ${TARGET} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
