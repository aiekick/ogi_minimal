cmake_minimum_required(VERSION 3.24)

set(PROJECT ogi_minimal_gpu_linux_xgl)
project(${PROJECT} LANGUAGES C CXX)

set(SRC_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
source_group(main FILES ${SRC_SOURCES})

set(OGI_BACKENDS
	${OGI_ROOT_DIR}/backends/ogi_xlib.cpp
	${OGI_ROOT_DIR}/backends/ogi_xlib.h
	${OGI_ROOT_DIR}/backends/ogi_lgl.cpp
	${OGI_ROOT_DIR}/backends/ogi_lgl.h
	${OGI_ROOT_DIR}/backends/ogi_opengl32.cpp
	${OGI_ROOT_DIR}/backends/ogi_opengl32.h
	${OGI_ROOT_DIR}/backends/ogi_draw_gpu.cpp
	${OGI_ROOT_DIR}/backends/ogi_draw_gpu.h
	${OGI_ROOT_DIR}/backends/ogi_skin_core.cpp
	${OGI_ROOT_DIR}/backends/ogi_skin_core.h
	${OGI_ROOT_DIR}/backends/ogi_settings_file.cpp
	${OGI_ROOT_DIR}/backends/ogi_settings_file.h
	${OGI_ROOT_DIR}/backends/ogi_draw_nanovg.cpp
	${OGI_ROOT_DIR}/backends/ogi_draw_nanovg.h
	${OGI_ROOT_DIR}/backends/ogi_skin_nanovg.cpp
	${OGI_ROOT_DIR}/backends/ogi_skin_nanovg.h
	${OGI_ROOT_DIR}/backends/ogi_nanovg.cpp
	${OGI_ROOT_DIR}/backends/ogi_nanovg.h
)
source_group(backends FILES ${OGI_BACKENDS})

# libX11 and libEGL are SYSTEM libraries pulled HERE, by the host app,
# never by the ogi lib. no libGL : every gl entry point comes through glad,
# resolved on the egl context
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS EGL)

add_executable(${PROJECT} ${SRC_SOURCES} ${OGI_BACKENDS})
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT} PRIVATE contrib ogi glad nanovg X11::X11 OpenGL::EGL ${CMAKE_DL_LIBS})

set_target_properties(${PROJECT} PROPERTIES
	OUTPUT_NAME ${PROJECT}
	RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN_DIR}
	FOLDER ogi/minimal
)
