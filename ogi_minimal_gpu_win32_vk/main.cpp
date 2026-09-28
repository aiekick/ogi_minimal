#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "vkContext.h"  // the app vulkan context : volk first

#include <ogi.h>
#include <backends/ogi_win32.h>
#include <backends/ogi_vulkan.h>
#include <backends/ogi_settings_file.h>

static ogi::CharID s_app_name{"ogi_minimal_gpu_win32_vk"};

#include <ezHwm.hpp>
struct Hwm {
    float cpu{0.0f};
    float gpu{0.0f};
    bool hasGpu{false};
    const char* title{nullptr};
};
static ez::hwm::Sampler::SamplerPtr sp_sampler;
static Hwm s_hwMeasures;
void initHwm() {
    sp_sampler = ez::hwm::Sampler::create();
    if (sp_sampler != nullptr) {
        s_hwMeasures.hasGpu = sp_sampler->isGpuMeasureAvailable();
    }
}
void updateHwm(const ogi::PlatformWindowHandle& aWindow) {
    if (sp_sampler != nullptr) {
        sp_sampler->sample();
        s_hwMeasures.cpu = sp_sampler->getCpuUsagePercent();
        if (sp_sampler->isGpuMeasureAvailable()) {
            s_hwMeasures.gpu = sp_sampler->getGpuUsagePercent();
        }
        if (s_hwMeasures.hasGpu) {
            ogi::win32::setPlatformWindowTitle(aWindow, ogi::format("%s - cpu:%.2f%% - gpu:%.2f%%", s_app_name, s_hwMeasures.cpu, s_hwMeasures.gpu));
        } else {
            ogi::win32::setPlatformWindowTitle(aWindow, ogi::format("%s - cpu:%.2f%% - gpu:no", s_app_name, s_hwMeasures.cpu));
        }
    }
}

int WINAPI WinMain(HINSTANCE /*a_instance*/, HINSTANCE /*a_prev_instance*/, LPSTR /*a_cmd_line*/, int /*a_cmd_show*/) {
    auto mainWindow = ogi::win32::createAppWindow(s_app_name, 1280, 720);
    if (mainWindow == nullptr) {
        std::cout << "Fail to create the window" << std::endl;
        return EXIT_FAILURE;
    }
    // the windowing axis alone : vulkan needs no render context from it
    ogi::win32::PlatformApi platformApi;
    ogi::RendererVulkan vkRenderer;
    platformApi.setWindowCreationEnabled(false);
    ogi::createContext();
    ogi::initDefaults();
    ogi::themeDarkOrangeBlue();
    ogi::win32::setPlatformApi(&platformApi);
    ogi::setPlatformWindowHandle(ogi::kMainPlatformWindowLabel, mainWindow);
    ogi::setSettingsApi(new ogi::SettingsApiFile("ogi.ini"));
    ogi::loadSettings();
    if (!ogi::win32::init(mainWindow)) {
        std::cout << "Fail to init win32 backend" << std::endl;
        return EXIT_FAILURE;
    }
    // THE APP VULKAN CONTEXT : instance, device, swapchain, its pass and its
    // frames in flight belong to the app, ogi only receives the handles
    VkContext vkContext;
#ifdef _DEBUG
    const auto enableValidation = true;  // silently off without the sdk
#else
    const auto enableValidation = false;
#endif
    if (!vkContext.init(static_cast<HWND>(mainWindow), 2u, enableValidation)) {
        std::cout << "Fail to create the vulkan context" << std::endl;
        return EXIT_FAILURE;
    }
    ogi::VulkanInitInfo initInfo;
    initInfo.instance = vkContext.getInstance();
    initInfo.physicalDevice = vkContext.getPhysicalDevice();
    initInfo.device = vkContext.getDevice();
    initInfo.queueFamily = vkContext.getQueueFamily();
    initInfo.queue = vkContext.getQueue();
    initInfo.renderPass = vkContext.getRenderPass();
    initInfo.framesInFlight = vkContext.getFramesInFlight();
    initInfo.mainWindow = mainWindow;
    if (!vkRenderer.init(initInfo, true)) {
        std::cout << "Fail to init VULKAN renderer" << std::endl;
        return EXIT_FAILURE;
    }
    initHwm();
    ogi::frect m_lastRect{};
    const auto& io = ogi::getIo();
    bool show_demo_window{true};
    bool show_another_window{true};
    ogi::fvec4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
    bool m_quitRequested{false};
    while (!m_quitRequested) {
        if (!ogi::win32::pumpMessages(ogi::hasPendingWork() ? 16 : 250)) {
            m_quitRequested = true;
        }
        ogi::win32::newFrame();
        auto clientRect = ogi::win32::getWindowClientRect(mainWindow);
        if (clientRect != m_lastRect) {
            m_lastRect = clientRect;
            ogi::requestFullRedraw();
        }
        ogi::newFrame();
        clientRect.pos = {};
        //clientRect.size.x *= 0.5f;
        //clientRect.size.y *= 0.5f;
        //clientRect.pos.x = clientRect.size.x;
        //clientRect.pos.y = clientRect.size.y;
        if (ogi::beginViewport("viewport", clientRect)) {
            if (show_demo_window) {
                ogi::showDemoWindow(&show_demo_window);
            }
            static float f = 0.0f;
            static int counter = 0;
            if (ogi::beginWindow("Hello, world!")) {
                ogi::text("This is some useful text.");
                ogi::beginLayoutHorizontal("h");
                ogi::checkBox("Demo Window", &show_demo_window);
                ogi::checkBox("Another Window", &show_another_window);
                ogi::endLayoutHorizontal();
                ogi::slider("float", &f, 0.0f, 1.0f);
                ogi::colorEdit("clear color", &clear_color);
                if (ogi::button("Button")) {
                    counter++;
                }
                ogi::sameRow();
                // getHash is needed for have the same id for this widget since the text change frequently by button interactions
                ogi::text(ogi::getHash("counter"), ogi::format("counter = %d", counter));
                static bool show_fps{false};
                ogi::checkBox("show fps (cause a redraw each frames)", &show_fps);
                if (show_fps) {
                    // not need to use getHash here, since, the widget will be drawn each frame du to the io.deltaTime changes
                    ogi::text(ogi::format("Application average %.3f ms/frame (%.1f FPS)", io.deltaTime, io.deltaTime * 1000.0f));
                }
            }
            ogi::endWindow();
            if (show_another_window) {
                if (ogi::beginWindow("Another window", &show_another_window)) {
                    ogi::text("Hello from another window!");
                    ogi::text(ogi::getHash("vk.images"), ogi::format("swapchain images : %u", vkContext.getSwapchainImageCount()));
                    ogi::text(ogi::getHash("vk.validation"), ogi::format("validation : %s", vkContext.isValidationOn() ? "on" : "off"));
                    if (ogi::button("Close Me")) {
                        show_another_window = false;
                    }
                }
                ogi::endWindow();
            }
        }
        ogi::endViewport();
        ogi::render();
        if (ogi::hasPendingWork()) {
            // THE APP FRAME : ogi caches before the pass, the scene (a clear
            // here), the gui composed over it
            const auto commandBuffer = vkContext.beginFrame();
            if (commandBuffer != VK_NULL_HANDLE) {
                vkRenderer.recordLayers(commandBuffer, vkContext.getExtent());
                const float sceneColor[4] = {clear_color.x, clear_color.y, clear_color.z, clear_color.w};
                vkContext.beginScenePass(sceneColor);
                vkRenderer.recordComposite(commandBuffer);
                vkContext.endScenePass();
                vkContext.endFrame();
            }
        }
        updateHwm(mainWindow);
    }
    ogi::saveSettings();
    vkRenderer.unit();
    ogi::win32::shutdown();
    ogi::destroyContext();
    vkContext.unit();
    return EXIT_SUCCESS;
}
