// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
#include "hikogui/crt.hpp"

#include "hikogui/loop.hpp"
#include "hikogui/task.hpp"
#include "hikogui/ranges.hpp"
#include "hikogui/widgets/text_field_delegate.hpp"
#include "hikogui/widgets/text_field_widget.hpp"
#include "hikogui/widgets/momentary_button_widget.hpp"
#include "hikogui/widgets/row_column_widget.hpp"
#include "hikogui/widgets/selection_widget.hpp"
#include "hikogui/widgets/toolbar_button_widget.hpp"
#include "hikogui/widgets/system_menu_widget.hpp"
#include "utils/hikogui/widgets/model_widget.hpp"
#include <ranges>
#include <cassert>
#include <filesystem>
#include <spdlog/spdlog.h>

#include <synthium/manager.h>

namespace hi { inline namespace v1 {
template<std::convertible_to<std::string> T>
class default_text_field_delegate<T> : public text_field_delegate {
public:
    using value_type = T;

    observer<value_type> value;

    default_text_field_delegate(forward_of<observer<value_type>> auto&& value) noexcept : value(hi_forward(value))
    {
        _value_cbt = this->value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    label validate(text_field_widget& sender, std::string_view text) noexcept override
    {
        return {};
    }

    std::string text(text_field_widget& sender) noexcept override
    {
        return *value;
    }

    void set_text(text_field_widget& sender, std::string_view text) noexcept override
    {
        try {
            value = text;
        } catch (std::exception const&) {
            // Ignore the error, don't modify the value.
            return;
        }
    }

private:
    typename decltype(value)::callback_token _value_cbt;
};

class default_momentary_button_delegate : public button_delegate {
public:

    default_momentary_button_delegate() noexcept {}

    void activate(abstract_button_widget& sender) noexcept override
    {
        this->_notifier();
    }
    /// @endprivatesection
};
}}

// This is a co-routine that manages the main window.
hi::task<> main_window(hi::gui_system& gui)
{
    // Load the icon to show in the upper left top of the window.
    auto icon = hi::icon(hi::png::load(hi::URL{"resource:warpgate.png"}));

    // Create a window, when `window` gets out-of-scope the window is destroyed.
    auto window = gui.make_window(hi::label{std::move(icon), "Warpgate"});
 
    
    auto &column = window->content().make_widget<hi::column_widget>("A1");
    hi::observer<std::string> model_name{};
    auto model_name_delegate = std::make_shared<hi::default_text_field_delegate<std::string>>(model_name);
    auto& text_widget = column.make_widget<hi::text_field_widget>(model_name_delegate);
    text_widget.continues = true;

    auto load_delegate = std::make_shared<hi::default_momentary_button_delegate>();
    column.make_widget<hi::momentary_button_widget>(load_delegate, hi::label{"Load"});

    hi::observer<uint32_t> selected_faction = 0;
    hi::observer<std::vector<std::pair<uint32_t, hi::label>>> factions;
    {
        auto proxy = factions.copy();
        proxy->emplace_back(0, hi::label{"NS"});
        proxy->emplace_back(1, hi::label{"VS"});
        proxy->emplace_back(2, hi::label{"NC"});
        proxy->emplace_back(3, hi::label{"TR"});
    }
    column.make_widget<hi::selection_widget>(selected_faction, factions);

    // Create the vulkan triangle-widget as the content of the window. The content
    // of the window is a grid, we only use the cell "A1" for this widget.
    auto& model_view = window->content().make_widget<model_widget>("B1", *window->surface);

    hi::observer<std::shared_ptr<synthium::Manager>> manager_obs;

    std::jthread load_manager([&](){
        std::filesystem::path server("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/");
        std::vector<std::filesystem::path> assets;
        for(int i = 0; i < 24; i++) {
            assets.push_back(server / ("assets_x64_" + std::to_string(i) + ".pack2"));
        }
        std::shared_ptr<synthium::Manager> manager = std::make_shared<synthium::Manager>(assets);
        manager_obs = manager;
    });

    auto manager_load_cb_token = manager_obs.subscribe([&](std::shared_ptr<synthium::Manager> manager){
        model_view.set_manager(manager);
        spdlog::info("Manager loaded.");
    }, hi::callback_flags::main);

    auto load_cb_token = load_delegate->subscribe([&]{
        spdlog::info("Loading model '{}'", (*text_widget.delegate).text(text_widget));
        model_view.load_model((*text_widget.delegate).text(text_widget));
    });

    auto faction_cb_token = selected_faction.subscribe([&](uint32_t value){
        spdlog::info("Faction changed to {}", value);
        model_view.set_faction(value);
    }, hi::callback_flags::local);

    // Wait until the window is "closing" because the operating system says so, or when
    // the X is pressed.
    co_await window->closing;
}

// The main (platform independent) entry point of the application.
int hi_main(int argc, char *argv[])
{
    // Start the RenderDoc server so that the application is easy to debug in RenderDoc.
    //auto doc = hi::RenderDoc{};

    // Start the GUI-system.
    auto gui = hi::gui_system::make_unique();

    // Create and manage the main-window.
    main_window(*gui);

    // Start the main-loop until the main-window is closed.
    return hi::loop::main().resume();
}
