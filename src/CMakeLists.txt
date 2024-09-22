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

target_include_directories(${TARGET} PRIVATE "D:\\AA\\project\\my_project\\third_library\\curl-master\\winx64\\libcurl-vc2022-x64-debug-dll-ssl-static\\include")
if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LIBCURL_NAME "D:\\AA\\project\\my_project\\third_library\\curl-master\\winx64\\libcurl-vc2022-x64-debug-dll-ssl-static")
        target_link_libraries(${TARGET} PRIVATE "${LIBCURL_NAME}/lib/libcurl_debug.lib")
        add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                "${LIBCURL_NAME}/bin/libcurl_debug.dll"
                "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
        )
    else()
        set(LIBCURL_NAME "D:\\AA\\project\\my_project\\third_library\\curl-master\\winx64\\libcurl-vc22-x64-release-dll-ssl-static")
        target_link_libraries(${TARGET} PRIVATE "${LIBCURL_NAME}/lib/libcurl.lib")
        add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                "${LIBCURL_NAME}/bin/libcurl.dll"
                "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
        )
    endif()
elseif (UNIX)
    target_link_libraries(${TARGET} PRIVATE pthread)
    target_link_libraries(${TARGET} PRIVATE ${LIBCURL_PATH}/lib64/libcurl.so)
endif ()

INSTALL(TARGETS ${TARGET} RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}../install)
