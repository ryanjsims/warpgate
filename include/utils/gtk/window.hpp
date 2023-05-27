#pragma once
#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/paned.h>
#include <gtkmm/expression.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/treelistmodel.h>
#include <gtkmm/filterlistmodel.h>
#include <gtkmm/filter.h>
#include <gtkmm/stringfilter.h>
#include <gtkmm/listview.h>
#include <gtkmm/listitem.h>
#include <gtkmm/popovermenubar.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/searchbar.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/label.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/iconpaintable.h>

#include <glibmm/main.h>

#include <giomm/liststore.h>
#include <giomm/menu.h>

#include <sigc++/sigc++.h>
#include <synthium/manager.h>

#include "utils/gtk/model_renderer.hpp"
#include "utils/gtk/asset_type.hpp"

#include <regex>

class Asset2ListItem;
class LoadedListItem;
class NamelistFilter;

namespace warpgate::gtk {
    class Window : public Gtk::Window {
    public:
        Window();
        ~Window() override;

        void on_manager_loaded(bool success);
    protected:
        std::shared_ptr<synthium::Manager> m_manager;
        std::vector<std::string> m_namelist;
        bool namelist_dirty = false;
        std::jthread load_manager;

        Gtk::PopoverMenuBar m_menubar;
        std::shared_ptr<Gio::Menu> m_context_menu;
        std::vector<std::shared_ptr<Gtk::PopoverMenu>> m_menus;
        Gtk::Box m_box_root {Gtk::Orientation::VERTICAL, 0}
               , m_box_namelist {Gtk::Orientation::VERTICAL, 0}
               , m_box_loaded {Gtk::Orientation::VERTICAL, 0};
        Gtk::Paned m_pane_root, m_pane_lists;

        Gtk::ListView m_view_namelist, m_view_loaded;
        Gtk::ScrolledWindow m_scroll_namelist, m_scroll_loaded;
        std::shared_ptr<Gtk::SingleSelection> m_select_namelist, m_select_loaded;
        Gtk::Label m_label_namelist{"Namelist"}, m_label_loaded{"Models"};
        std::shared_ptr<Gtk::TreeListModel> m_tree_namelist, m_tree_loaded;
        //std::shared_ptr<Gtk::FilterListModel> m_filtered_namelist;
        //std::shared_ptr<Gtk::StringFilter> m_filter;
        std::regex m_regex;

        std::shared_ptr<Gio::ListStore<Asset2ListItem>> m_namelist_root;
        std::shared_ptr<Gio::ListStore<LoadedListItem>> m_loaded_root;

        Gtk::Statusbar m_status_bar;
        Gtk::FileChooserDialog m_file_dialog;
        Gtk::SearchEntry m_search_namelist;
        Gtk::Box m_search_box {Gtk::Orientation::HORIZONTAL, 0};

        ModelRenderer m_renderer;

        std::vector<std::pair<std::string, AssetType>> m_models_to_load;

        void on_setup_namelist_item(const std::shared_ptr<Gtk::ListItem>& list_item);
        void on_bind_namelist_item(const std::shared_ptr<Gtk::ListItem>& list_item);

        void on_setup_loaded_list_item(const std::shared_ptr<Gtk::ListItem>& list_item);
        void on_bind_loaded_list_item(const std::shared_ptr<Gtk::ListItem>& list_item);
        void on_teardown_loaded_list_item(const std::shared_ptr<Gtk::ListItem>& list_item);

        void add_namelist_to_store_filtered(
            std::shared_ptr<Gio::ListStore<Asset2ListItem>> store,
            std::regex filter = std::regex{""}
        );

        Glib::RefPtr<Gio::ListStore<Asset2ListItem>> create_namelist_model(const std::shared_ptr<Glib::ObjectBase>& item = {});
        Glib::RefPtr<Gio::ListStore<LoadedListItem>> create_loaded_list_model(const std::shared_ptr<Glib::ObjectBase>& item = {});

        void on_namelist_selection_changed(uint32_t idx, uint32_t length);
        void on_namelist_row_activated(uint32_t index);

        void on_namelist_search_changed();

        void on_loaded_list_selection_changed(uint32_t idx, uint32_t length);
        void on_loaded_list_row_activated(uint32_t index);
        void on_loaded_list_right_click(std::shared_ptr<Gtk::PopoverMenu> menu, int n_buttons, double x, double y);

        void on_load_namelist();
        void on_gen_namelist();
        void on_export();
        void on_quit();

        void on_export_loaded();
        void on_remove_loaded();

        void on_file_dialog_signal_response(int response);
        bool on_idle_load_manager();
        bool on_idle_load_namelist();
        bool on_idle_load_model();
    };
};