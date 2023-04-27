#pragma once

#include "hikogui/module.hpp"
#include "hikogui/codec/png.hpp"
#include "hikogui/GUI/gui_system.hpp"
#include "hikogui/GFX/RenderDoc.hpp"
#include "hikogui/GFX/gfx_surface_delegate_vulkan.hpp"
#include "hikogui/widgets/widget.hpp"
#include "json.hpp"

#include <synthium/manager.h>

class ModelRenderer;

// Every widget must inherit from hi::widget.
class model_widget : public hi::widget, public hi::gfx_surface_delegate_vulkan {
public:
    // Every constructor of a widget starts with a `window` and `parent` argument.
    // In most cases these are automatically filled in when calling a container widget's `make_widget()` function.
    model_widget(
        hi::widget *parent,
        hi::gfx_surface &surface,
        std::shared_ptr<synthium::Manager> manager
    ) noexcept;

    ~model_widget();

    void load_model(std::string name);

    // The set_constraints() function is called when the window is first initialized,
    // or when a widget wants to change its constraints.
    [[nodiscard]] hi::box_constraints update_constraints() noexcept override;

    // The `set_layout()` function is called when the window has resized, or when
    // a widget wants to change the internal layout.
    //
    // NOTE: The size of the layout may be larger than the maximum constraints of this widget.
    void set_layout(hi::widget_layout const& context) noexcept override;

    // The `draw()` function is called when all or part of the window requires redrawing.
    // This may happen when showing the window for the first time, when the operating-system
    // requests a (partial) redraw, or when a widget requests a redraw of itself.
    //
    // This draw() function only draws the GUI part of the widget, there is another draw() function
    // that will draw the 3D part.
    void draw(hi::draw_context const& context) noexcept override;

    // This draw() function draws the 3D model.
    // It is called before the GUI is drawn and allows drawing directly onto the swap-chain.
    //
    // As HikoGUI reuses previous drawing of the swap-chain it is important to let the render-pass
    // load the data from the frame-buffer (not set to don't-care) and to not render outside the @a render_area.
    void draw(uint32_t swapchain_index, vk::Semaphore start, vk::Semaphore finish, vk::Rect2D render_area) noexcept override;

    // This function is called when the vulkan-device changes.
    void build_for_new_device(
        VmaAllocator allocator,
        vk::Instance instance,
        vk::Device device,
        vk::Queue graphics_queue,
        uint32_t graphics_queue_family_index) noexcept override;

    // This function is called when the swap-chain changes, this can happen:
    //  - When a new window is created with this widget.
    //  - When the widget is moved to another window.
    //  - When the size of the window changes.
    //
    void build_for_new_swapchain(std::vector<vk::ImageView> const& views, vk::Extent2D size, vk::SurfaceFormatKHR format) noexcept
        override;

    // This function is called when the vulkan-device has gone away.
    // This may happen:
    //  - When the application is closed.
    //  - When the GPU device has a problem.
    void teardown_for_device_lost() noexcept override;

    // This function is called the surface is going away.
    // This may happen:
    //  - The window is closed.
    //  - The widget is being moved to another window.
    //  - The window is resizing.
    void teardown_for_swapchain_lost() noexcept override;

    // Override this function when your widget needs to be controllable by mouse interaction.
    [[nodiscard]] hi::hitbox hitbox_test(hi::point2i position) const noexcept override;

    // Override the handle_event(command) to handle high level commands.
    bool handle_event(hi::gui_event const& event) noexcept override;

private:
    hi::gfx_surface &_surface;
    std::vector<uint8_t> model_data, asset_data;
    std::shared_ptr<ModelRenderer> m_renderer;
    std::shared_ptr<synthium::Manager> m_manager;
    VkRect2D _view_port;
    bool swapchainInitialized = false;

    bool started_rotation = false, started_pan = false;
};