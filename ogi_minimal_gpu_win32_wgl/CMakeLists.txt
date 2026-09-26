cmake_minimum_required(VERSION 3.24)

set(PROJECT ogi_minimal_gpu_win32_wgl)
project(${PROJECT} LANGUAGES C CXX)

set(SRC_SOURCES 
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
source_group(main FILES ${SRC_SOURCES})

set(OGI_BACKENDS
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_win32.cpp
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_win32.h
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_wgl.cpp
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_wgl.h
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_opengl32.cpp
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_opengl32.h
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_draw_gpu.cpp
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_draw_gpu.h
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_skin_core.cpp
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_skin_core.h
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_settings_file.cpp
	${CMAKE_SOURCE_DIR}/ogi/backends/ogi_settings_file.h
)
source_group(backends FILES ${OGI_BACKENDS})

add_executable(${PROJECT} WIN32 ${SRC_SOURCES} ${OGI_BACKENDS})
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT} PRIVATE contrib ogi opengl32 glad pdh)
if(MSVC)
	target_compile_definitions(${PROJECT} PRIVATE -DNOMINMAX)
endif()

set_target_properties(${PROJECT} PROPERTIES
	OUTPUT_NAME ${PROJECT}
	RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN_DIR}
	VS_DEBUGGER_WORKING_DIRECTORY ${BIN_DIR}
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	FOLDER ogi/minimal
)
