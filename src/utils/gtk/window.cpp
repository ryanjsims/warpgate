#pragma warning( disable : 4250 )
#include "utils/gtk/window.hpp"
#include <thread>
#include <regex>

#include <gtkmm/treelistmodel.h>
#include <gtkmm/treeexpander.h>
#include <gtkmm/error.h>
#include <gtkmm/label.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/gestureclick.h>
#include <giomm/simpleactiongroup.h>
#include <giomm/datainputstream.h>

#include <dme_loader.h>
#include <mrn_loader.h>
#include "utils/adr.h"
#include "utils/materials_3.h"
#include "utils/gltf/common.h"
#include "utils/gltf/dmat.h"

using namespace warpgate::gtk;

struct ParameterData {
    warpgate::Semantic semantic;
    warpgate::Parameter::D3DXParamClass paramclass;
    warpgate::Parameter::D3DXParamType paramtype;
    std::vector<uint8_t> data;
    std::shared_ptr<const warpgate::Material> material;
};

class Asset2ListItem : public Glib::Object {
public:
    Glib::ustring m_name, m_namehash_str;
    std::string m_stdname, m_stdnamehash_str;
    uint64_t m_namehash;
    bool m_compressed;
    uint64_t m_data_length;
    AssetType m_type;

    std::optional<std::string> virtual_parent;
    std::optional<std::string> virtual_type;

    void* user_data = nullptr;

    static std::shared_ptr<Asset2ListItem> create(std::string name, AssetType type = AssetType::UNKNOWN, const synthium::Asset2Raw *asset = nullptr) {
        return Glib::make_refptr_for_instance<Asset2ListItem>(new Asset2ListItem(name, type, asset));
    }

    static AssetType type_from_filename(std::string name) {
        if(name.find(".adr") != std::string::npos) {
            return AssetType::ACTOR_RUNTIME;
        }
        if(name.find(".mrn") != std::string::npos) {
            return AssetType::ANIMATION_NETWORK;
        }
        if(name.find(".dme") != std::string::npos) {
            return AssetType::MODEL;
        }
        if(name.find(".dma") != std::string::npos) {
            return AssetType::PALETTE;
        }
        return AssetType::UNKNOWN;
    }

    static Glib::ustring icon_from_asset_type(AssetType atype) {
        switch(atype) {
        case AssetType::ACTOR_RUNTIME:
            return "package-x-generic-symbolic";
        case AssetType::ANIMATION:
            return "animation-symbolic";
        case AssetType::ANIMATION_NETWORK:
            return "network-wired-symbolic";
        case AssetType::MATERIAL:
            return "emoji-symbols-symbolic";
        case AssetType::MODEL:
            return "application-x-appliance-symbolic";
        case AssetType::PALETTE:
            return "applications-graphics-symbolic";
        case AssetType::PARAMETER:
            return "preferences-system-symbolic";
        case AssetType::ANIMATION_SET:
        case AssetType::SKELETON:
            return "skeleton-symbolic";
        case AssetType::TEXTURE:
            return "image-x-generic-symbolic";
        case AssetType::VALUE:
            return "application-x-addon-symbolic";
        case AssetType::VIRTUAL:
            return "folder-symbolic";
        case AssetType::UNKNOWN:
        default:
            return "emblem-important-symbolic";
        }
    }

    ~Asset2ListItem() override {
        if(user_data != nullptr) {
            delete user_data;
        }
    }

protected:
    Asset2ListItem(std::string name, AssetType type = AssetType::UNKNOWN, const synthium::Asset2Raw *asset = nullptr) {
        m_name.assign(name.c_str());
        m_stdname = name;
        m_type = type;
        if(asset) {
            m_namehash = asset->name_hash;
            m_stdnamehash_str = std::to_string(asset->name_hash);
            m_namehash_str.assign(m_stdnamehash_str.c_str());
            m_compressed = asset->is_zipped();
            m_data_length = asset->data_length;
        } else {
            m_namehash = 0;
            m_compressed = false;
            m_data_length = 0;
        }
    }
};

class LoadedListItem : public Glib::Object {
public:
    Glib::ustring m_name;
    std::string m_stdname;
    AssetType m_type;

    using stack_t = std::vector<std::string>;

    static std::shared_ptr<LoadedListItem> create(std::string name, AssetType type = AssetType::UNKNOWN) {
        return Glib::make_refptr_for_instance<LoadedListItem>(new LoadedListItem(name, type));
    }

    stack_t find_child(std::shared_ptr<LoadedListItem> search) {
        stack_t to_return;
        if(search.get() == this) {
            to_return.push_back(m_stdname);
        } else {
            for(auto it = m_children_by_name.begin(); it != m_children_by_name.end() && to_return.size() == 0; it++) {
                to_return = it->second->find_child(search);
            }
            to_return.push_back(m_stdname);
        }
        return to_return;
    }

    bool add_child(std::shared_ptr<LoadedListItem> child) {
        if(m_children_by_name.find(child->m_stdname) != m_children_by_name.end()) {
            spdlog::warn("Child '{}' already exists in this tree", child->m_stdname);
            return false;
        }
        m_children.push_back(child);
        m_children_by_name[child->m_stdname] = child;
        return true;
    }

    std::shared_ptr<LoadedListItem> get_child(stack_t path) {
        if(path.empty()) {
            return nullptr;
        }
        if(path.back() == m_stdname) {
            path.pop_back();
        }
        auto iter = m_children_by_name.find(path.back());
        if(iter == m_children_by_name.end()) {
            spdlog::error("Path corrupt: '{}' not in '{}'", path.back(), m_stdname);
            return nullptr;
        }
        path.pop_back();
        if(path.size() == 0) {
            return iter->second;
        }
        return iter->second->get_child(path);
    }

    std::vector<std::shared_ptr<LoadedListItem>> children() const {
        return m_children;
    }

protected:
    std::vector<std::shared_ptr<LoadedListItem>> m_children;
    std::unordered_map<std::string, std::shared_ptr<LoadedListItem>> m_children_by_name;

    LoadedListItem(std::string name, AssetType type) {
        m_name.assign(name.c_str());
        m_stdname = name;
        m_type = type;
    }
};

class NamelistFilter : public Gtk::Filter {
public:
    NamelistFilter(std::regex compare) 
        : Gtk::Filter()
        , m_compare(compare)
    {

    }
    ~NamelistFilter() {}

    void set_compare(std::regex compare) {
        m_compare = compare;
    }

    std::regex get_compare() {
        return m_compare;
    }

    bool match(const Glib::RefPtr<Glib::ObjectBase>& item) {
        return match_vfunc(item);
    }

protected:
    bool match_vfunc(const Glib::RefPtr<Glib::ObjectBase>& item) override {
        auto namelist_item = std::dynamic_pointer_cast<Asset2ListItem>(item);
        if(!namelist_item) {
            return false;
        }

        return std::regex_search(namelist_item->m_stdname, m_compare);
    }


private:
    std::regex m_compare;
};

GenerateNamelistState::GenerateNamelistState()
    : Glib::ObjectBase("GenerateNamelistState")
    , property_finished(*this, "finished", true)
    , pack_index(0)
    , asset_index(0)
    , items_completed(0)
    , total_items(0)
{

}

GenerateNamelistState::~GenerateNamelistState() {}

ExportModelState::ExportModelState()
    : Glib::ObjectBase("ExportModelState")
    , property_finished(*this, "finished", true)
    , property_export_item(*this, "export_item", nullptr)
    , property_path(*this, "path", ".")
{}

ExportModelState::~ExportModelState() {}

std::shared_ptr<warpgate::utils::ActorSockets> ExportModelState::actorSockets = nullptr;

Window::Window() 
    : m_manager(nullptr)
{
    set_title("Warpgate");
    set_default_size(1280, 960);
    set_icon_name("warpgate");
    set_child(m_box_root);

    auto win_menu = Gio::Menu::create();

    auto file_menu = Gio::Menu::create();
    win_menu->append_submenu("_File", file_menu);

    auto file_section = Gio::Menu::create();
    file_section->append("Load _Namelist...", "warpgate.load_namelist");
    file_section->append("_Generate Namelist...", "warpgate.gen_namelist");
    file_menu->append_section(file_section);

    auto file_section2 = Gio::Menu::create();
    file_section2->append("_Export...", "warpgate.export");
    file_menu->append_section(file_section2);

    auto file_section3 = Gio::Menu::create();
    file_section3->append("_Quit", "warpgate.quit");
    file_menu->append_section(file_section3);

    auto action_group = Gio::SimpleActionGroup::create();
    action_group->add_action("load_namelist", sigc::mem_fun(*this, &Window::on_load_namelist));
    auto action = action_group->add_action("gen_namelist", sigc::mem_fun(*this, &Window::on_gen_namelist));
    m_generate_enable_binding = Glib::Binding::bind_property(m_generator.property_finished.get_proxy(), action->property_enabled());
    action_group->add_action("export", sigc::mem_fun(*this, &Window::on_export));
    action_group->add_action("quit", sigc::mem_fun(*this, &Window::on_quit));
    insert_action_group("warpgate", action_group);

    m_context_menu = Gio::Menu::create();

    auto section1 = Gio::Menu::create();
    section1->append("_Export", "loaded.export");
    section1->append("_Remove", "loaded.remove");
    m_context_menu->append_section(section1);
    
    auto context_action_group = Gio::SimpleActionGroup::create();
    context_action_group->add_action("export", sigc::mem_fun(*this, &Window::on_export_loaded));
    context_action_group->add_action("remove", sigc::mem_fun(*this, &Window::on_remove_loaded));
    insert_action_group("loaded", context_action_group);

    auto namelist_filter_store = Gio::ListStore<Gtk::FileFilter>::create();
    auto txt_filter = Gtk::FileFilter::create();
    txt_filter->set_name("Text files (.txt)");
    txt_filter->add_mime_type("text/plain");
    namelist_filter_store->append(txt_filter);

    m_namelist_dialog = Gtk::FileDialog::create();
    
    // Yes, please use std::string for utf-8 strings, when the STL in c++20
    //      absolutely **does not** support conversion from u8string to string...
    //      I love it so much more than even just using a Glib::ustring like
    //      ***every other*** API uses for that purpose... >.>
    //m_gen_namelist_dialog->set_initial_name("namelist.txt");

    //At least using the C API works I guess, but I *shouldn't* have to use it
    gtk_file_dialog_set_initial_name(m_namelist_dialog->gobj(), "namelist.txt");
    m_namelist_dialog->set_filters(namelist_filter_store);
    m_namelist_dialog->set_default_filter(txt_filter);

    auto model_filter_store = Gio::ListStore<Gtk::FileFilter>::create();
    auto gltf_filter = Gtk::FileFilter::create();
    gltf_filter->set_name("glTF model files (.gltf/.glb)");
    gltf_filter->add_mime_type("model/gltf+json");
    gltf_filter->add_mime_type("model/gltf-binary");
    model_filter_store->append(gltf_filter);

    m_model_dialog = Gtk::FileDialog::create();
    m_model_dialog->set_filters(model_filter_store);
    m_model_dialog->set_default_filter(gltf_filter);    

    m_menubar.set_menu_model(win_menu);
    m_menubar.set_can_focus(false);
    m_menubar.set_focusable(false);

    m_progress_visible_binding = Glib::Binding::bind_property(
        m_generator.property_finished.get_proxy(),
        m_progress_bar.property_visible(),
        Glib::Binding::Flags::INVERT_BOOLEAN
    );

    m_progress_bar.set_hexpand(true);
    m_progress_bar.set_visible(false);

    m_box_status.append(m_status_bar);
    m_box_status.append(m_status_separator);
    m_box_status.append(m_progress_bar);

    m_box_root.append(m_menubar);
    m_box_root.append(m_pane_root);
    m_box_root.append(m_box_status);

    m_pane_lists.set_orientation(Gtk::Orientation::VERTICAL);
    
    auto namelist_factory = Gtk::SignalListItemFactory::create();
    namelist_factory->signal_setup().connect(sigc::mem_fun(*this, &Window::on_setup_namelist_item));
    namelist_factory->signal_bind().connect(sigc::mem_fun(*this, &Window::on_bind_namelist_item));

    m_namelist_root = create_namelist_model();

    m_tree_namelist = Gtk::TreeListModel::create(m_namelist_root, sigc::mem_fun(*this, &Window::create_namelist_model), false, false);
    m_select_namelist = Gtk::SingleSelection::create(m_tree_namelist);
    m_select_namelist->set_can_unselect();
    m_select_namelist->signal_selection_changed().connect(sigc::mem_fun(*this, &Window::on_namelist_selection_changed));

    m_view_namelist.set_model(m_select_namelist);
    m_view_namelist.set_factory(namelist_factory);
    m_view_namelist.signal_activate().connect(sigc::mem_fun(*this, &Window::on_namelist_row_activated));

    m_view_namelist.set_size_request(200, -1);

    m_scroll_namelist.set_child(m_view_namelist);
    m_scroll_namelist.set_can_focus(false);
    m_scroll_namelist.set_focusable(false);
    m_scroll_namelist.set_vexpand(true);
    
    m_label_namelist.set_margin_top(5);
    m_label_namelist.set_margin_start(5);
    m_label_namelist.set_margin_end(5);
    m_search_namelist.set_margin_start(5);
    m_search_namelist.set_margin_end(5);

    m_search_namelist.set_search_delay(500);
    m_search_namelist.signal_search_changed().connect(sigc::mem_fun(*this, &Window::on_namelist_search_changed));

    m_search_namelist.set_placeholder_text("Filter...");
    m_box_namelist.set_spacing(5);
    m_box_namelist.append(m_label_namelist);
    m_box_namelist.append(m_search_namelist);
    m_box_namelist.append(m_scroll_namelist);
    m_pane_lists.set_start_child(m_box_namelist);

    auto loaded_factory = Gtk::SignalListItemFactory::create();
    loaded_factory->signal_setup().connect(sigc::mem_fun(*this, &Window::on_setup_loaded_list_item));
    loaded_factory->signal_bind().connect(sigc::mem_fun(*this, &Window::on_bind_loaded_list_item));
    loaded_factory->signal_teardown().connect(sigc::mem_fun(*this, &Window::on_teardown_loaded_list_item));

    m_loaded_root = create_loaded_list_model();
    
    m_tree_loaded = Gtk::TreeListModel::create(m_loaded_root, sigc::mem_fun(*this, &Window::create_loaded_list_model), false, false);
    m_select_loaded = Gtk::SingleSelection::create(m_tree_loaded);
    m_select_loaded->set_can_unselect();
    m_select_loaded->signal_selection_changed().connect(sigc::mem_fun(*this, &Window::on_loaded_list_selection_changed));

    m_view_loaded.set_model(m_select_loaded);
    m_view_loaded.set_factory(loaded_factory);
    m_view_loaded.signal_activate().connect(sigc::mem_fun(*this, &Window::on_loaded_list_row_activated));

    m_view_loaded.set_size_request(200, -1);

    m_scroll_loaded.set_child(m_view_loaded);
    m_scroll_loaded.set_vexpand(true);

    m_label_loaded.set_margin_top(5);
    m_label_loaded.set_margin_start(5);
    m_label_loaded.set_margin_end(5);

    m_box_loaded.append(m_label_loaded);
    m_box_loaded.append(m_scroll_loaded);
    m_box_loaded.set_spacing(5);
    m_pane_lists.set_end_child(m_box_loaded);

    m_pane_root.set_start_child(m_pane_lists);
    m_pane_root.set_end_child(m_renderer.get_area());

    //Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::on_idle_load_manager));
    load_manager = std::jthread(sigc::mem_fun(*this, &Window::on_idle_load_manager));
    m_status_bar.push("Loading Manager...");

    m_output_directory = std::make_shared<std::filesystem::path>();
}

Window::~Window() {
    m_image_queue.close();
    for(auto it = m_image_processor_pool.begin(); it != m_image_processor_pool.end(); it++) {
        it->join();
    }
}

void Window::on_manager_loaded(bool success) {
    if(success) {
        spdlog::info("Manager loaded");
        m_status_bar.push("Manager loaded");
        m_generator.total_items = 0;
        for(uint32_t i = 0; i < m_manager->pack_count(); i++) {
            m_generator.total_items += m_manager->get_pack(i).asset_count();
        }
        if(m_manager->contains("ActorSockets.xml")) {
            std::vector<uint8_t> data = m_manager->get("ActorSockets.xml")->get_data();
            ExportModelState::actorSockets = std::make_shared<utils::ActorSockets>(data);
        }

        for(uint32_t i = 0; i < 4; i++) {
            m_image_processor_pool.push_back(std::thread{
                utils::gltf::dmat::process_images, 
                std::ref(*m_manager), 
                std::ref(m_image_queue), 
                m_output_directory
            });
        }
    } else {
        m_status_bar.push("Manager failed to load");
    }
}


void Window::on_setup_namelist_item(const std::shared_ptr<Gtk::ListItem>& list_item) {
    spdlog::trace("on_setup_namelist_item");
    auto expander = Gtk::make_managed<Gtk::TreeExpander>();
    auto label = Gtk::make_managed<Gtk::Label>();
    auto icon = Gtk::make_managed<Gtk::Image>();
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
    box->append(*icon);
    box->append(*label);
    expander->set_child(*box);
    list_item->set_child(*expander);
}

void Window::on_bind_namelist_item(const std::shared_ptr<Gtk::ListItem>& list_item) {
    spdlog::trace("on_bind_namelist_item");
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row)
        return;
    auto col = std::dynamic_pointer_cast<Asset2ListItem>(row->get_item());
    list_item->set_selectable(col->m_type != AssetType::VIRTUAL);
    list_item->set_activatable(col->m_type != AssetType::VIRTUAL);
    if (!col)
        return;
    auto expander = dynamic_cast<Gtk::TreeExpander*>(list_item->get_child());
    if (!expander)
        return;
    expander->set_list_row(row);
    auto box = dynamic_cast<Gtk::Box*>(expander->get_child());
    if(!box)
        return;
    auto icon = dynamic_cast<Gtk::Image*>(box->get_first_child());
    if(!icon)
        return;
    auto label = dynamic_cast<Gtk::Label*>(icon->get_next_sibling());
    if (!label)
        return;
    if(col->m_name.size() != 0) {
        label->set_text(col->m_name);
    } else {
        label->set_text(col->m_namehash_str);
    }
    icon->set_from_icon_name(Asset2ListItem::icon_from_asset_type(col->m_type));
}

void Window::on_setup_loaded_list_item(const std::shared_ptr<Gtk::ListItem>& list_item) {
    spdlog::trace("on_setup_loaded_list_item");
    auto expander = Gtk::make_managed<Gtk::TreeExpander>();
    auto label = Gtk::make_managed<Gtk::Label>();
    auto icon = Gtk::make_managed<Gtk::Image>();
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
    auto context_menu = std::make_shared<Gtk::PopoverMenu>(m_context_menu);
    context_menu->set_has_arrow(false);
    int min, natural, min_base, natural_base;
    context_menu->get_first_child()->measure(Gtk::Orientation::HORIZONTAL, -1, min, natural, min_base, natural_base);
    context_menu->set_offset(natural >> 1, 0);
    m_menus.push_back(context_menu);
    std::weak_ptr<Gtk::PopoverMenu> context_weak_ref = context_menu;

    auto right_click = Gtk::GestureClick::create();
    right_click->set_button(3);
    right_click->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    right_click->signal_pressed().connect(
        [this, context_weak_ref](int buttons, double x, double y) {
            if(context_weak_ref.expired()) {
                spdlog::error("Context menu already destroyed");
                return;
            }
            std::shared_ptr<Gtk::PopoverMenu> menu = context_weak_ref.lock();
            this->on_loaded_list_right_click(menu, buttons, x, y);
        }
    );
    box->append(*icon);
    box->append(*label);
    context_menu->set_parent(*box);
    box->add_controller(right_click);
    expander->set_child(*box);
    list_item->set_child(*expander);
}

void Window::on_bind_loaded_list_item(const std::shared_ptr<Gtk::ListItem>& list_item) {
    spdlog::trace("on_bind_loaded_list_item");
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row)
        return;
    list_item->set_selectable(true);
    auto col = std::dynamic_pointer_cast<LoadedListItem>(row->get_item());
    if (!col)
        return;
    auto expander = dynamic_cast<Gtk::TreeExpander*>(list_item->get_child());
    if (!expander)
        return;
    expander->set_list_row(row);
    auto box = dynamic_cast<Gtk::Box*>(expander->get_child());
    if(!box)
        return;
    auto icon = dynamic_cast<Gtk::Image*>(box->get_first_child());
    if(!icon)
        return;
    auto label = dynamic_cast<Gtk::Label*>(icon->get_next_sibling());
    if (!label)
        return;
    if(col->m_name.size() != 0) {
        label->set_text(col->m_name);
    }
    icon->set_from_icon_name(Asset2ListItem::icon_from_asset_type(col->m_type));
}

void Window::on_teardown_loaded_list_item(const std::shared_ptr<Gtk::ListItem>& list_item) {
    spdlog::trace("on_teardown_loaded_list_item");
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row)
        return;
    auto expander = dynamic_cast<Gtk::TreeExpander*>(list_item->get_child());
    if (!expander)
        return;
    auto box = dynamic_cast<Gtk::Box*>(expander->get_child());
    if(!box)
        return;
    auto menu = dynamic_cast<Gtk::PopoverMenu*>(box->get_last_child());
    size_t index;
    for(index = 0; index < m_menus.size(); index++) {
        if(m_menus[index].get() == menu) {
            break;
        }
    }
    if(index < m_menus.size()) {
        m_menus.erase(m_menus.begin() + index);
    }
}

void Window::add_namelist_to_store_filtered(
    std::shared_ptr<Gio::ListStore<Asset2ListItem>> store,
    std::regex filter
) {
    for(uint32_t i = 0; i < m_namelist.size() && m_manager != nullptr; i++) {
        if(!m_manager->contains(m_namelist[i])) {
            continue;
        }

        if(!std::regex_search(m_namelist[i], filter)) {
            continue;
        }
        auto asset2 = m_manager->get_raw(m_namelist[i]);
        if(!asset2) {
            continue;
        }

        auto item = Asset2ListItem::create(
            m_namelist[i],
            Asset2ListItem::type_from_filename(m_namelist[i]),
            &(*asset2)
        );
        store->append(item);
    }
}

void handle_parameter(ParameterData* parameter, std::shared_ptr<Gio::ListStore<Asset2ListItem>> result) {
    switch(parameter->paramclass) {
    case warpgate::Parameter::D3DXParamClass::SCALAR:
        switch(parameter->paramtype) {
        case warpgate::Parameter::D3DXParamType::BOOL:
            result->append(Asset2ListItem::create(parameter->data[0] ? "true" : "false", AssetType::VALUE));
            break;
        case warpgate::Parameter::D3DXParamType::INT:
            if(parameter->data.size() < sizeof(int)) {
                break;
            }
            result->append(Asset2ListItem::create(fmt::format("{:d}", *(int*)parameter->data.data()), AssetType::VALUE));
            break;
        case warpgate::Parameter::D3DXParamType::FLOAT:
            if(parameter->data.size() < sizeof(float)) {
                break;
            }
            result->append(Asset2ListItem::create(fmt::format("{:.6f}", *(float*)parameter->data.data()), AssetType::VALUE));
            break;
        default:
            result->append(Asset2ListItem::create(fmt::format("Scalar of unhandled D3DXParamType {}", (uint32_t)parameter->paramtype), AssetType::VALUE));
            break;
        }
        break;
    case warpgate::Parameter::D3DXParamClass::VECTOR:
        switch(parameter->paramtype) {
        case warpgate::Parameter::D3DXParamType::BOOL:{
            std::span<bool> view((bool*)parameter->data.data(), parameter->data.size() / sizeof(bool));
            for(uint32_t i = 0; i < view.size(); i++) {
                result->append(Asset2ListItem::create(view[i] ? "true" : "false", AssetType::VALUE));
            }
            break;}
        case warpgate::Parameter::D3DXParamType::INT:{
            std::span<int> view((int*)parameter->data.data(), parameter->data.size() / sizeof(int));
            for(uint32_t i = 0; i < view.size(); i++) {
                result->append(Asset2ListItem::create(fmt::format("{:d}", view[i]), AssetType::VALUE));
            }
            break;}
        case warpgate::Parameter::D3DXParamType::FLOAT:{
            std::span<float> view((float*)parameter->data.data(), parameter->data.size() / sizeof(float));
            for(uint32_t i = 0; i < view.size(); i++) {
                result->append(Asset2ListItem::create(fmt::format("{:.6f}", view[i]), AssetType::VALUE));
            }
            break;}
        default:
            result->append(Asset2ListItem::create(fmt::format("Vector of unhandled D3DXParamType {}", (uint32_t)parameter->paramtype), AssetType::VALUE));
            break;
        }
        break;
    case warpgate::Parameter::D3DXParamClass::OBJECT:
        switch(parameter->paramtype) {
        case warpgate::Parameter::D3DXParamType::TEXTURE:
        case warpgate::Parameter::D3DXParamType::TEXTURE1D:
        case warpgate::Parameter::D3DXParamType::TEXTURE2D:
        case warpgate::Parameter::D3DXParamType::TEXTURE3D:
        case warpgate::Parameter::D3DXParamType::TEXTURECUBE:{
            std::optional<std::string> texture_name = parameter->material->texture(parameter->semantic);
            if(!texture_name.has_value()) {
                std::stringstream stream;
                stream << std::setw(8) << std::setfill('0') << std::uppercase << std::hex << *(uint32_t*)parameter->data.data();
                texture_name = stream.str();
            }
            result->append(Asset2ListItem::create(*texture_name, AssetType::TEXTURE));
            break;}
        default:
            result->append(Asset2ListItem::create(fmt::format("Object of unhandled D3DXParamType {}", (uint32_t)parameter->paramtype), AssetType::VALUE));
            break;
        }
        break;
    default:
        result->append(Asset2ListItem::create(fmt::format("Unhandled D3DXParamClass {} of unhandled D3DXParamType {}", (uint32_t)parameter->paramclass, (uint32_t)parameter->paramtype), AssetType::VALUE));
        break;
    }
}

std::shared_ptr<Gio::ListStore<Asset2ListItem>> Window::create_namelist_model(const std::shared_ptr<Glib::ObjectBase>& item) {
    spdlog::trace("create_namelist_model");
    auto col = std::dynamic_pointer_cast<Asset2ListItem>(item);
    
    std::shared_ptr<Gio::ListStore<Asset2ListItem>> result;
    // Root case
    if(!col){
        result = Gio::ListStore<Asset2ListItem>::create();
        add_namelist_to_store_filtered(result);
        return result;
    }
    switch(col->m_type) {
    case AssetType::ACTOR_RUNTIME: {
        std::vector<uint8_t> data = m_manager->get(col->m_stdname)->get_data();
        warpgate::utils::ADR adr(data);
        result = Gio::ListStore<Asset2ListItem>::create();
        std::optional<std::string> network_name = adr.animation_network();
        if(network_name.has_value() && m_manager->contains(*network_name)) {
            auto network = m_manager->get_raw(*network_name);
            result->append(
                Asset2ListItem::create(
                    *network_name,
                    AssetType::ANIMATION_NETWORK,
                    &(*network)
                )
            );
        }

        std::optional<std::string> animation_set = adr.animation_set();
        if(animation_set.has_value() && animation_set->size() > 0) {
            result->append(
                Asset2ListItem::create(
                    *animation_set,
                    AssetType::ANIMATION_SET
                )
            );
        }

        std::optional<std::string> base_model_name = adr.base_model();
        if(base_model_name.has_value() && m_manager->contains(*base_model_name)) {
            auto base_model = m_manager->get_raw(*base_model_name);
            result->append(
                Asset2ListItem::create(
                    *base_model_name,
                    AssetType::MODEL,
                    &(*base_model)
                )
            );
        }

        std::optional<std::string> base_palette_name = adr.base_palette();
        if(base_palette_name.has_value() && m_manager->contains(*base_palette_name)) {
            auto base_palette = m_manager->get_raw(*base_palette_name);
            result->append(
                Asset2ListItem::create(
                    *base_palette_name,
                    AssetType::PALETTE,
                    &(*base_palette)
                )
            );
        }
        break;}
    case AssetType::ANIMATION_NETWORK: {
        std::filesystem::path path(col->m_stdname);
        std::string stem = path.stem().string();
        std::string name;
        if(utils::lowercase(stem.substr(stem.size() - 3)) != "x64") {
            name = stem + "x64" + path.extension().string();
        } else {
            name = col->m_stdname;
        }
        if(!m_manager->contains(name)) {
            return result;
        }
        result = Gio::ListStore<Asset2ListItem>::create();
        auto skeletons = Asset2ListItem::create("Skeletons", AssetType::VIRTUAL);
        skeletons->virtual_parent = name;
        skeletons->virtual_type = "skeletons";
        result->append(skeletons);

        auto animations = Asset2ListItem::create("Animations", AssetType::VIRTUAL);
        animations->virtual_parent = name;
        animations->virtual_type = "animations";
        result->append(animations);
        break;}
    case AssetType::MATERIAL: {
        std::vector<uint8_t> data = m_manager->get(*col->virtual_parent)->get_data();
        warpgate::DMAT dmat(data);
        uint32_t material_definition = std::stoul(*col->virtual_type);
        std::shared_ptr<const warpgate::Material> material = nullptr;
        for(uint32_t i = 0; i < dmat.material_count(); i++) {
            if(dmat.material(i)->definition() == material_definition) {
                material = dmat.material(i);
                break;
            }
        }
        if(material == nullptr) {
            return result;
        }

        result = Gio::ListStore<Asset2ListItem>::create();
        for(uint32_t i = 0; i < material->param_count(); i++) {
            auto parameter = material->parameter(i);
            auto parameter_li = Asset2ListItem::create(
                warpgate::semantic_name(parameter.semantic_hash()),
                AssetType::PARAMETER
            );

            ParameterData* data = new ParameterData{};
            if(data == nullptr) {
                break;
            }
            data->semantic = parameter.semantic_hash();
            data->paramclass = parameter._class();
            data->paramtype = parameter.type();
            std::span<uint8_t> span = parameter.data();
            data->data = std::vector<uint8_t>(span.begin(), span.end());
            data->material = material;

            // listitem takes ownership of data
            parameter_li->user_data = (void*)data;
            result->append(parameter_li);
        }
        break;}
    case AssetType::PALETTE: {
        if(!m_manager->contains(col->m_stdname)) {
            return result;
        }
        // std::vector<uint8_t> data = m_manager->get(col->m_stdname)->get_data();
        // warpgate::DMAT dmat(data);

        result = Gio::ListStore<Asset2ListItem>::create();
        auto materials = Asset2ListItem::create("Materials", AssetType::VIRTUAL);
        materials->virtual_parent = col->m_stdname;
        materials->virtual_type = "materials";
        result->append(materials);

        auto textures = Asset2ListItem::create("Textures", AssetType::VIRTUAL);
        textures->virtual_parent = col->m_stdname;
        textures->virtual_type = "textures";
        result->append(textures);
        // std::vector<std::string> texture_names = dmat.textures();
        // for(auto it = texture_names.begin(); it != texture_names.end(); it++) {
        //     std::string value = *it;
        //     result->append(
        //         Asset2ListItem::create(
        //             value,
        //             AssetType::TEXTURE
        //         )
        //     );
        // }
        break;}
    case AssetType::PARAMETER: {
        ParameterData* parameter = (ParameterData*)col->user_data;
        if(parameter == nullptr) {
            break;
        }

        result = Gio::ListStore<Asset2ListItem>::create();
        handle_parameter(parameter, result);
        break;}
    case AssetType::VIRTUAL: {
        result = Gio::ListStore<Asset2ListItem>::create();
        std::string vtype = *col->virtual_type;
        if(vtype == "skeletons" || vtype == "animations") {
            std::string name = *col->virtual_parent;
            std::vector<uint8_t> data = m_manager->get(name)->get_data();
            warpgate::mrn::MRN mrn;
            try {
                mrn = warpgate::mrn::MRN(data, name);
            } catch(std::runtime_error e) {
                spdlog::error("While loading {}: {}", name, e.what());
                return result;
            }

            if(vtype == "animations") {
                std::vector<std::string> anim_names = mrn.file_names()->files()->filenames()->strings();
                for(auto it = anim_names.begin(); it != anim_names.end(); it++) {
                    std::string value = *it;
                    result->append(
                        Asset2ListItem::create(
                            value,
                            AssetType::ANIMATION
                        )
                    );
                }
            } else {
                std::vector<std::string> skeleton_names = mrn.skeleton_names()->skeleton_names()->strings();
                for(auto it = skeleton_names.begin(); it != skeleton_names.end(); it++) {
                    std::string value = *it;
                    result->append(
                        Asset2ListItem::create(
                            value,
                            AssetType::SKELETON
                        )
                    );
                }
            }
        } else if (vtype == "textures" || vtype == "materials") {
            std::vector<uint8_t> data = m_manager->get(*col->virtual_parent)->get_data();
            warpgate::DMAT dmat(data);

            if(vtype == "textures") {
                std::vector<std::string> texture_names = dmat.textures();
                for(auto it = texture_names.begin(); it != texture_names.end(); it++) {
                    std::string value = *it;
                    result->append(
                        Asset2ListItem::create(
                            value,
                            AssetType::TEXTURE
                        )
                    );
                }
            } else {
                for(uint32_t i = 0; i < dmat.material_count(); i++) {
                    uint32_t material_definition = dmat.material(i)->definition();
                    nlohmann::json definition = utils::materials3::materials["materialDefinitions"][std::to_string(material_definition)];
                    auto listitem = Asset2ListItem::create(
                        definition["name"],
                        AssetType::MATERIAL
                    );
                    listitem->virtual_parent = *col->virtual_parent;
                    listitem->virtual_type = std::to_string(material_definition);
                    result->append(listitem);
                }
            }
        }
        break;}
    }
    
    return result;
}

std::shared_ptr<Gio::ListStore<LoadedListItem>> Window::create_loaded_list_model(const std::shared_ptr<Glib::ObjectBase>& item) {
    auto root = std::dynamic_pointer_cast<LoadedListItem>(item);
    std::shared_ptr<Gio::ListStore<LoadedListItem>> result;

    // Create the top level model
    if(!root) {
        // Result needs to be created in every case where there is a sub-tree
        result = Gio::ListStore<LoadedListItem>::create();
        std::vector<std::string> loaded_names = m_renderer.get_model_names();
        for(uint32_t i = 0; i < loaded_names.size(); i++) {
            result->append(LoadedListItem::create(loaded_names[i], Asset2ListItem::type_from_filename(loaded_names[i])));
        }
    } else {
        // Todo - add info about the item here, like skeleton, attached animations, etc
        //        the LoadedListItem will need slots for those in its class though
    }

    return result;
}

void Window::on_namelist_selection_changed(uint32_t idx, uint32_t length) {
    auto item = m_select_namelist->get_selected_item();
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(item);
    if (!row)
        return;
    auto col = std::dynamic_pointer_cast<Asset2ListItem>(row->get_item());
    if (!col)
        return;
    spdlog::info("Selected {}", col->m_stdname.size() > 0 ? col->m_stdname : col->m_stdnamehash_str);
}

void Window::on_namelist_row_activated(uint32_t index) {
    auto item = std::dynamic_pointer_cast<Gio::ListModel>(m_view_namelist.get_model())->get_object(index);
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(item);
    if (!row)
        return;
    auto col = std::dynamic_pointer_cast<Asset2ListItem>(row->get_item());
    if (!col)
        return;
    spdlog::info("Activated {}", col->m_stdname);
    if(m_models_to_load.size() == 0) {
        Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::on_idle_load_model));
    }

    m_models_to_load.push_back(std::make_pair(col->m_stdname, col->m_type));
}

void Window::on_namelist_search_changed() {
    Glib::ustring usearch = m_search_namelist.get_text();
    std::string search = usearch.c_str();
    try {
        m_regex = std::regex(search, std::regex_constants::icase);
    } catch(std::regex_error& e) {
        spdlog::error("Failed to set regex: {}", e.what());
        return;
    }

    Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::on_idle_load_namelist));
    m_view_namelist.set_sensitive(false);
}

void Window::on_loaded_list_selection_changed(uint32_t idx, uint32_t length) {
    auto item = m_select_loaded->get_selected_item();
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(item);
    if (!row)
        return;
    auto col = std::dynamic_pointer_cast<LoadedListItem>(row->get_item());
    if (!col)
        return;
    spdlog::info("Selected Loaded Model {}", col->m_stdname);
}

void Window::on_loaded_list_row_activated(uint32_t index) {
    auto item = std::dynamic_pointer_cast<Gio::ListModel>(m_view_loaded.get_model())->get_object(index);
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(item);
    if (!row)
        return;
    auto col = std::dynamic_pointer_cast<LoadedListItem>(row->get_item());
    if (!col)
        return;
    spdlog::info("Activated Loaded Model {}", col->m_stdname);
}

void Window::on_loaded_list_right_click(std::shared_ptr<Gtk::PopoverMenu> menu, int n_buttons, double x, double y) {
    Gdk::Rectangle rect{(int)x, (int)y, 1, 1};
    menu->set_pointing_to(rect);
    menu->popup();

    auto box = menu->get_parent();
    auto expander = box->get_parent();
    auto listitem = expander->get_parent();
    auto listview = listitem->get_parent();
    auto curritem = listview->get_first_child();
    uint32_t index = 0;
    while(curritem && curritem != listitem) {
        curritem = curritem->get_next_sibling();
        index++;
    }
    if(curritem) {
        m_select_loaded->set_selected(index);
    }
}

void Window::on_load_namelist() {
    spdlog::debug("Called load namelist");
    m_namelist_dialog->set_accept_label("Import");
    m_namelist_dialog->set_title("Import Namelist");
    m_namelist_dialog->open(*this, sigc::mem_fun(*this, &Window::on_load_namelist_response));
}

void Window::on_load_namelist_response(std::shared_ptr<Gio::AsyncResult> &result) {
    std::shared_ptr<Gio::File> file;
    try {
        file = m_namelist_dialog->open_finish(result);
    } catch (Gtk::DialogError &e) {
        spdlog::error("Error opening namelist: {}", e.what());
        return;
    }
    std::string data;
    try {
        auto inputstream = file->read(nullptr);
        inputstream->seek(0, Glib::SeekType::END);
        size_t size = inputstream->tell(), read;
        data.resize(size);
        inputstream->seek(0, Glib::SeekType::SET);
        inputstream->read_all(data.data(), size, read);
        inputstream->close();
    } catch(Gio::Error &e) {
        spdlog::error("Reading file: {}", e.what());
        return;
    }
    std::istringstream datastream(data);
    data.clear();
    m_namelist.clear();

    std::string line;
    while(std::getline(datastream, line)) {
        size_t index = line.find_first_of("\n\r");
        if(index != std::string::npos) {
            line = line.substr(0, index);
        }
        m_namelist.push_back(line);
    }

    Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::on_idle_load_namelist));
    m_view_namelist.set_sensitive(false);
}

bool Window::on_idle_load_manager() {
    std::filesystem::path server("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/");
    std::vector<std::filesystem::path> assets;
    for(int i = 0; i < 24; i++) {
        assets.push_back(server / ("assets_x64_" + std::to_string(i) + ".pack2"));
    }
    assets.push_back(server / "data_x64_0.pack2");
    try {
        m_manager = std::make_shared<synthium::Manager>(assets);
        on_manager_loaded(true);
    } catch(std::exception &e) {
        spdlog::error("Failed to load manager: {}", e.what());
        on_manager_loaded(false);
    }
    return false;
}

bool Window::on_idle_load_namelist() {
    m_namelist_root->remove_all();
    add_namelist_to_store_filtered(m_namelist_root, m_regex);
    m_view_namelist.set_sensitive(true);
    return false;
}

bool Window::on_idle_load_model() {
    if(m_manager == nullptr) {
        return true;
    }
    auto[model, type] = m_models_to_load.back();
    m_models_to_load.pop_back();

    m_renderer.load_model(model, type, m_manager);

    m_loaded_root->append(LoadedListItem::create(model, type));
    return m_models_to_load.size() > 0;
}

bool Window::on_idle_generate_namelist() {
    if(m_generator.pack_index >= m_manager->pack_count()) {
        m_generator.property_finished = true;
        std::shared_ptr<Gio::FileOutputStream> stream;
        if(m_generator.file->query_exists()) {
            stream = m_generator.file->replace();
        } else {
            stream = m_generator.file->append_to();
        }
        for(auto it = m_generator.names.begin(); it != m_generator.names.end(); it++) {
            stream->write(it->data(), it->size());
            stream->write("\n", 1);
        }
        stream->close();
        m_generator.names.clear();
        m_generator.file = nullptr;
        m_status_bar.push("Namelist generation complete!");
        return false;
    }
    synthium::Pack2 &pack = m_manager->get_pack(m_generator.pack_index);
    if(m_generator.asset_index >= pack.asset_count()) {
        m_generator.pack_index++;
        m_generator.asset_index = 0;
        return true;
    }
    std::shared_ptr<synthium::Asset2> asset = pack.asset(m_generator.asset_index, false);
    m_generator.asset_index++;
    std::vector<uint8_t> data = asset->get_data();
    if(data.size() == 0 || strncmp("<ActorRuntime>", (const char*)data.data(), 14) != 0) {
        m_generator.items_completed++;
        m_progress_bar.set_fraction((double)m_generator.items_completed / (double)m_generator.total_items);
        return true;
    }

    utils::ADR adr(data);
    auto dme = adr.base_model();
    if(dme) {
        std::filesystem::path dme_path = *dme;
        std::string adr_name = dme_path.stem().string();
        if(adr_name.find("_LOD0") != std::string::npos) {
            adr_name = adr_name.substr(0, adr_name.find("_LOD0"));
        } else if(adr_name.find("_Lod0") != std::string::npos) {
            adr_name = adr_name.substr(0, adr_name.find("_Lod0"));
        } else if(adr_name.find("_lod0") != std::string::npos) {
            adr_name = adr_name.substr(0, adr_name.find("_lod0"));
        }
        adr_name += ".adr";
        m_generator.names.insert(adr_name);
    }

    auto dma = adr.base_palette();
    if(dma) {
        std::filesystem::path dma_path = *dma;
        std::string adr_name = dma_path.stem().string();
        if(adr_name.find("_LOD0") != std::string::npos) {
            adr_name = adr_name.substr(0, adr_name.find("_LOD0"));
        } else if(adr_name.find("_Lod0") != std::string::npos) {
            adr_name = adr_name.substr(0, adr_name.find("_Lod0"));
        } else if(adr_name.find("_lod0") != std::string::npos) {
            adr_name = adr_name.substr(0, adr_name.find("_lod0"));
        }
        adr_name += ".adr";
        m_generator.names.insert(adr_name);
    }

    auto mrn = adr.animation_network();
    if(mrn) {
        std::filesystem::path mrn_path = *mrn;
        std::string mrn_name = mrn_path.stem().string();
        if(mrn_name.find("X64") == std::string::npos && mrn_name.find("x64") == std::string::npos) {
            mrn_name += "X64";
        }
        mrn_name += ".mrn";
        m_generator.names.insert(mrn_name);
    }

    m_generator.items_completed++;
    m_progress_bar.set_fraction((double)m_generator.items_completed / (double)m_generator.total_items);
    return true;
}

bool Window::on_idle_export_model() {
    try {
        std::filesystem::create_directories(*m_output_directory / "textures");
    } catch(std::filesystem::filesystem_error& err) {
        spdlog::error("Export failed!");
        spdlog::error("Failed to create directory {}: {}", err.path1().string(), err.what());
        m_exporter.property_finished = true;
        return false;
    }
    std::shared_ptr<LoadedListItem> item = m_exporter.property_export_item.get_value();
    std::shared_ptr<synthium::Asset2> asset = m_manager->get(item->m_stdname);
    std::vector<uint8_t> adr_data, dmat_data, dme_data;
    std::optional<std::string> dme_name, dmat_name;
    std::shared_ptr<DME> dme;
    std::shared_ptr<DMAT> dmat;
    std::shared_ptr<utils::ADR> adr;
    switch(item->m_type) {
    case AssetType::ACTOR_RUNTIME:
        adr_data = asset->get_data();
        adr = std::make_shared<utils::ADR>(adr_data);
        dme_name = adr->base_model();
        if(!dme_name) {
            spdlog::error("Export failed!");
            spdlog::error("ActorRuntime '{}' did not have a base model to export!", item->m_stdname);
            m_exporter.property_finished = true;
            return false;
        }
        dmat_name = adr->base_palette();
        if(dmat_name) {
            dmat_data = m_manager->get(*dmat_name)->get_data();
            dmat = std::make_shared<DMAT>(dmat_data);
        }

        dme_data = m_manager->get(*dme_name)->get_data();
        if(dmat) {
            dme = std::make_shared<DME>(dme_data, m_exporter.property_path.get_value().stem().string(), dmat);
        } else {
            dme = std::make_shared<DME>(dme_data, m_exporter.property_path.get_value().stem().string());
        }
        break;
    case AssetType::MODEL:
        dme_data = asset->get_data();
        dme = std::make_shared<DME>(dme_data, m_exporter.property_path.get_value().stem().string());
        break;
    default:
        spdlog::error("Export failed!");
        spdlog::error("Cannot export item '{}'", item->m_stdname);
        m_exporter.property_finished = true;
        return false;
    }
    
    int parent_index;
    tinygltf::Model gltf = utils::gltf::dme::build_gltf_from_dme(*dme, m_image_queue, *m_output_directory, true, true, false, &parent_index);
    std::string basename = std::filesystem::path(item->m_stdname).stem().string();
    size_t lod_index = utils::lowercase(basename).find("_lod0");
    if(lod_index != std::string::npos) {
        spdlog::info("Removing _lod0 from '{}'", basename);
        basename = basename.substr(0, lod_index);
        spdlog::info("Removed _lod0 from '{}'", basename);
    }
    //asset = m_manager->get("ActorSockets.xml")
    if(ExportModelState::actorSockets->model_indices.find(basename) != ExportModelState::actorSockets->model_indices.end()) {
        int cog_index = utils::gltf::findCOGIndex(gltf, gltf.nodes[parent_index]);
        if(cog_index != -1) {
            parent_index = cog_index;
        }
        utils::gltf::dme::add_actorsockets_to_gltf(gltf, *ExportModelState::actorSockets, basename, parent_index);
    } else {
        spdlog::warn("'{}' not found in actorSockets!", basename);
    }

    uint8_t* buffer = nullptr;
    size_t size = 0;
    tinygltf::TinyGLTF writer;
    std::string format = m_exporter.property_path.get_value().extension().string();
    if(format != ".glb" && format != ".gltf") {
        spdlog::warn("Extension was not gltf or glb! Defaulting to gltf");
        format = ".gltf";
    }
    writer.SerializeGltfSceneToBuffer(&gltf, &buffer, &size, m_exporter.property_path.get_value().string(), false, format == ".glb", format == ".gltf", format == ".glb");
    if(size == 0 || buffer == nullptr) {
        spdlog::error("Failed to serialize glTF file!");
        m_exporter.property_finished = true;
        return false;
    }
    std::shared_ptr<Gio::FileOutputStream> stream;
    if(m_exporter.file->query_exists()) {
        stream = m_exporter.file->replace();
    } else {
        stream = m_exporter.file->append_to();
    }
    GError *c_error = nullptr;
    bool success = g_output_stream_write_all(G_OUTPUT_STREAM(stream->gobj()), buffer, size, nullptr, nullptr, &c_error);
    if(!success) {
        Glib::Error error = Glib::Error(c_error);
        spdlog::error("Error writing to output stream: {}", error.what());
    }
    stream->close();
    delete[] buffer;
    m_exporter.property_finished = true;
    return false;
}

void Window::on_gen_namelist() {
    spdlog::info("Called generate namelist");
    if(m_generator.property_finished) {
        m_generator.property_finished = false;
        m_generator.pack_index = 0;
        m_generator.asset_index = 0;
        m_generator.items_completed = 0;
        m_progress_bar.set_fraction((double)m_generator.items_completed / (double)m_generator.total_items);

        m_namelist_dialog->set_accept_label("Generate");
        m_namelist_dialog->set_title("Generate Namelist");
        m_namelist_dialog->save(*this, sigc::mem_fun(*this, &Window::on_gen_namelist_response));
    }
}

void Window::on_gen_namelist_response(std::shared_ptr<Gio::AsyncResult> &result) {
    try {
        m_generator.file = m_namelist_dialog->save_finish(result);
    } catch(Gtk::DialogError &e) {
        spdlog::error("Failed saving namelist: {}", e.what());
        return;
    }
    if(m_generator.file == nullptr) {
        return;
    }
    m_status_bar.push("Generating namelist...");
    Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::on_idle_generate_namelist));
}

void Window::on_export() {
    spdlog::info("Called export");
}

void Window::on_quit() {
    spdlog::info("Called quit");
    set_visible(false);
}

void Window::on_export_loaded() {
    spdlog::info("Called export loaded");
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(m_select_loaded->get_selected_item());
    if(!row) {
        return;
    }
    auto model = std::dynamic_pointer_cast<LoadedListItem>(row->get_item());
    if(!model) {
        return;
    }

    m_exporter.property_export_item = model;
    m_model_dialog->set_title("Export Model...");
    m_model_dialog->set_accept_label("Export");
    std::filesystem::path p(model->m_stdname);
    gtk_file_dialog_set_initial_name(m_model_dialog->gobj(), (p.stem().string() + ".gltf").c_str());
    m_model_dialog->save(*this, sigc::mem_fun(*this, &Window::on_export_loaded_response));
}

void Window::on_export_loaded_response(std::shared_ptr<Gio::AsyncResult> &result) {
    try {
        m_exporter.file = m_model_dialog->save_finish(result);
    } catch(Gtk::DialogError &e) {
        switch(e.code()) {
        case Gtk::DialogError::FAILED:
            spdlog::error("Choosing export file: {}", e.what());
            break;
        case Gtk::DialogError::CANCELLED:
            spdlog::debug("Operation cancelled by user");
            break;
        case Gtk::DialogError::DISMISSED:
            spdlog::debug("Dialog dismissed by user");
            break;
        default:
            spdlog::error("Something weird happened: code {} - {}", e.code(), e.what());
            break;
        }

        return;
    }

    // FFS gtkmm... you return a corrupted std::string in the c++ api when its /this easy/...
    char* c_path = g_file_get_path(m_exporter.file->gobj());
    std::string path;
    if(c_path != nullptr) {
        path = c_path;
        m_exporter.property_path = path;
        *m_output_directory = m_exporter.property_path.get_value().parent_path();
    }

    m_exporter.property_finished = false;
    Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::on_idle_export_model));
}

void Window::on_remove_loaded() {
    spdlog::info("Called remove loaded");
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(m_select_loaded->get_selected_item());
    if(!row) {
        return;
    }
    auto model = std::dynamic_pointer_cast<LoadedListItem>(row->get_item());
    if(!model) {
        return;
    }
    spdlog::info("Gonna destroy model '{}'", model->m_stdname);
    m_renderer.destroy_model(model->m_stdname);
    uint32_t index;
    for(index = 0; index < m_loaded_root->get_n_items(); index++) {
        if(m_loaded_root->get_item(index) == model) {
            break;
        }
    }
    m_loaded_root->remove(index);
}