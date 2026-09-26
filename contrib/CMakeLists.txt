
add_library(contrib INTERFACE)
target_include_directories(contrib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

include(glad.cmake)
