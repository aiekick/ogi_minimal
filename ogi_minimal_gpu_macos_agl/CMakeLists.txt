cmake_minimum_required(VERSION 3.24)

set(PROJECT ogi_minimal_gpu_macos_agl)
project(${PROJECT} LANGUAGES C CXX)

set(SRC_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
source_group(main FILES ${SRC_SOURCES})

# the two cocoa backends are objective-c++ under ARC : compiled as such by
# their own option, so the plain .cpp names stay and no OBJCXX language is
# enabled for the whole project
set(OGI_OBJCXX_BACKENDS
	${OGI_ROOT_DIR}/backends/ogi_cocoa.cpp
	${OGI_ROOT_DIR}/backends/ogi_agl.cpp
)
set_source_files_properties(${OGI_OBJCXX_BACKENDS} PROPERTIES COMPILE_OPTIONS "-x;objective-c++;-fobjc-arc")

set(OGI_BACKENDS
	${OGI_OBJCXX_BACKENDS}
	${OGI_ROOT_DIR}/backends/ogi_cocoa.h
	${OGI_ROOT_DIR}/backends/ogi_agl.h
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

add_executable(${PROJECT} ${SRC_SOURCES} ${OGI_BACKENDS})
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# the system frameworks are pulled HERE, by the host app, never by the ogi lib :
# Cocoa (the window), Carbon (the virtual key codes), OpenGL (the context)
target_link_libraries(${PROJECT} PRIVATE contrib ogi glad nanovg "-framework Cocoa" "-framework Carbon" "-framework OpenGL")
# the gl renderers speak a deprecated api on macos, not a removed one
target_compile_definitions(${PROJECT} PRIVATE GL_SILENCE_DEPRECATION)

set_target_properties(${PROJECT} PROPERTIES
	OUTPUT_NAME ${PROJECT}
	RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN_DIR}
	FOLDER ogi/minimal
)
