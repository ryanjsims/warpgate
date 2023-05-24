#pragma once
#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/treelistmodel.h>
#include <gtkmm/listview.h>
#include <gtkmm/listitem.h>
#include <gtkmm/popovermenubar.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/label.h>
#include <glibmm/main.h>

#include <sigc++/sigc++.h>
#include <synthium/manager.h>

#include "utils/gtk/model_renderer.hpp"
#include "utils/gtk/asset_type.hpp"

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
        Gtk::Box m_box_root {Gtk::Orientation::VERTICAL, 0}
               , m_box_namelist {Gtk::Orientation::VERTICAL, 0}
               , m_box_modellist {Gtk::Orientation::VERTICAL, 0};
        Gtk::Paned m_pane_root, m_pane_lists;
        Gtk::ListView m_view_namelist;
        Gtk::ScrolledWindow m_scroll_namelist;
        std::shared_ptr<Gtk::SingleSelection> m_select_namelist, m_select_loaded;
        Gtk::Statusbar m_status_bar;
        Gtk::FileChooserDialog m_file_dialog;
        Gtk::Label m_label_namelist{"Namelist"}, m_label_modellist{"Models"};
        Gtk::SearchEntry m_search_namelist;

        ModelRenderer m_renderer;

        std::shared_ptr<Gtk::TreeListModel> m_tree_namelist;
        std::vector<std::pair<std::string, AssetType>> m_models_to_load;

        void on_setup_namelist_item(const std::shared_ptr<Gtk::ListItem>& list_item);
        void on_bind_namelist_item(const std::shared_ptr<Gtk::ListItem>& list_item);

        void on_setup_models_item(const std::shared_ptr<Gtk::ListItem>& list_item);
        void on_bind_models_item(const std::shared_ptr<Gtk::ListItem>& list_item);

        Glib::RefPtr<Gio::ListModel> create_namelist_model(const std::shared_ptr<Glib::ObjectBase>& item = {});

        void on_namelist_selection_changed(uint32_t idx, uint32_t length);
        void on_namelist_row_activated(uint32_t index);

        void on_load_namelist();
        void on_gen_namelist();
        void on_export();
        void on_quit();

        void on_file_dialog_signal_response(int response);
        bool on_idle_load_manager();
        bool on_idle_load_namelist();
        bool on_idle_load_model();
    };
};