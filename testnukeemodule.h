#ifndef TESTNUKEEMODULE_H
#define TESTNUKEEMODULE_H
#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>

#include <interface/NUKEEInteface.h>   // NUKEModule + AppInstance
#include <interface/Importers.h>       // SAMPLE: register a plugin asset importer + ImporterDefer
#include <API/Model/Texture.h>         // the native asset the sample importer produces
#include <API/Model/resdb.h>           // ResDB register the produced asset
#include <fstream>
#include <imgui/imgui.h>               // drawn through the shared NukeImGui context

// Singletons backed by a function-local static are per-DLL. Since NukeEngine is now a
// shared DLL, the engine's own singletons/registry are shared with the host — but always
// prefer the passed-in `instance` for host state (camera, keyboard, windows).

using namespace std;
using namespace nuke;

#define MODULE_TITLE    "Test NukeEngine module"
#define MODULE_AUTHOR   "Luastris"
#define MODULE_DESC     "Sample plugin showing how NukeEngine loads custom modules."
#define MODULE_VERSION  "1.0.0.0"
#define MODULE_SITE     "https://luastris.com"

#define MSG_ON_KB       "[TestPlugin] key [M] pressed"

#define WINDOW_TITLE    "Test Nuke Window"
#define WINDOW_BODY_TXT "This window is drawn by a loaded plugin (TestNUKEModule.dll).\n\nIt registers an ImGui window via AppInstance::PushWindow and a keyboard callback (press M). This proves the gameplay-plugin pipeline: boost::dll load -> Run() on a worker thread -> shared engine DLL state."
#define WINDOW_KEY      "testnukemodule-testwin"

#define MENU_PATH       "Plugins/TestPlugin"
#define MENU_MSG        "Message"
#define MENU_WIN        "Toggle test window"

// ============== Callbacks ================

// Keyboard handler: prints a message when 'm' is pressed.
void MsgPrint(unsigned char c, int x, int y) {
    if (c == 'm')
        cout << MSG_ON_KB << endl;
}

// Menu callbacks.
void Message() { cout << "[TestPlugin] menu -> Message clicked" << endl; }

static AppInstance* g_app = nullptr;   // host, for the persisted window-open flag

// --- SAMPLE plugin importer (interface/Importers.h) -------------------------------------------
// Demonstrates adding support for a format the engine core can't read. This toy importer turns a
// ".democolor" text file (contents: "R G B", each 0-255) into a native 128x128 solid-colour .nutex.
// Real importers put their decoder here (EPS rasteriser, PSD reader, a studio's mesh format, ...).
// AssImporter::ImportAny dispatches here for the registered extension; the browser Import dialog +
// Explorer drag&drop both funnel through it. Runs on the import worker — do file IO here, and route
// every ResDB write through AssImporter::Reg (the engine applies it on the main thread).
static void RegisterDemoImporter()
{
    AssetImporter imp;
    imp.label = "Demo Solid Colour";
    imp.exts  = { ".democolor" };
    imp.import = [](const std::string& srcPath, const std::string& destDir) -> bool
    {
        int r = 255, g = 0, b = 255;
        { std::ifstream f(srcPath); if (f) f >> r >> g >> b; }
        auto cl = [](int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); };
        r = cl(r); g = cl(g); b = cl(b);

        Texture* tex = new Texture();                 // build a native engine asset
        tex->guid = ResDB::NewGuid();
        tex->width = tex->height = 128; tex->format = Texture::FMT_RGBA8; tex->mipCount = 1;
        tex->pixels.resize((size_t)128 * 128 * 4);
        for (size_t i = 0; i < tex->pixels.size(); i += 4)
        { tex->pixels[i] = (unsigned char)r; tex->pixels[i+1] = (unsigned char)g; tex->pixels[i+2] = (unsigned char)b; tex->pixels[i+3] = 255; }

        std::string base = srcPath;                   // stem (no dir, no ext)
        size_t slash = base.find_last_of("/\\"); if (slash != std::string::npos) base = base.substr(slash + 1);
        size_t dot = base.find_last_of('.');      if (dot   != std::string::npos) base = base.substr(0, dot);
        std::string out = destDir + std::string("/") + base + ".nutex";
        if (!tex->SaveToFile(out)) { delete tex; return false; }

        ImporterDefer([tex, out] {                  // ResDB isn't thread-safe -> main thread
            ResDB::getSingleton()->RegisterTexture(tex);
            ResDB::getSingleton()->SetAssetPath(tex->guid, out);
        });
        std::cout << "[TestPlugin]\timported '" << srcPath << "' -> " << out << std::endl;
        return true;
    };
    RegisterImporter(imp);
}

void TestWindow() {
    if (!g_app) return;
    bool* open = g_app->WindowOpen(WINDOW_KEY);   // host-owned, persisted by the editor
    if (!*open) return;
    ImGui::Begin(WINDOW_TITLE, open, 0);
    ImGui::TextWrapped(WINDOW_BODY_TXT);
    ImGui::End();
}

void toggleTestWindow() { if (g_app) { bool* o = g_app->WindowOpen(WINDOW_KEY); *o = !*o; } }

// =========================================

struct TestNUKEEModule : public NUKEModule
{
    // Fill in metadata before the plugin is loaded.
    TestNUKEEModule()
    {
        strcpy(title, MODULE_TITLE);
        strcpy(author, MODULE_AUTHOR);
        strcpy(description, MODULE_DESC);
        strcpy(version, MODULE_VERSION);
        strcpy(site, MODULE_SITE);
    }

    // Runs on a background thread. `instance` is the host (Editor.exe now, Player.exe later).
    void Run(AppInstance* instance) override
    {
        this->instance = instance;
        g_app = instance;
        stopped = false;

        cout << "[TestPlugin]\tRun() instance=" << instance
             << " editor=" << (instance->isEditor() ? "yes" : "no") << endl;

        // Draw our window through the host UI.
        instance->PushWindow(WINDOW_KEY, &TestWindow);

        // Add menu items: Plugins > TestPlugin > { Message, Toggle test window }.
        if (instance->menuStrip)
        {
            instance->menuStrip->AddItem(MENU_PATH, MENU_MSG, &Message);
            instance->menuStrip->AddItem(MENU_PATH, MENU_WIN, &toggleTestWindow);
        }

        // Hook the host keyboard (press 'm').
        if (instance->keyboard)
            *instance->keyboard += MsgPrint;

        RegisterDemoImporter();   // SAMPLE: a plugin asset importer (.democolor -> .nutex)

        cout << "[TestPlugin]\tloaded." << endl;
    }

    bool HasSettings() override { return true; }

    // Drawn inline as a panel inside the Plugin Manager (called every frame).
    void Settings() override
    {
        if (g_app) ImGui::Checkbox("Show test window", g_app->WindowOpen(WINDOW_KEY));
        ImGui::TextWrapped("This panel is rendered by the plugin, inside the manager.");
    }

    // Called on unload / app close: tear down everything we registered.
    void Shutdown() override
    {
        if (g_app) *g_app->WindowOpen(WINDOW_KEY) = false;
        if (instance)
        {
            instance->PopWindow(WINDOW_KEY);
            if (instance->menuStrip)
                instance->menuStrip->RemoveItem(MENU_PATH);   // drop the whole Plugins/TestPlugin branch
        }
        stopped = true;
        cout << "[TestPlugin]\tunloaded." << endl;
    }
};

// Exported under the unmangled symbol "plugin" — InitModules looks this up via boost::dll.
extern "C" BOOST_SYMBOL_EXPORT TestNUKEEModule plugin;
TestNUKEEModule plugin;

#endif // TESTNUKEEMODULE_H
