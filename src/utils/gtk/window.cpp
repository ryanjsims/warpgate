#include "utils/gtk/window.hpp"
#include <thread>
#include <regex>

#include <gtkmm/treelistmodel.h>
#include <gtkmm/treeexpander.h>
#include <gtkmm/label.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/dialog.h>
#include <gtkmm/gestureclick.h>
#include <giomm/simpleactiongroup.h>
#include <giomm/datainputstream.h>

#include <dme_loader.h>
#include <mrn_loader.h>
#include "utils/adr.h"

using namespace warpgate::gtk;

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
            return "media-seek-forward-symbolic";
        case AssetType::ANIMATION_NETWORK:
            return "network-wired-symbolic";
        case AssetType::MODEL:
            return "application-x-appliance-symbolic";
        case AssetType::PALETTE:
            return "view-paged-symbolic";
        case AssetType::ANIMATION_SET:
        case AssetType::SKELETON:
            return "emblem-shared-symbolic";
        case AssetType::TEXTURE:
            return "image-x-generic-symbolic";
        case AssetType::VIRTUAL:
            return "folder-symbolic";
        case AssetType::UNKNOWN:
        default:
            return "emblem-important-symbolic";
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

Window::Window() : m_manager(nullptr), m_file_dialog(*this, "Import Namelist", Gtk::FileChooser::Action::OPEN) {
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
    action_group->add_action("gen_namelist", sigc::mem_fun(*this, &Window::on_gen_namelist));
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

    m_file_dialog.add_button("Import", Gtk::ResponseType::ACCEPT)->signal_clicked().connect([&]{
        m_file_dialog.hide();
    });
    m_file_dialog.add_button("Cancel", Gtk::ResponseType::CANCEL)->signal_clicked().connect([&]{
        m_file_dialog.hide();
    });
    m_file_dialog.signal_response().connect(sigc::mem_fun(*this, &Window::on_file_dialog_signal_response));
    m_file_dialog.signal_close_request().connect([&]{
        m_file_dialog.hide();
        return true;
    }, false);

    m_menubar.set_menu_model(win_menu);
    m_menubar.set_can_focus(false);
    m_menubar.set_focusable(false);

    m_box_root.append(m_menubar);
    m_box_root.append(m_pane_root);
    m_box_root.append(m_status_bar);

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

    // auto right_click = Gtk::GestureClick::create();
    // right_click->set_button(3);
    // right_click->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    // right_click->signal_pressed().connect(sigc::mem_fun(*this, &Window::on_loaded_list_right_click));
    //m_box_loaded.add_controller(right_click);

    m_box_loaded.append(m_label_loaded);
    m_box_loaded.append(m_scroll_loaded);
    m_box_loaded.set_spacing(5);
    m_pane_lists.set_end_child(m_box_loaded);

    m_pane_root.set_start_child(m_pane_lists);
    m_pane_root.set_end_child(m_renderer.get_area());

    //Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::on_idle_load_manager));
    load_manager = std::jthread(sigc::mem_fun(*this, &Window::on_idle_load_manager));
    m_status_bar.push("Loading Manager...");
}

Window::~Window() {

}

void Window::on_manager_loaded(bool success) {
    if(success) {
        spdlog::info("Manager loaded");
        m_status_bar.push("Manager loaded");
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
    case AssetType::PALETTE: {
        if(!m_manager->contains(col->m_stdname)) {
            return result;
        }
        std::vector<uint8_t> data = m_manager->get(col->m_stdname)->get_data();
        warpgate::DMAT dmat(data);

        result = Gio::ListStore<Asset2ListItem>::create();
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
    // if(m_models_to_load.size() == 0) {
    //     Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::on_idle_load_model));
    // }

    // m_models_to_load.push_back(std::make_pair(col->m_stdname, col->m_type));
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
    spdlog::info("Called load namelist");
    m_file_dialog.show();
}

void Window::on_file_dialog_signal_response(int response) {
    spdlog::info("Got response {}", response);
    if(response != Gtk::ResponseType::ACCEPT) {
        return;
    }
    auto file_chooser = (Gtk::FileChooser*)(&m_file_dialog);
    auto file = file_chooser->get_file();
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

void Window::on_gen_namelist() {
    spdlog::info("Called generate namelist");
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