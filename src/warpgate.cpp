// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
#include "hikogui/crt.hpp"

#include "hikogui/loop.hpp"
#include "hikogui/task.hpp"
#include "hikogui/ranges.hpp"
#include "utils/hikogui/widgets/model_widget.hpp"
#include <ranges>
#include <cassert>
#include <filesystem>
#include <spdlog/spdlog.h>

#include <synthium/manager.h>

// This is a co-routine that manages the main window.
hi::task<> main_window(hi::gui_system& gui)
{
    // Load the icon to show in the upper left top of the window.
    auto icon = hi::icon(hi::png::load(hi::URL{"resource:warpgate.png"}));

    // Create a window, when `window` gets out-of-scope the window is destroyed.
    auto window = gui.make_window(hi::label{std::move(icon), "Warpgate"});

    std::filesystem::path server("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/");
    std::vector<std::filesystem::path> assets;
    for(int i = 0; i < 24; i++) {
        assets.push_back(server / ("assets_x64_" + std::to_string(i) + ".pack2"));
    }
    std::shared_ptr<synthium::Manager> manager = std::make_shared<synthium::Manager>(assets);

    // Create the vulkan triangle-widget as the content of the window. The content
    // of the window is a grid, we only use the cell "A1" for this widget.
    window->content().make_widget<model_widget>("A1", *window->surface, manager);

    // Wait until the window is "closing" because the operating system says so, or when
    // the X is pressed.
    co_await window->closing;
}

// The main (platform independent) entry point of the application.
int hi_main(int argc, char *argv[])
{
    // Start the RenderDoc server so that the application is easy to debug in RenderDoc.
    auto doc = hi::RenderDoc{};

    // Start the GUI-system.
    auto gui = hi::gui_system::make_unique();

    // Create and manage the main-window.
    main_window(*gui);

    // Start the main-loop until the main-window is closed.
    return hi::loop::main().resume();
}
