#pragma once

// volk FIRST : it defines VK_NO_PROTOTYPES and pulls the vulkan headers
#include <volk.h>

#include <windows.h>  // HWND : the window the surface is made of

#include <cstdint>
#include <vector>

// THE APP VULKAN CONTEXT of this minimal : the context a 3d app already has,
// reduced to what a frame needs — instance, device, queue, swapchain, ONE
// render pass and its frames in flight. ogi knows none of it but the handles
// it is given (VulkanInitInfo). a frame reads :
//
//   auto commandBuffer = context.beginFrame();     // null = skip this turn
//   renderer.recordLayers(commandBuffer, context.getExtent());
//   context.beginScenePass(clearColor);
//   ... the app scene ...
//   renderer.recordComposite(commandBuffer);
//   context.endScenePass();
//   context.endFrame();                            // submit + present
class VkContext {
private:
    struct FrameSync {
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        VkSemaphore imageAvailable{VK_NULL_HANDLE};
        VkFence inFlight{VK_NULL_HANDLE};
    };

private:
    HWND m_window{nullptr};  // observing : the app owns its window
    VkInstance m_instance{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};
    VkSurfaceKHR m_surface{VK_NULL_HANDLE};
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    uint32_t m_queueFamily{};
    VkDevice m_device{VK_NULL_HANDLE};
    VkQueue m_queue{VK_NULL_HANDLE};
    VkSurfaceFormatKHR m_surfaceFormat{};
    VkRenderPass m_renderPass{VK_NULL_HANDLE};
    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
    VkExtent2D m_extent{};
    std::vector<VkImageView> m_views;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkSemaphore> m_renderFinished;  // one per swapchain image : a present waits on the one of its image
    VkCommandPool m_commandPool{VK_NULL_HANDLE};
    std::vector<FrameSync> m_frames;
    uint32_t m_frameIdx{};
    uint32_t m_imageIndex{};
    bool m_swapchainDirty{false};
    bool m_validationOn{false};

public:
    // aEnableValidation degrades to off when the khronos layer is absent (no sdk)
    bool init(HWND aWindow, uint32_t aFramesInFlight, bool aEnableValidation);
    void unit();
    // the frame : null when there is nothing to paint (minimized, the
    // swapchain was out of date and got rebuilt)
    VkCommandBuffer beginFrame();
    void beginScenePass(const float (&aClearColor)[4]);
    void endScenePass();
    void endFrame();
    // the handles a gui layer or a scene pipeline is built on
    VkInstance getInstance() const { return m_instance; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkDevice getDevice() const { return m_device; }
    uint32_t getQueueFamily() const { return m_queueFamily; }
    VkQueue getQueue() const { return m_queue; }
    VkRenderPass getRenderPass() const { return m_renderPass; }
    VkExtent2D getExtent() const { return m_extent; }
    uint32_t getFramesInFlight() const { return static_cast<uint32_t>(m_frames.size()); }
    uint32_t getSwapchainImageCount() const { return static_cast<uint32_t>(m_views.size()); }
    bool isValidationOn() const { return m_validationOn; }

private:
    bool m_createInstance(bool aEnableValidation);
    bool m_pickDevice();
    bool m_createRenderPass();
    bool m_createFrames(uint32_t aFramesInFlight);
    bool m_createSwapchain();
    void m_destroySwapchain();
};
