#include <clocale>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#include <ogi.h>
#include <backends/ogi_x11.h>
#include <backends/ogi_xlib.h>
#include <backends/ogi_settings_file.h>

static ogi::CharID s_app_name{"ogi_minimal_cpu_linux_x11"};

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
            ogi::xlib::setPlatformWindowTitle(aWindow, ogi::format("%s - cpu:%.2f%% - gpu:%.2f%%", s_app_name, s_hwMeasures.cpu, s_hwMeasures.gpu));
        } else {
            ogi::xlib::setPlatformWindowTitle(aWindow, ogi::format("%s - cpu:%.2f%% - gpu:no", s_app_name, s_hwMeasures.cpu));
        }
    }
}

int main(int /*argc*/, char** /*argv*/) {
    // the input method reads the locale : utf-8 text needs it set first
    if (std::setlocale(LC_ALL, "") == nullptr) {
        std::cout << "setlocale fail to apply, the typed text falls back to latin-1" << std::endl;
    }
    auto mainWindow = ogi::xlib::createAppWindow(s_app_name, 1280, 720);
    if (mainWindow == nullptr) {
        std::cout << "Fail to create the window (is DISPLAY set ?)" << std::endl;
        return EXIT_FAILURE;
    }
    ogi::RendererX11 x11Renderer;
    ogi::xlib::PlatformApi m_platformApi;
    int32_t m_lastWidth{};
    int32_t m_lastHeight{};
    ogi::createContext();
    ogi::initDefaults();
    ogi::useTheme("dark orange blue");
    ogi::xlib::setPlatformApi(&m_platformApi);
    ogi::setPlatformWindowHandle(ogi::kMainPlatformWindowLabel, mainWindow);
    ogi::setSettingsApi(new ogi::SettingsApiFile("ogi.ini"));
    ogi::loadSettings();
    if (!ogi::xlib::init(mainWindow)) {
        std::cout << "Fail to init xlib backend" << std::endl;
        return EXIT_FAILURE;
    }
    x11Renderer.setWindow(ogi::xlib::getDisplay(), static_cast<unsigned long>(reinterpret_cast<uintptr_t>(mainWindow)));
    if (!x11Renderer.init(true)) {
        std::cout << "Fail to init renderer" << std::endl;
        return EXIT_FAILURE;
    }
    initHwm();
    const auto& io = ogi::getIo();
    bool show_demo_window{true};
    bool show_another_window{true};
    ogi::fvec4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
    bool m_quitRequested{false};
    while (!m_quitRequested) {
        if (!ogi::xlib::pumpMessages(ogi::hasPendingWork() ? 16 : 250)) {
            m_quitRequested = true;
        }
        ogi::xlib::newFrame();
        auto clientRect = ogi::xlib::getWindowClientRect(mainWindow);
        const auto w = static_cast<int32_t>(clientRect.size.x);
        const auto h = static_cast<int32_t>(clientRect.size.y);
        if ((w != m_lastWidth) || (h != m_lastHeight)) {
            m_lastWidth = w;
            m_lastHeight = h;
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
                ogi::checkBox("Demo Window", &show_demo_window);
                ogi::checkBox("Another Window", &show_another_window);
                ogi::slider("float", &f, 0.0f, 1.0f);
                ogi::colorEdit("clear color", &clear_color);
                if (ogi::button("Button")) {
                    counter++;
                }
                ogi::sameRow();
                ogi::text(ogi::getHash("counter"), ogi::format("counter = %d", counter));
                ogi::text(ogi::getHash("framerate"),
                    ogi::format("Application average %.3f ms/frame (%.1f FPS)", io.deltaTime, io.deltaTime * 1000.0f));
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
            x11Renderer.clearBackbuffer(clear_color);  // the "app scene"
            x11Renderer.render();                      // consumes and clears the context draw api
            x11Renderer.present();                     // XPutImage of the backbuffer to the window
        }
        updateHwm(mainWindow);
    }
    ogi::saveSettings();
    x11Renderer.unit();      // frees its GC : the connection must still stand
    ogi::xlib::shutdown();  // the window and the connection, last
    ogi::destroyContext();
    return EXIT_SUCCESS;
}
