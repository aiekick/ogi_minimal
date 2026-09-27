# THE VULKAN SEAM, WITHOUT A SDK : volk loads vulkan-1.dll (libvulkan.so.1) at
# runtime and resolves every entry point itself, so nothing links against an
# installed sdk. the headers ride vendored beside it, the two archives carry
# the same vulkan-sdk tag and stay paired
if(NOT TARGET volk)
    set(VOLK_ARCHIVE ${CONTRIB_DIR}/libs/volk-vulkan-sdk-1.4.357.0.tar.gz)
    set(VULKAN_HEADERS_ARCHIVE ${CONTRIB_DIR}/libs/Vulkan-Headers-vulkan-sdk-1.4.357.0.tar.gz)
    if(NOT EXISTS ${VOLK_ARCHIVE} OR NOT EXISTS ${VULKAN_HEADERS_ARCHIVE})
        message(FATAL_ERROR "the volk target is missing and its archives are not in ${CONTRIB_DIR}/libs")
    endif()
    include(FetchContent)
    FetchContent_Declare(vulkanheaders
        URL ${VULKAN_HEADERS_ARCHIVE}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(vulkanheaders)
    # volk looks for an installed sdk BEFORE a headers target : the vendored
    # headers are forced, an sdk never pairs another version with volk
    set(VULKAN_HEADERS_INSTALL_DIR ${vulkanheaders_SOURCE_DIR} CACHE PATH "" FORCE)
    # the platform surface entry points volk must resolve
    if(WIN32)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR CACHE STRING "" FORCE)
    elseif(APPLE)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_METAL_EXT CACHE STRING "" FORCE)
    elseif(UNIX)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR CACHE STRING "" FORCE)
    endif()
    set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(volk
        URL ${VOLK_ARCHIVE}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(volk)
    set_target_properties(volk PROPERTIES FOLDER 3rdparty)
    if(TARGET Vulkan-Headers)
        set_target_properties(Vulkan-Headers PROPERTIES FOLDER 3rdparty)
    endif()
    set_target_properties(volk PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
