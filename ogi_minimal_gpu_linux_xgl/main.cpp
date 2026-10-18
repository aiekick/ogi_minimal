#include <clocale>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#include <ogi.h>
#include <backends/ogi_xlib.h>
#include <glad/glad.h>  // the loader of the app's own gl drawing
#include <backends/ogi_opengl32.h>
#include <backends/ogi_lgl.h>
#include <backends/ogi_settings_file.h>
#include <backends/ogi_nanovg.h>

static ogi::CharID s_app_name{"ogi_minimal_gpu_linux_xgl"};

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
    // THE GL PAIRING : the context is born on the window to the asked
    // version and profile, on the connection of the window, then the loader
    // resolves the entry points on it
    ogi::lgl::ContextDesc contextDesc;
    contextDesc.coreProfile = true;
    contextDesc.majorVersion = 3;
    contextDesc.minorVersion = 3;
    contextDesc.swapInterval = 1;  // vsync
    contextDesc.nativeDisplay = ogi::xlib::getDisplay();
    if (!ogi::lgl::init(mainWindow, contextDesc)) {
        std::cout << "Fail to create the gl context" << std::endl;
        ogi::lgl::unit();
        ogi::xlib::shutdown();
        return EXIT_FAILURE;
    }
    // the loader is the app's choice : glad here, fed by the pairing
    if (gladLoadGLLoader(ogi::lgl::procAddress) == 0) {
        std::cout << "Fail to load the gl entry points" << std::endl;
        ogi::lgl::unit();
        ogi::xlib::shutdown();
        return EXIT_FAILURE;
    }
    ogi::lgl::PlatformApi platformApi;
    ogi::RendererGL glRenderer;
    ogi::RendererNanoVg vgRenderer;
    ogi::createContext();
    ogi::initDefaults();
    ogi::useTheme("dark orange blue");
    ogi::xlib::setPlatformApi(&platformApi);
    ogi::setPlatformWindowHandle(ogi::kMainPlatformWindowLabel, mainWindow);
    ogi::setSettingsApi(new ogi::SettingsApiFile("ogi.ini"));
    ogi::loadSettings();
    if (!ogi::xlib::init(mainWindow)) {
        std::cout << "Fail to init xlib backend" << std::endl;
        return EXIT_FAILURE;
    }
    enum API { API_CORE = 1, API_NANOVG, ApiCount };
    // register draw/skin api for the core. 0 is the default
    ogi::setSkinApi<ogi::SkinApiCore>(API_CORE);
    ogi::setDrawApi<ogi::DrawApiGPU>(API_CORE);
    if (!glRenderer.init(ogi::lgl::procAddress, false)) {
        std::cout << "Fail to init GL renderer" << std::endl;
        return EXIT_FAILURE;
    }
    // register draw/skin api for nanovg
    ogi::setSkinApi<ogi::SkinApiVG>(API_NANOVG);
    ogi::setDrawApi<ogi::DrawApiVG>(API_NANOVG);
    if (!vgRenderer.init(ogi::lgl::procAddress, false)) {
        std::cout << "Fail to init NANOVG renderer" << std::endl;
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
        if (!ogi::xlib::pumpMessages(ogi::hasPendingWork() ? 16 : 250)) {
            m_quitRequested = true;
        }
        ogi::xlib::newFrame();
        auto clientRect = ogi::xlib::getWindowClientRect(mainWindow);
        if (clientRect != m_lastRect) {
            m_lastRect = clientRect;
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
                    const auto& coreApi = ogi::getDrawApiFamilyId() == ogi::DrawApiGPU::familyId();
                    ogi::beginLayoutHorizontal("rendering api");
                    ogi::pushItemWidth(-1.0f);
                    if (ogi::checkButton("Core", coreApi)) {
                        ogi::selectSkinApi(API_CORE);
                        ogi::selectDrawApi(API_CORE);
                    }
                    if (ogi::checkButton("NanoVg", !coreApi)) {
                        ogi::selectSkinApi(API_NANOVG);
                        ogi::selectDrawApi(API_NANOVG);
                    }
                    ogi::popItemWidth();
                    ogi::endLayoutHorizontal();
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
            ogi::lgl::makeCurrent(nullptr);  // nullptr = the main surface
            glViewport(clientRect.pos.x, clientRect.pos.y, clientRect.size.x, clientRect.size.y);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            // the renderer consumes and clears the context draw api
            if (ogi::getDrawApiFamilyId() == ogi::DrawApiVG::familyId()) {
                vgRenderer.render();
            } else {
                glRenderer.render();
            }
            ogi::lgl::present(nullptr);
        }
        updateHwm(mainWindow);
    }
    ogi::saveSettings();
    glRenderer.unit();
    vgRenderer.unit();
    ogi::lgl::unit();       // the context rides the connection : before it closes
    ogi::xlib::shutdown();  // the window and the connection, last
    ogi::destroyContext();
    return EXIT_SUCCESS;
}
