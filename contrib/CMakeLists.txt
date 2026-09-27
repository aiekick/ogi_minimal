set(CONTRIB_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(contrib INTERFACE)
target_include_directories(contrib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

include(cmake/functions.cmake)
include(cmake/glad.cmake)
include(cmake/nanovg.cmake)
include(cmake/volk.cmake)
