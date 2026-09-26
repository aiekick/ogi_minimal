# TARGET FIRST, DIRECTORY AFTER. a dependency is a TARGET : when a parent
# project already provides glad, that one wins and nothing is brought
# twice ; otherwise we bring it from the vendored archive ; and when the
# archive is missing too, the configure refuses by name instead of
# failing later on an unknown target
if(NOT TARGET glad)
    set(GLAD_ARCHIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/glad.tar.gz)
    if(NOT EXISTS ${GLAD_ARCHIVE_PATH})
        message(FATAL_ERROR "the glad target is missing of the archive is not available")
    endif()
    include(FetchContent)
    FetchContent_Declare(glad
        URL ${GLAD_ARCHIVE_PATH}
    	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(glad)

    add_library(glad STATIC
        ${glad_SOURCE_DIR}/src/glad.c
        ${glad_SOURCE_DIR}/include/glad/glad.h
    )

    target_include_directories(glad PUBLIC
        ${glad_SOURCE_DIR}/include
    )

    set_target_properties(glad PROPERTIES FOLDER 3rdparty)
    set_target_properties(glad PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
