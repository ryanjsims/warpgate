#pragma warning( disable : 4250 )
#include "utils/gtk/window.hpp"
#include <gtkmm/application.h>
#include <gtkmm/settings.h>

#include <chrono>
#include <windows.h>
#include "utils/renderdoc_app.h"

using namespace std::chrono_literals;

class RenderDoc {
public:
    RenderDoc() {
         auto dll_urls = std::vector{
            std::filesystem::path{"renderdoc.dll"},
            std::filesystem::path{"C:/Program Files/RenderDoc/renderdoc.dll"},
            std::filesystem::path{"C:/Program Files (x86)/RenderDoc/renderdoc.dll"}};

        auto mod = [&]() -> HMODULE {
            for (auto& dll_url : dll_urls) {
                spdlog::debug("Trying to load: {}", dll_url.string());

                if (auto mod = LoadLibraryW(dll_url.native().c_str())) {
                    return mod;
                }
            }
            return nullptr;
        }();

        if (mod == nullptr) {
            spdlog::warn("Could not load renderdoc.dll");
            return;
        }

        auto RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        if (RENDERDOC_GetAPI == nullptr) {
            spdlog::error("Could not find RENDERDOC_GetAPI in renderdoc.dll");
            return;
        }

        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, &api);
        if (ret != 1) {
            spdlog::error("RENDERDOC_GetAPI returns invalid value {}", ret);
            api = nullptr;
        }

        // At init, on linux/android.
        // For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
        // if(void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
        //{
        //    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        //    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
        //    assert(ret == 1);
        //}

        set_overlay(false, false, false);
    }

    void set_overlay(bool frameRate, bool frameNumber, bool captureList) noexcept {
        if (not api) {
            return;
        }

        uint32_t or_mask = eRENDERDOC_Overlay_None;
        uint32_t and_mask = eRENDERDOC_Overlay_None;

        if (frameRate || frameNumber || captureList) {
            or_mask |= eRENDERDOC_Overlay_Enabled;
        } else {
            and_mask |= eRENDERDOC_Overlay_Enabled;
        }

        if (frameRate) {
            or_mask |= eRENDERDOC_Overlay_FrameRate;
        } else {
            and_mask |= eRENDERDOC_Overlay_FrameRate;
        }

        if (frameNumber) {
            or_mask |= eRENDERDOC_Overlay_FrameNumber;
        } else {
            and_mask |= eRENDERDOC_Overlay_FrameNumber;
        }

        if (captureList) {
            or_mask |= eRENDERDOC_Overlay_CaptureList;
        } else {
            and_mask |= eRENDERDOC_Overlay_CaptureList;
        }

        auto& api_ = *static_cast<RENDERDOC_API_1_4_1 *>(api);

        and_mask = ~and_mask;
        api_.MaskOverlayBits(and_mask, or_mask);
    }
private:
    void* api;
};

//#ifdef _WIN32
//int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR win_argv, int nCmdShow)
//#else
int main(int argc, char** argv)
//#endif
{
    //#ifdef _WIN32
    // int argc = __argc;
    // char** argv = (char**)win_argv;
    //#endif
    RenderDoc doc{};

    std::shared_ptr<Gtk::Application> app = Gtk::Application::create("org.warpgate.exporter");
    auto settings = Gtk::Settings::get_default();
    settings->set_property<gboolean>("gtk-application-prefer-dark-theme", true);
    auto icon_theme = Gtk::IconTheme::get_for_display(Gdk::Display::get_default());
    icon_theme->add_search_path("share/icons/hicolor/symbolic/apps/");
    if(icon_theme->has_icon("warpgate")) {
        Gtk::Window::set_default_icon_name("warpgate");
    } else {
        spdlog::warn("'warpgate' icon not found");
    }


    try {
        return app->make_window_and_run<warpgate::gtk::Window>(argc, argv);
    } catch(Glib::Error &e) {
        spdlog::error("Glib error: {} - {} - {}", e.code(), g_quark_to_string(e.domain()), e.what());
        return e.code();
    }
}
