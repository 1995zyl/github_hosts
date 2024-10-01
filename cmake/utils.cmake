macro(source_group_by_dir source_files)
    if(MSVC)
        set(sgbd_cur_dir ${CMAKE_CURRENT_SOURCE_DIR})
        foreach(sgbd_file ${${source_files}})
            string(REGEX REPLACE ${sgbd_cur_dir}/\(.*\) \\1 sgbd_fpath ${sgbd_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif(sgbd_nogroup)
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach(sgbd_file)
    endif(MSVC)
endmacro(source_group_by_dir)

macro(link_and_install_libcurl)
    if(WIN32)
        if(NOT DEFINED LIBCURL_PATH)
            message(FATAL_ERROR "LIBCURL_PATH is not defined!!!")
        endif()

        target_include_directories(${TARGET} PRIVATE "${LIBCURL_PATH}/include")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_link_libraries(${TARGET} PRIVATE "${LIBCURL_PATH}/lib/libcurl_debug.lib")
            add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                    "${LIBCURL_PATH}/bin/libcurl_debug.dll"
                    "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
            )
            INSTALL(FILES "${LIBCURL_PATH}/bin/libcurl_debug.dll" DESTINATION ${CMAKE_INSTALL_PREFIX})
        else()
            target_link_libraries(${TARGET} PRIVATE "${LIBCURL_PATH}/lib/libcurl.lib")
            add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                    "${LIBCURL_PATH}/bin/libcurl.dll"
                    "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
            )
            INSTALL(FILES "${LIBCURL_PATH}/bin/libcurl.dll" DESTINATION ${CMAKE_INSTALL_PREFIX})
        endif()
    elseif (UNIX)
        target_link_libraries(${TARGET} PRIVATE curl)
    endif ()
endmacro(link_and_install_libcurl)