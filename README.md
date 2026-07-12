# TestNUKEModule

The pristine sample plugin for [NukeEngine](https://github.com/Luastris/NukeEngine-Eco) —
a minimal, fully commented reference for writing your own module. The documentation IS
the code: read [testnukeemodule.h](testnukeemodule.h).

## What it demonstrates

- The unified plugin model: a `NUKEModule` subclass exported under the unmangled symbol
  `plugin` (`BOOST_SYMBOL_EXPORT`), loaded by the host via boost::dll and `Run()` on a
  worker thread.
- Module metadata (title/author/description/version/site) shown in the Plugin Manager.
- Registering an ImGui window through the host (`AppInstance::PushWindow`, host-persisted
  open flag via `WindowOpen(key)`), menu items (`menuStrip->AddItem`), and a keyboard
  hook — plus tearing ALL of it down in `Shutdown()`.
- An inline `Settings()` panel rendered inside the Plugin Manager.

It draws through the shared [NukeImGui](https://github.com/Luastris/NukeImGui) DLL, which
makes it an **editor-only** module: packaging auto-excludes it from game dists.

## Building

Part of the [NukeEngine-Eco](https://github.com/Luastris/NukeEngine-Eco) superbuild, or
standalone: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64` +
`cmake --build build --config Debug` (needs `VCPKG_ROOT`; engine first). The built DLL
goes to the editor's `modules/` folder; enable it per project in the Plugin Manager.
