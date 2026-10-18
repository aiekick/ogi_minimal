cmake_minimum_required(VERSION 3.24)

set(PROJECT ogi_minimal_cpu_linux_x11)
project(${PROJECT} LANGUAGES C CXX)

set(SRC_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
source_group(main FILES ${SRC_SOURCES})

set(OGI_BACKENDS
	${OGI_ROOT_DIR}/backends/ogi_xlib.cpp
	${OGI_ROOT_DIR}/backends/ogi_xlib.h
	${OGI_ROOT_DIR}/backends/ogi_skin_core.cpp
	${OGI_ROOT_DIR}/backends/ogi_skin_core.h
	${OGI_ROOT_DIR}/backends/ogi_draw_cpu.cpp
	${OGI_ROOT_DIR}/backends/ogi_draw_cpu.h
	${OGI_ROOT_DIR}/backends/ogi_renderer_software.cpp
	${OGI_ROOT_DIR}/backends/ogi_renderer_software.h
	${OGI_ROOT_DIR}/backends/ogi_x11.cpp
	${OGI_ROOT_DIR}/backends/ogi_x11.h
	${OGI_ROOT_DIR}/backends/ogi_settings_file.cpp
	${OGI_ROOT_DIR}/backends/ogi_settings_file.h
)
source_group(backends FILES ${OGI_BACKENDS})

# libX11 is pulled HERE, by the host app, never by the ogi lib
find_package(X11 REQUIRED)

add_executable(${PROJECT} ${SRC_SOURCES} ${OGI_BACKENDS})
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT} PRIVATE ogi contrib X11::X11)

set_target_properties(${PROJECT} PROPERTIES
	OUTPUT_NAME ${PROJECT}
	RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN_DIR}
	FOLDER ogi/minimal
)
