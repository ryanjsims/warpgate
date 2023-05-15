#include "dme_loader.h"
#include "utils/vulkan/model_renderer.hpp"
#include "gli/texture.hpp"
#include "utils/hikogui/widgets/model_widget.hpp"

#include <fstream>

// Every constructor of a widget starts with a `window` and `parent` argument.
// In most cases these are automatically filled in when calling a container widget's `make_widget()` function.
model_widget::model_widget(
    hi::widget *parent,
    hi::gfx_surface &surface,
    std::shared_ptr<synthium::Manager> manager
) noexcept : widget(parent), _surface(surface), m_manager(manager) {
    _surface.add_delegate(this);
}

model_widget::~model_widget()
{
    _surface.remove_delegate(this);
}

void model_widget::set_manager(std::shared_ptr<synthium::Manager> manager) {
    m_manager = manager;
}

void model_widget::load_model(std::string name) {
    if(m_manager == nullptr) {
        return;
    }
    
    std::shared_ptr<synthium::Asset2> asset = m_manager->get(name);
    if(asset == nullptr) {
        return;
    }
    model_data = asset->get_data();
    std::shared_ptr<warpgate::DME> model = std::make_shared<warpgate::DME>(model_data, name);
    std::unordered_map<uint32_t, gli::texture> textures;
    for(uint32_t material_index = 0; material_index < model->dmat()->material_count(); material_index++) {
        for(uint32_t parameter_index = 0; parameter_index < model->dmat()->material(material_index)->param_count(); parameter_index++) {
            const warpgate::Parameter& parameter = model->dmat()->material(material_index)->parameter(parameter_index);
            switch(parameter.type()){
                case warpgate::Parameter::D3DXParamType::TEXTURE1D:
                case warpgate::Parameter::D3DXParamType::TEXTURE2D:
                case warpgate::Parameter::D3DXParamType::TEXTURE3D:
                case warpgate::Parameter::D3DXParamType::TEXTURE:
                case warpgate::Parameter::D3DXParamType::TEXTURECUBE:
                    break;
                default:
                    continue;
            }

            if(textures.find(parameter.get<uint32_t>(parameter.data_offset())) != textures.end()) {
                continue;
            }

            std::optional<std::string> texture_name = model->dmat()->material(material_index)->texture(parameter.semantic_hash());
            if(!texture_name.has_value()) {
                continue;
            }
            asset = m_manager->get(*texture_name);
            asset_data = asset->get_data();
            textures[parameter.get<uint32_t>(parameter.data_offset())] = gli::texture(gli::load_dds((char*)asset_data.data(), asset_data.size()));
        }
    }

    m_renderer->loadModel(model, textures);
}

void model_widget::set_faction(uint32_t faction) {
    m_renderer->set_faction(faction);
}

// The set_constraints() function is called when the window is first initialized,
// or when a widget wants to change its constraints.
[[nodiscard]] hi::box_constraints model_widget::update_constraints() noexcept
{
    // Almost all widgets will reset the `_layout` variable here so that it will
    // trigger the calculations in `set_layout()` as well.
    _layout = {};

    // Certain expensive calculations, such as loading of images and shaping of text
    // can be done in this function.

    // The constrains below have different minimum, preferred and maximum sizes.
    // When the window is initially created it will try to size itself so that
    // the contained widgets are at their preferred size. Having a different minimum
    // and/or maximum size will allow the window to be resizable.
    return {{1024, 860}, {1024, 860}, {1024, 860}, hi::alignment{}, theme().margin()};
}

// The `set_layout()` function is called when the window has resized, or when
// a widget wants to change the internal layout.
//
// NOTE: The size of the layout may be larger than the maximum constraints of this widget.
void model_widget::set_layout(hi::widget_layout const& context) noexcept
{
    // Update the `_layout` with the new context, in this case we want to do some
    // calculations when the size or location of the widget was changed.
    if (compare_store(_layout, context)) {
        auto view_port = context.rectangle_on_window();
        auto window_height = context.window_size.height();

        // We calculate the view-port used for 3D rendering from the location and size
        // of the widget within the window. We use the window-height so that we can make
        // Vulkan compatible coordinates. Vulkan uses y-axis down, while HikoGUI uses y-axis up.
        _view_port = VkRect2D{
            VkOffset2D{hi::narrow_cast<int32_t>(view_port.left()), hi::narrow_cast<int32_t>(window_height - view_port.top())},
            VkExtent2D{hi::narrow_cast<uint32_t>(view_port.width()), hi::narrow_cast<uint32_t>(view_port.height())}};
    }
}

// The `draw()` function is called when all or part of the window requires redrawing.
// This may happen when showing the window for the first time, when the operating-system
// requests a (partial) redraw, or when a widget requests a redraw of itself.
//
// This draw() function only draws the GUI part of the widget, there is another draw() function
// that will draw the 3D part.
void model_widget::draw(hi::draw_context const& context) noexcept
{
    // We request a redraw for each frame, in case the 3D model changes on each frame.
    // In normal cases we should take into account if the 3D model actually changes before requesting a redraw.
    request_redraw();

    // We only need to draw the widget when it is visible and when the visible area of
    // the widget overlaps with the scissor-rectangle (partial redraw) of the drawing context.
    if (*mode > hi::widget_mode::invisible and overlaps(context, layout())) {
        // The 3D drawing will be done directly on the swap-chain before the GUI is drawn.
        // By making a hole in the GUI we can show the 3D drawing underneath it, otherwise
        // the solid-background color of the GUI would show instead.
        context.draw_hole(_layout, _layout.rectangle());
    }
}

// This draw() function draws the 3D model.
// It is called before the GUI is drawn and allows drawing directly onto the swap-chain.
//
// As HikoGUI reuses previous drawing of the swap-chain it is important to let the render-pass
// load the data from the frame-buffer (not set to don't-care) and to not render outside the @a render_area.
void model_widget::draw(uint32_t swapchain_index, vk::Semaphore start, vk::Semaphore finish, vk::Rect2D render_area) noexcept
{
    assert(m_renderer != nullptr);

    // The _triangle_example is the "vulkan graphics engine", into which we pass:
    //  - Which swap-chain image to draw into,
    //  - the semaphores when to start drawing, and when the drawing is finished.
    //  - The render-area, which is like the dirty-rectangle that needs to be redrawn.
    //  - View-port the part of the frame buffer that matches this widget's rectangle.
    //
    // The "vulkan graphics engine" is responsible to not drawn outside the neither
    // the render-area nor outside the view-port.
    m_renderer->render(
        swapchain_index,
        static_cast<VkSemaphore>(start),
        static_cast<VkSemaphore>(finish),
        static_cast<VkRect2D>(render_area),
        _view_port);
}

// This function is called when the vulkan-device changes.
void model_widget::build_for_new_device(
    VmaAllocator allocator,
    vk::Instance instance,
    vk::Device device,
    vk::Queue graphics_queue,
    uint32_t graphics_queue_family_index) noexcept
{
    // In our case if the vulkan-device changes, then we restart the complete "graphics engines".
    m_renderer = std::make_shared<ModelRenderer>(
        allocator,
        static_cast<VkDevice>(device),
        static_cast<VkQueue>(graphics_queue),
        graphics_queue_family_index
    );
}

// This function is called when the swap-chain changes, this can happen:
//  - When a new window is created with this widget.
//  - When the widget is moved to another window.
//  - When the size of the window changes.
//
void model_widget::build_for_new_swapchain(std::vector<vk::ImageView> const& views, vk::Extent2D size, vk::SurfaceFormatKHR format) noexcept
{
    assert(m_renderer != nullptr);

    // Create a list of old style VkImageView of the swap-chain, HikoGUI uses the C++ vulkan bindings internally.
    auto views_ = hi::make_vector<VkImageView>(std::views::transform(views, [](auto const& view) {
        return static_cast<VkImageView>(view);
    }));

    // Tell the "graphics engine" to make itself ready for a new swap-chain.
    // This often means the setup of most of the graphics pipelines and render-passes.
    m_renderer->buildForNewSwapchain(views_, static_cast<VkExtent2D>(size), static_cast<VkFormat>(format.format));
}

// This function is called when the vulkan-device has gone away.
// This may happen:
//  - When the application is closed.
//  - When the GPU device has a problem.
void model_widget::teardown_for_device_lost() noexcept
{
    // We shutdown the "graphics engine"
    m_renderer = {};
}

// This function is called the surface is going away.
// This may happen:
//  - The window is closed.
//  - The widget is being moved to another window.
//  - The window is resizing.
void model_widget::teardown_for_swapchain_lost() noexcept
{
    assert(m_renderer != nullptr);

    // Tell the graphics engine to tear down the pipelines and render-passes and everything
    // that is connected to the swap-chain.
    m_renderer->teardownForLostSwapchain();
}

// Override this function when your widget needs to be controllable by mouse interaction.
[[nodiscard]] hi::hitbox model_widget::hitbox_test(hi::point2i position) const noexcept
{
    // Check if the (mouse) position is within the visual-area of the widget.
    // The hit_rectangle is the _layout.rectangle() intersected with the _layout.clipping_rectangle.
    if (*mode >= hi::widget_mode::partial && layout().contains(position)) {
        // The `this` argument allows the gui_window to forward mouse events to handle_event(mouse) of this widget.
        // The `position` argument is used to handle widgets that are visually overlapping, widgets with higher elevation
        // get priority. When this widget is enabled it should show a button-cursor, otherwise just the normal arrow.
        return {
            id, _layout.elevation, *mode >= hi::widget_mode::partial ? hi::hitbox_type::button : hi::hitbox_type::_default};

    } else {
        return {};
    }
}

// Override the handle_event(command) to handle high level commands.
bool model_widget::handle_event(hi::gui_event const& event) noexcept
{
    switch (event.type()) {
    case hi::gui_event_type::gui_activate:
        if (*mode >= hi::widget_mode::partial) {
            // Handle activate, by default the "spacebar" causes this command.
            return true;
        }
        break;

    case hi::gui_event_type::keyboard_grapheme:
        hi_log_error("User typed the letter U+{:x}.", static_cast<uint32_t>(get<0>(event.grapheme())));
        return true;

    case hi::gui_event_type::mouse_up:
        if (*mode >= hi::widget_mode::partial and event.is_left_button_up(_layout.rectangle())) {
            return handle_event(hi::gui_event_type::gui_activate);
        } else if(*mode >= hi::widget_mode::partial && event.mouse().cause.middle_button && started_rotation) {
            m_renderer->camera().commit_rotation();
            started_rotation = false;
            return true;
        } else if(*mode >= hi::widget_mode::partial && event.mouse().cause.middle_button && started_pan) {
            m_renderer->camera().commit_pan();
            started_pan = false;
            return true;
        }
        break;

    case hi::gui_event_type::mouse_drag:
        if (*mode >= hi::widget_mode::partial && event.mouse().down.middle_button && (event.keyboard_modifiers == hi::keyboard_modifiers::none || started_rotation) && !started_pan) {
            started_rotation = true;
            spdlog::trace("drag_delta = ({}, {})", event.drag_delta().x(), event.drag_delta().y());
            m_renderer->camera().rotate_about_target(
                (float)event.drag_delta().x() / (float)_view_port.extent.width * 360.0f,
                (float)event.drag_delta().y() / (float)_view_port.extent.height * -360.0f
            );
            return true;
        } else if(*mode >= hi::widget_mode::partial && event.mouse().down.middle_button && (event.keyboard_modifiers == hi::keyboard_modifiers::shift || started_pan) && !started_rotation) {
            started_pan = true;
            m_renderer->camera().pan(
                (m_renderer->camera().right() * -1.0f * ((float)event.drag_delta().x() / (float)_view_port.extent.width)
                + m_renderer->camera().up() * -1.0f * ((float)event.drag_delta().y() / (float)_view_port.extent.height))
                * m_renderer->camera().radius()
            );
            return true;
        }
        break;
    
    case hi::gui_event_type::mouse_wheel:
        if (*mode >= hi::widget_mode::partial) {
            spdlog::trace("wheel_delta = ({}, {})", event.mouse().wheel_delta.x(), event.mouse().wheel_delta.y());
            m_renderer->camera().translate(m_renderer->camera().forward() * (float)event.mouse().wheel_delta.y() / std::abs((float)event.mouse().wheel_delta.y()) * -0.1f * m_renderer->camera().radius());
            m_renderer->camera().commit_translate();
            return true;
        }
        break;

    default:;
    }

    // The default handle_event() will handle hovering and auto-scrolling.
    return widget::handle_event(event);
}