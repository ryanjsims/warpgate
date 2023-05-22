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
        Gtk::Box m_box {Gtk::Orientation::HORIZONTAL, 0}, m_vbox {Gtk::Orientation::VERTICAL, 0};
        Gtk::Paned m_pane;
        ModelRenderer m_renderer;
        Gtk::ListView m_files_view;
        Gtk::ScrolledWindow m_files_pane;
        std::shared_ptr<Gtk::SingleSelection> m_singleselection;
        Gtk::FileChooserDialog m_file_dialog;

        std::shared_ptr<Gtk::TreeListModel> m_files_tree;
        std::vector<std::pair<std::string, AssetType>> m_models_to_load;

        void on_setup_listitem(const std::shared_ptr<Gtk::ListItem>& list_item);
        void on_bind_listitem(const std::shared_ptr<Gtk::ListItem>& list_item);

        Glib::RefPtr<Gio::ListModel> create_model(const std::shared_ptr<Glib::ObjectBase>& item = {});
        void on_selection_changed(uint32_t idx, uint32_t length);

        void on_listview_row_activated(uint32_t index);

        void on_load_namelist();
        void on_gen_namelist();
        void on_export();
        void on_quit();

        void on_file_dialog_signal_response(int response);
        bool on_idle_load_namelist();
        bool on_idle_load_model();
    };
};