#include "utils/gtk/window.hpp"
#include <thread>

#include <gtkmm/treelistmodel.h>
#include <gtkmm/treeexpander.h>
#include <gtkmm/label.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/dialog.h>
#include <giomm/liststore.h>
#include <giomm/menu.h>
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

Window::Window() : m_manager(nullptr), m_file_dialog(*this, "Import Namelist", Gtk::FileChooser::Action::OPEN) {
    set_title("Warpgate");
    set_default_size(1280, 960);
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
    // = Gtk::FileChooserDialog("Import Namelist", Gtk::FileChooser::Action::OPEN);
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
    
    auto factory = Gtk::SignalListItemFactory::create();
    factory->signal_setup().connect(sigc::mem_fun(*this, &Window::on_setup_namelist_item));
    factory->signal_bind().connect(sigc::mem_fun(*this, &Window::on_bind_namelist_item));

    auto root = create_namelist_model();

    m_tree_namelist = Gtk::TreeListModel::create(root, sigc::mem_fun(*this, &Window::create_namelist_model), false, false);
    m_select_namelist = Gtk::SingleSelection::create(m_tree_namelist);
    m_select_namelist->signal_selection_changed().connect(sigc::mem_fun(*this, &Window::on_namelist_selection_changed));

    m_view_namelist.set_model(m_select_namelist);
    m_view_namelist.set_factory(factory);
    m_view_namelist.signal_activate().connect(sigc::mem_fun(*this, &Window::on_namelist_row_activated));

    m_view_namelist.set_size_request(200, -1);

    m_scroll_namelist.set_child(m_view_namelist);
    m_scroll_namelist.set_can_focus(false);
    m_scroll_namelist.set_focusable(false);
    m_scroll_namelist.set_vexpand(true);
    
    m_label_namelist.set_margin(5);

    m_search_namelist.set_placeholder_text("Filter...");
    m_box_namelist.append(m_label_namelist);
    m_box_namelist.append(m_search_namelist);
    m_box_namelist.append(m_scroll_namelist);
    m_pane_lists.set_start_child(m_box_namelist);

    m_label_modellist.set_margin(5);

    m_box_modellist.append(m_label_modellist);
    m_pane_lists.set_end_child(m_box_modellist);

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
    expander->set_child(*label);
    list_item->set_child(*expander);
}

void Window::on_bind_namelist_item(const std::shared_ptr<Gtk::ListItem>& list_item) {
    spdlog::trace("on_bind_namelist_item");
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row)
        return;
    list_item->set_selectable(true);
    auto col = std::dynamic_pointer_cast<Asset2ListItem>(row->get_item());
    if (!col)
        return;
    auto expander = dynamic_cast<Gtk::TreeExpander*>(list_item->get_child());
    if (!expander)
        return;
    expander->set_list_row(row);
    auto label = dynamic_cast<Gtk::Label*>(expander->get_child());
    if (!label)
        return;
    if(col->m_name.size() != 0) {
        label->set_text(col->m_name);
    } else {
        label->set_text(col->m_namehash_str);
    }
}

void Window::on_setup_models_item(const std::shared_ptr<Gtk::ListItem>& list_item) {
    spdlog::trace("on_setup_models_item");
    auto label = Gtk::make_managed<Gtk::Label>();
    list_item->set_child(*label);
}

void Window::on_bind_models_item(const std::shared_ptr<Gtk::ListItem>& list_item) {
    spdlog::trace("on_bind_models_item");
    //auto row = 
}

std::shared_ptr<Gio::ListModel> Window::create_namelist_model(const std::shared_ptr<Glib::ObjectBase>& item) {
    spdlog::trace("Create model");
    auto col = std::dynamic_pointer_cast<Asset2ListItem>(item);
    
    std::shared_ptr<Gio::ListStore<Asset2ListItem>> result;
    // Root case
    if(!col){
        result = Gio::ListStore<Asset2ListItem>::create();
        for(uint32_t i = 0; i < m_namelist.size() && m_manager != nullptr; i++) {
            if(!m_manager->contains(m_namelist[i])) {
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
            result->append(item);
        }
    } else if(col && col->m_type == AssetType::ACTOR_RUNTIME) {
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
    } else if(col && col->m_type == AssetType::ANIMATION_NETWORK) {
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
        std::vector<uint8_t> data = m_manager->get(name)->get_data();
        warpgate::mrn::MRN mrn;
        try {
            mrn = warpgate::mrn::MRN(data, name);
        } catch(std::runtime_error e) {
            spdlog::error("While loading {}: {}", name, e.what());
            return result;
        }

        result = Gio::ListStore<Asset2ListItem>::create();
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
    } else if(col && col->m_type == AssetType::PALETTE) {
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
    m_tree_namelist = Gtk::TreeListModel::create(create_namelist_model(), sigc::mem_fun(*this, &Window::create_namelist_model), false, false);
    m_select_namelist->set_model(m_tree_namelist);
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