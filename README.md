# ogi_minimal

The smallest hosts of [ogi](https://github.com/aiekick/ogi) : one
`main.cpp` per os and renderer pairing, the shape of the imgui examples.
Each one opens a window, wires the backends it needs, shows the ogi demo
window and a hello window, and presents on demand (the loop sleeps while
nothing changes).

| host | windowing | renderer |
|---|---|---|
| `ogi_minimal_cpu_win32_gdi` | `ogi_win32` | `RendererGdi`, the software rasterizer |
| `ogi_minimal_gpu_win32_wgl` | `ogi_win32` + `ogi_wgl` | `RendererGL` and `RendererNanoVg`, switchable live |
| `ogi_minimal_gpu_win32_vk` | `ogi_win32` | `RendererVulkan`, over the app's own vulkan context |
| `ogi_minimal_cpu_linux_x11` | `ogi_xlib` | `RendererX11`, the software rasterizer, no opengl |
| `ogi_minimal_gpu_linux_xgl` | `ogi_xlib` + `ogi_lgl` (EGL) | `RendererGL` and `RendererNanoVg`, switchable live |

The macos hosts (`ogi_minimal_cpu_macos_cocoa`, `ogi_minimal_gpu_macos_agl`)
are placeholders : they configure, they build nothing yet.

Every host follows the same order : the window, the gl context and its
loader when there is one, the ogi context, the platform seam
(`ogi::setPlatformApi`), the settings, the renderers ; and the reverse at
the exit, the windowing backend last.

## Dependencies

ogi comes as a submodule (`ogi/`) ; a parent project that already has the
`ogi` target gives its own instead. Everything else is vendored in
`contrib/` and extracted at configure time, no network : glad, nanovg,
volk with the vulkan headers (no sdk needed), and `ezHwm.hpp`, the cpu and
gpu load shown in the window title.

The system libraries are pulled by the host that needs them : `gdi32`
and `opengl32` on windows, `X11` and `EGL` on linux. The linux gl host
links no libGL : glad resolves every entry point on the egl context.

## Build

```
git submodule update --init
cmake -S . -B build
cmake --build build
```

The executables land in `bin/`.
