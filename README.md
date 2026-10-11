# webwin

The Win32 API for C++ code compiled to WebAssembly with Emscripten: `windows.h` and the other SDK headers, MSVC compatibility (intrinsics, keywords, `rand`), kernel objects (threads, events, mutexes, semaphores), files, virtual memory, and the window and monitor calls.

Threads run on Emscripten pthreads when the page is cross-origin isolated, or on a cooperative fiber scheduler on a single thread otherwise.

## Use

```cmake
set(WEBWIN_PTHREADS ON)   # pthreads build; OFF builds the fiber scheduler
add_subdirectory(webwin)
target_link_libraries(app PRIVATE webwin)
```

Linking `webwin` adds its include directories and force-includes `compat/msvc_compat.h` in C++ sources. The page must serve the files listed in the target's `WEB_FILES` property next to the wasm: `web_fs.js` (game data folder access) and `coi-serviceworker.js` (cross-origin isolation on static hosting).

## Origin

Extracted from the web port of [KisakBlack](https://github.com/SwagSoftware/KisakBlack) by [riicchhaarrd](https://github.com/riicchhaarrd/KisakBlack/tree/web-port). Licensed under the GPL-3.0, like KisakBlack.
