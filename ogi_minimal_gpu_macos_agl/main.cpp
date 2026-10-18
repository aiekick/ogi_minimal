#include <cstdlib>
#include <cstdint>
#include <iostream>

#include <ogi.h>
#include <backends/ogi_cocoa.h>
#include <glad/glad.h>  // the loader of the app's own gl drawing
#include <backends/ogi_opengl32.h>
#include <backends/ogi_agl.h>
#include <backends/ogi_settings_file.h>
#include <backends/ogi_nanovg.h>

// no hardware measure in the title here : ezHwm has no macos sampler
static ogi::CharID s_app_name{"ogi_minimal_gpu_macos_agl"};

int main(int /*argc*/, char** /*argv*/) {
    auto mainWindow = ogi::cocoa::createAppWindow(s_app_name, 1280, 720);
    if (mainWindow == nullptr) {
        std::cout << "Fail to create the window" << std::endl;
        return EXIT_FAILURE;
    }
    // THE GL PAIRING : the context is born on the window to the asked
    // version and profile (macos answers a 3.3 core ask with 4.1 core), then
    // the loader resolves the entry points on it
    ogi::agl::ContextDesc contextDesc;
    contextDesc.coreProfile = true;
    contextDesc.majorVersion = 3;
    contextDesc.minorVersion = 3;
    contextDesc.swapInterval = 1;  // vsync
    if (!ogi::agl::init(mainWindow, contextDesc)) {
        std::cout << "Fail to create the gl context" << std::endl;
        ogi::agl::unit();
        ogi::cocoa::shutdown();
        return EXIT_FAILURE;
    }
    // the loader is the app's choice : glad here, fed by the pairing
    if (gladLoadGLLoader(ogi::agl::procAddress) == 0) {
        std::cout << "Fail to load the gl entry points" << std::endl;
        ogi::agl::unit();
        ogi::cocoa::shutdown();
        return EXIT_FAILURE;
    }
    ogi::agl::PlatformApi platformApi;
    ogi::RendererGL glRenderer;
    ogi::RendererNanoVg vgRenderer;
    ogi::createContext();
    ogi::initDefaults();
    ogi::useTheme("dark orange blue");
    ogi::setPlatformApi(&platformApi);
    ogi::setPlatformWindowHandle(ogi::kMainPlatformWindowLabel, mainWindow);
    ogi::setSettingsApi(new ogi::SettingsApiFile("ogi.ini"));
    ogi::loadSettings();
    if (!ogi::cocoa::init(mainWindow)) {
        std::cout << "Fail to init cocoa backend" << std::endl;
        return EXIT_FAILURE;
    }
    enum API { API_CORE = 1, API_NANOVG, ApiCount };
    // register draw/skin api for the core. 0 is the default
    ogi::setSkinApi<ogi::SkinApiCore>(API_CORE);
    ogi::setDrawApi<ogi::DrawApiGPU>(API_CORE);
    if (!glRenderer.init(ogi::agl::procAddress, false)) {
        std::cout << "Fail to init GL renderer" << std::endl;
        return EXIT_FAILURE;
    }
    // register draw/skin api for nanovg
    ogi::setSkinApi<ogi::SkinApiVG>(API_NANOVG);
    ogi::setDrawApi<ogi::DrawApiVG>(API_NANOVG);
    if (!vgRenderer.init(ogi::agl::procAddress, false)) {
        std::cout << "Fail to init NANOVG renderer" << std::endl;
        return EXIT_FAILURE;
    }
    ogi::frect m_lastRect{};
    const auto& io = ogi::getIo();
    bool show_demo_window{true};
    bool show_another_window{true};
    ogi::fvec4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
    bool m_quitRequested{false};
    while (!m_quitRequested) {
        if (!ogi::cocoa::pumpMessages(ogi::hasPendingWork() ? 16 : 250)) {
            m_quitRequested = true;
        }
        ogi::cocoa::newFrame();
        // the ui speaks points, the framebuffer counts the retina pixels
        auto clientRect = ogi::cocoa::getWindowClientRect(mainWindow);
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
            ogi::agl::makeCurrent(nullptr);  // nullptr = the main surface ; it follows the view size too
            const auto framebufferSize = platformApi.getPlatformWindowFramebufferSize(mainWindow);
            glViewport(0, 0, static_cast<GLsizei>(framebufferSize.x), static_cast<GLsizei>(framebufferSize.y));
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            // the renderer consumes and clears the context draw api
            if (ogi::getDrawApiFamilyId() == ogi::DrawApiVG::familyId()) {
                vgRenderer.render();
            } else {
                glRenderer.render();
            }
            ogi::agl::present(nullptr);
        }
    }
    ogi::saveSettings();
    glRenderer.unit();
    vgRenderer.unit();
    ogi::agl::unit();        // the context lives on the view : before the window goes
    ogi::cocoa::shutdown();  // the window, last
    ogi::destroyContext();
    return EXIT_SUCCESS;
}
