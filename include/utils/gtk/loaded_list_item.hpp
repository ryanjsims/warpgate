#pragma once
#include "utils/gtk/asset_type.hpp"

#include <glibmm/object.h>
#include <glibmm/ustring.h>

#include <vector>
#include <unordered_map>

class LoadedListItem : public Glib::Object {
public:
    Glib::ustring m_name;
    std::string m_stdname;
    warpgate::gtk::AssetType m_type;

    using stack_t = std::vector<std::string>;

    static std::shared_ptr<LoadedListItem> create(std::string name, warpgate::gtk::AssetType type = warpgate::gtk::AssetType::UNKNOWN);

    stack_t find_child(std::shared_ptr<LoadedListItem> search);
    stack_t find_child(std::string name);

    bool add_child(std::shared_ptr<LoadedListItem> child);
    bool remove_child(std::string name);
    void set_parent(LoadedListItem* parent);

    std::shared_ptr<LoadedListItem> get_child(stack_t path);
    std::shared_ptr<LoadedListItem> get_child(std::string name);

    std::vector<std::shared_ptr<LoadedListItem>> children() const;
    LoadedListItem* parent() const;

protected:
    LoadedListItem* m_parent;
    std::vector<std::shared_ptr<LoadedListItem>> m_children;
    std::unordered_map<std::string, std::shared_ptr<LoadedListItem>> m_children_by_name;

    LoadedListItem(std::string name, warpgate::gtk::AssetType type);
    ~LoadedListItem() override;
};