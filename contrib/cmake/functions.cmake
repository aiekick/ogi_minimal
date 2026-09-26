# builds a library from the sources of a vendored archive. the TARGET goes
# first : when a parent project already provides it, nothing is brought
# twice ; else the archive ; else a named refusal. FILES and INCLUDES are
# relative to the extracted root
# ex :
# add_lib_archive(glad STATIC
#     ARCHIVE  ${CONTRIB_DIR}/glad.tar.gz
#     FILES    src/glad.c include/glad/glad.h
#     INCLUDES include
#     FOLDER   3rdparty)
function(add_lib_archive aName aType)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "" "ARCHIVE;FOLDER" "FILES;INCLUDES")
    if(TARGET ${aName})
        return()
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "add_lib_archive(${aName}) : unknown arguments ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_ARCHIVE OR NOT ARG_FILES)
        message(FATAL_ERROR "add_lib_archive(${aName}) : ARCHIVE and FILES are required")
    endif()
    if(NOT EXISTS ${ARG_ARCHIVE})
        message(FATAL_ERROR "the ${aName} target is missing and its archive is not at ${ARG_ARCHIVE}")
    endif()
    include(FetchContent)
    FetchContent_Declare(${aName}
        URL ${ARG_ARCHIVE}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(${aName})
    # FetchContent names its variables after the LOWERCASED dependency name
    string(TOLOWER ${aName} lowerName)
    set(sourceDir ${${lowerName}_SOURCE_DIR})
    list(TRANSFORM ARG_FILES PREPEND ${sourceDir}/)
    add_library(${aName} ${aType} ${ARG_FILES})
    if(ARG_INCLUDES)
        list(TRANSFORM ARG_INCLUDES PREPEND ${sourceDir}/)
        # SYSTEM : nanovg is vendored third-party, silence its header warnings
        # (anonymous struct/union GNU extensions) at every consumer include site
        target_include_directories(${aName} SYSTEM PUBLIC ${ARG_INCLUDES})
    endif()
    if(ARG_FOLDER)
        set_target_properties(${aName} PROPERTIES FOLDER ${ARG_FOLDER})
    endif()
    set_target_properties(${aName} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endfunction()
