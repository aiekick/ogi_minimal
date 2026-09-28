#include "vkContext.h"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace {

const char* const kValidationLayer = "VK_LAYER_KHRONOS_validation";

bool vkOk(VkResult aResult, const char* apWhat) {
    if (aResult != VK_SUCCESS) {
        std::cout << "vkContext : " << apWhat << " failed (VkResult " << static_cast<int32_t>(aResult) << ")\n";
        return false;
    }
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity, VkDebugUtilsMessageTypeFlagsEXT,  //
                                             const VkDebugUtilsMessengerCallbackDataEXT* apData, void*) {
    if ((aSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) && (apData != nullptr)) {
        std::cout << "vulkan validation : " << apData->pMessage << "\n";
    }
    return VK_FALSE;
}

bool isValidationLayerAvailable() {
    uint32_t layerCount = 0u;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    for (const auto& layer : layers) {
        if (std::strcmp(layer.layerName, kValidationLayer) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool VkContext::init(HWND aWindow, uint32_t aFramesInFlight, bool aEnableValidation) {
    m_window = aWindow;
    if (!vkOk(volkInitialize(), "volkInitialize (is a vulkan driver installed ?)") || !m_createInstance(aEnableValidation)) {
        return false;
    }
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hinstance = GetModuleHandleW(nullptr);
    surfaceInfo.hwnd = m_window;
    if (!vkOk(vkCreateWin32SurfaceKHR(m_instance, &surfaceInfo, nullptr, &m_surface), "vkCreateWin32SurfaceKHR")) {
        return false;
    }
    return m_pickDevice() && m_createRenderPass() && m_createFrames(aFramesInFlight) && m_createSwapchain();
}

bool VkContext::m_createInstance(bool aEnableValidation) {
    // the instance : what a win32 surface needs, the debug messenger when the
    // validation layer is there
    std::vector<const char*> extensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    m_validationOn = aEnableValidation && isValidationLayerAvailable();
    if (m_validationOn) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ogi minimal";
    appInfo.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    if (m_validationOn) {
        instanceInfo.enabledLayerCount = 1u;
        instanceInfo.ppEnabledLayerNames = &kValidationLayer;
    }
    if (!vkOk(vkCreateInstance(&instanceInfo, nullptr, &m_instance), "vkCreateInstance")) {
        return false;
    }
    volkLoadInstance(m_instance);
    if (m_validationOn) {
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = &debugCallback;
        vkCreateDebugUtilsMessengerEXT(m_instance, &debugInfo, nullptr, &m_debugMessenger);
    }
    return true;
}

bool VkContext::m_pickDevice() {
    // a family that draws AND presents, a discrete gpu preferred
    uint32_t deviceCount = 0u;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    auto bestScore = -1;
    for (const auto device : devices) {
        uint32_t familyCount = 0u;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());
        for (uint32_t familyIdx = 0u; familyIdx < familyCount; ++familyIdx) {
            VkBool32 presentSupported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIdx, m_surface, &presentSupported);
            if (((families[familyIdx].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u) || (presentSupported != VK_TRUE)) {
                continue;
            }
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device, &properties);
            const auto score = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 2 : 1;
            if (score > bestScore) {
                bestScore = score;
                m_physicalDevice = device;
                m_queueFamily = familyIdx;
            }
            break;
        }
    }
    if (m_physicalDevice == VK_NULL_HANDLE) {
        std::cout << "vkContext : no device draws and presents\n";
        return false;
    }
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
    std::cout << "vkContext : " << properties.deviceName << "\n";
    const auto queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_queueFamily;
    queueInfo.queueCount = 1u;
    queueInfo.pQueuePriorities = &queuePriority;
    const char* const deviceExtensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1u;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = 1u;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    if (!vkOk(vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device), "vkCreateDevice")) {
        return false;
    }
    volkLoadDevice(m_device);
    vkGetDeviceQueue(m_device, m_queueFamily, 0u, &m_queue);
    // unorm, never srgb : the colors are authored in the space they are written
    uint32_t formatCount = 0u;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());
    if (formats.empty()) {
        return false;
    }
    m_surfaceFormat = formats[0];
    for (const auto& format : formats) {
        if (((format.format == VK_FORMAT_B8G8R8A8_UNORM) || (format.format == VK_FORMAT_R8G8B8A8_UNORM)) && (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            m_surfaceFormat = format;
            break;
        }
    }
    return true;
}

bool VkContext::m_createRenderPass() {
    // THE SCENE PASS : cleared on entry, the scene then the gui land in it
    VkAttachmentDescription color{};
    color.format = m_surfaceFormat.format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference colorRef{};
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1u;
    subpass.pColorAttachments = &colorRef;
    // the acquire semaphore is waited at COLOR_ATTACHMENT_OUTPUT : the pass
    // writes nothing before the image is really available
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo passInfo{};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.attachmentCount = 1u;
    passInfo.pAttachments = &color;
    passInfo.subpassCount = 1u;
    passInfo.pSubpasses = &subpass;
    passInfo.dependencyCount = 1u;
    passInfo.pDependencies = &dependency;
    return vkOk(vkCreateRenderPass(m_device, &passInfo, nullptr, &m_renderPass), "vkCreateRenderPass");
}

bool VkContext::m_createFrames(uint32_t aFramesInFlight) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueFamily;
    if (!vkOk(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool), "vkCreateCommandPool")) {
        return false;
    }
    m_frames.resize(std::max(aFramesInFlight, 1u));
    for (auto& frame : m_frames) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1u;
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // nothing in flight yet
        if (!vkOk(vkAllocateCommandBuffers(m_device, &allocInfo, &frame.commandBuffer), "vkAllocateCommandBuffers")  //
            || !vkOk(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &frame.imageAvailable), "vkCreateSemaphore")  //
            || !vkOk(vkCreateFence(m_device, &fenceInfo, nullptr, &frame.inFlight), "vkCreateFence")) {
            return false;
        }
    }
    return true;
}

bool VkContext::m_createSwapchain() {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);
    auto extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        RECT clientRect{};
        GetClientRect(m_window, &clientRect);
        extent.width = static_cast<uint32_t>(clientRect.right - clientRect.left);
        extent.height = static_cast<uint32_t>(clientRect.bottom - clientRect.top);
    }
    if ((extent.width == 0u) || (extent.height == 0u)) {
        return false;  // minimized : the next frame tries again
    }
    m_extent = extent;
    // as few images as the surface allows : the acquire paces the loop on the
    // vsync, no frame queues ahead of the one shown
    auto imageCount = std::max(caps.minImageCount, 2u);
    if ((caps.maxImageCount > 0u) && (imageCount > caps.maxImageCount)) {
        imageCount = caps.maxImageCount;
    }
    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = m_surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = m_surfaceFormat.format;
    swapchainInfo.imageColorSpace = m_surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = m_extent;
    swapchainInfo.imageArrayLayers = 1u;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = caps.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;  // the only mode the spec guarantees, and it is vsync
    swapchainInfo.clipped = VK_TRUE;
    if (!vkOk(vkCreateSwapchainKHR(m_device, &swapchainInfo, nullptr, &m_swapchain), "vkCreateSwapchainKHR")) {
        return false;
    }
    uint32_t realImageCount = 0u;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &realImageCount, nullptr);
    std::vector<VkImage> images(realImageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &realImageCount, images.data());
    for (const auto image : images) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_surfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1u;
        viewInfo.subresourceRange.layerCount = 1u;
        VkImageView view{VK_NULL_HANDLE};
        vkCreateImageView(m_device, &viewInfo, nullptr, &view);
        m_views.push_back(view);
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1u;
        framebufferInfo.pAttachments = &view;
        framebufferInfo.width = m_extent.width;
        framebufferInfo.height = m_extent.height;
        framebufferInfo.layers = 1u;
        VkFramebuffer framebuffer{VK_NULL_HANDLE};
        vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &framebuffer);
        m_framebuffers.push_back(framebuffer);
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkSemaphore semaphore{VK_NULL_HANDLE};
        vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &semaphore);
        m_renderFinished.push_back(semaphore);
    }
    m_swapchainDirty = false;
    return true;
}

void VkContext::m_destroySwapchain() {
    for (const auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    m_framebuffers.clear();
    for (const auto view : m_views) {
        vkDestroyImageView(m_device, view, nullptr);
    }
    m_views.clear();
    for (const auto semaphore : m_renderFinished) {
        vkDestroySemaphore(m_device, semaphore, nullptr);
    }
    m_renderFinished.clear();
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

void VkContext::unit() {
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
        m_destroySwapchain();
        for (auto& frame : m_frames) {
            vkDestroyFence(m_device, frame.inFlight, nullptr);
            vkDestroySemaphore(m_device, frame.imageAvailable, nullptr);
        }
        m_frames.clear();
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    if (m_debugMessenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

VkCommandBuffer VkContext::beginFrame() {
    if (m_swapchainDirty || (m_swapchain == VK_NULL_HANDLE)) {
        vkDeviceWaitIdle(m_device);
        m_destroySwapchain();
        if (!m_createSwapchain()) {
            return VK_NULL_HANDLE;
        }
    }
    auto& frame = m_frames[m_frameIdx];
    vkWaitForFences(m_device, 1u, &frame.inFlight, VK_TRUE, UINT64_MAX);
    const auto acquired = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, frame.imageAvailable, VK_NULL_HANDLE, &m_imageIndex);
    if (acquired == VK_ERROR_OUT_OF_DATE_KHR) {
        m_swapchainDirty = true;
        return VK_NULL_HANDLE;
    }
    m_swapchainDirty = (acquired == VK_SUBOPTIMAL_KHR);
    vkResetFences(m_device, 1u, &frame.inFlight);
    vkResetCommandBuffer(frame.commandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);
    return frame.commandBuffer;
}

void VkContext::beginScenePass(const float (&aClearColor)[4]) {
    VkClearValue clearValue{};
    std::memcpy(clearValue.color.float32, aClearColor, sizeof(clearValue.color.float32));
    VkRenderPassBeginInfo passBegin{};
    passBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passBegin.renderPass = m_renderPass;
    passBegin.framebuffer = m_framebuffers[m_imageIndex];
    passBegin.renderArea.extent = m_extent;
    passBegin.clearValueCount = 1u;
    passBegin.pClearValues = &clearValue;
    vkCmdBeginRenderPass(m_frames[m_frameIdx].commandBuffer, &passBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void VkContext::endScenePass() {
    vkCmdEndRenderPass(m_frames[m_frameIdx].commandBuffer);
}

void VkContext::endFrame() {
    auto& frame = m_frames[m_frameIdx];
    vkEndCommandBuffer(frame.commandBuffer);
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1u;
    submitInfo.pWaitSemaphores = &frame.imageAvailable;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1u;
    submitInfo.pCommandBuffers = &frame.commandBuffer;
    submitInfo.signalSemaphoreCount = 1u;
    submitInfo.pSignalSemaphores = &m_renderFinished[m_imageIndex];
    vkQueueSubmit(m_queue, 1u, &submitInfo, frame.inFlight);
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1u;
    presentInfo.pWaitSemaphores = &m_renderFinished[m_imageIndex];
    presentInfo.swapchainCount = 1u;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_imageIndex;
    const auto presented = vkQueuePresentKHR(m_queue, &presentInfo);
    if ((presented == VK_ERROR_OUT_OF_DATE_KHR) || (presented == VK_SUBOPTIMAL_KHR)) {
        m_swapchainDirty = true;
    }
    m_frameIdx = (m_frameIdx + 1u) % static_cast<uint32_t>(m_frames.size());
}
