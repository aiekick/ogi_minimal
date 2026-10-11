cmake_minimum_required(VERSION 3.24)

set(PROJECT ogi_minimal_gpu_win32_vk)
project(${PROJECT} LANGUAGES C CXX)

set(SRC_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vkContext.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vkContext.h
)
source_group(main FILES ${SRC_SOURCES})

set(OGI_BACKENDS
	${OGI_ROOT_DIR}/backends/ogi_win32.cpp
	${OGI_ROOT_DIR}/backends/ogi_win32.h
	${OGI_ROOT_DIR}/backends/ogi_vulkan.cpp
	${OGI_ROOT_DIR}/backends/ogi_vulkan.h
	${OGI_ROOT_DIR}/backends/ogi_vulkan_shaders.h
	${OGI_ROOT_DIR}/backends/ogi_draw_gpu.cpp
	${OGI_ROOT_DIR}/backends/ogi_draw_gpu.h
	${OGI_ROOT_DIR}/backends/ogi_skin_core.cpp
	${OGI_ROOT_DIR}/backends/ogi_skin_core.h
	${OGI_ROOT_DIR}/backends/ogi_settings_file.cpp
	${OGI_ROOT_DIR}/backends/ogi_settings_file.h
)
source_group(backends FILES ${OGI_BACKENDS})

add_executable(${PROJECT} WIN32 ${SRC_SOURCES} ${OGI_BACKENDS})
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# volk loads vulkan-1.dll at runtime : no vulkan library is linked
target_link_libraries(${PROJECT} PRIVATE contrib ogi volk pdh)

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
