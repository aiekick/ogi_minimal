#include <cstdlib>
#include <cstdint>
#include <iostream>

#include <ogi.h>
#include <backends/ogi_win32.h>
#include <glad/glad.h>  // the loader of the app's own gl drawing
#include <backends/ogi_opengl32.h>
#include <backends/ogi_wgl.h>
#include <backends/ogi_settings_file.h>

static ogi::CharID s_app_name{"ogi_minimal_gpu_win32_wgl"};

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

int WINAPI WinMain(HINSTANCE a_instance, HINSTANCE /*a_prev_instance*/, LPSTR /*a_cmd_line*/, int a_cmd_show) {
    auto mainWindow = ogi::win32::createAppWindow(s_app_name, 1280, 720);
    if (mainWindow == nullptr) {
        std::cout << "Fail to create the window" << std::endl;
        return EXIT_FAILURE;
    }
    // THE GL PAIRING : the context is born on the window to the asked
    // version and profile, then the loader resolves the entry points on it
    ogi::wgl::ContextDesc glContextDesc;
    glContextDesc.coreProfile = true;
    glContextDesc.majorVersion = 3;
    glContextDesc.minorVersion = 3;
    glContextDesc.swapInterval = 1;
    if (!ogi::wgl::init(mainWindow, glContextDesc)) {
        ogi::wgl::unit();
        return EXIT_FAILURE;
    }
    // the loader is the app's choice : glad here, fed by the pairing
    if (gladLoadGLLoader(ogi::wgl::procAddress) == 0) {
        std::cout << "Fail to load the gl entry points" << std::endl;
        ogi::wgl::unit();
        return EXIT_FAILURE;
    }
    ogi::wgl::PlatformApi platformApi;
    ogi::RendererGL glRenderer;
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
    if (!glRenderer.init(ogi::wgl::procAddress)) {
        std::cout << "Fail to init renderer" << std::endl;
        return EXIT_FAILURE;
    }
    initHwm();
    int32_t m_lastWidth{};
    int32_t m_lastHeight{};
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
        const auto width = static_cast<int32_t>(clientRect.size.x);
        const auto height = static_cast<int32_t>(clientRect.size.y);
        if (width != m_lastWidth || height != m_lastHeight) {
            m_lastWidth = width;
            m_lastHeight = height;
            ogi::requestFullRedraw();
        }
        ogi::newFrame();
        clientRect.pos = {};
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
                // getHash is needed for have the same id for this widget since the text qui change frequently by interactions
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
                if (ogi::beginWindow("Another Window", &show_another_window)) {
                    ogi::text("Hello from another window!");
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
            ogi::wgl::makeCurrent(nullptr);  // nullptr = the main surface
            glViewport(0, 0, width, height);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            glRenderer.render();  // consumes and clears the context draw api
            ogi::wgl::present(nullptr);
        }
        updateHwm(mainWindow);
    }
    ogi::saveSettings();
    glRenderer.unit();
    ogi::win32::shutdown();
    ogi::wgl::unit();
    ogi::destroyContext();
    return EXIT_SUCCESS;
}
