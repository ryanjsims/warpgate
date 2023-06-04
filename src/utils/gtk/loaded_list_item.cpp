#include "utils/gtk/loaded_list_item.hpp"

#include <spdlog/spdlog.h>

using namespace warpgate::gtk;

std::shared_ptr<LoadedListItem> LoadedListItem::create(std::string name, AssetType type) {
    return Glib::make_refptr_for_instance<LoadedListItem>(new LoadedListItem(name, type));
}

LoadedListItem* LoadedListItem::parent() const {
    return m_parent;
}

LoadedListItem::stack_t LoadedListItem::find_child(std::shared_ptr<LoadedListItem> search) {
    LoadedListItem::stack_t to_return;
    if(search.get() == this) {
        to_return.push_back(m_stdname);
    } else {
        for(auto it = m_children_by_name.begin(); it != m_children_by_name.end() && to_return.size() == 0; it++) {
            to_return = it->second->find_child(search);
        }
        if(to_return.size() > 0) {
            to_return.push_back(m_stdname);
        }
    }
    return to_return;
}

LoadedListItem::stack_t LoadedListItem::find_child(std::string name) {
    LoadedListItem::stack_t to_return;
    if(name == m_stdname) {
        to_return.push_back(m_stdname);
    } else {
        for(auto it = m_children_by_name.begin(); it != m_children_by_name.end() && to_return.size() == 0; it++) {
            to_return = it->second->find_child(name);
        }
        if(to_return.size() > 0) {
            to_return.push_back(m_stdname);
        }
    }
    return to_return;
}

bool LoadedListItem::add_child(std::shared_ptr<LoadedListItem> child) {
    if(m_children_by_name.find(child->m_stdname) != m_children_by_name.end()) {
        spdlog::warn("Child '{}' already exists in this tree", child->m_stdname);
        return false;
    }
    m_children.push_back(child);
    m_children_by_name[child->m_stdname] = child;
    if(child->m_parent != nullptr) {
        child->m_parent->remove_child(child->m_stdname);
    }
    child->set_parent(this);
    return true;
}

bool LoadedListItem::remove_child(std::string name) {
    auto map_iter = m_children_by_name.find(name);
    if(map_iter == m_children_by_name.end()) {
        spdlog::warn("Child '{}' does not exist in this tree", name);
        return false;
    }
    auto vec_iter = m_children.begin();
    for(vec_iter; vec_iter != m_children.end(); vec_iter++) {
        if(*vec_iter == map_iter->second) {
            break;
        }
    }
    m_children.erase(vec_iter);
    m_children_by_name.erase(map_iter);
    return true;
}

void LoadedListItem::set_parent(LoadedListItem* parent) {
    m_parent = parent;
}

std::shared_ptr<LoadedListItem> LoadedListItem::get_child(LoadedListItem::stack_t path) {
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

std::shared_ptr<LoadedListItem> LoadedListItem::get_child(std::string name) {
    std::shared_ptr<LoadedListItem> to_return = nullptr;
    auto iter = m_children_by_name.find(name);
    if(iter != m_children_by_name.end()) {
        return iter->second;
    } else {
        for(auto it = m_children_by_name.begin(); it != m_children_by_name.end() && to_return == nullptr; it++) {
            to_return = it->second->get_child(name);
        }
    }
    return to_return;
}

std::vector<std::shared_ptr<LoadedListItem>> LoadedListItem::children() const {
    return m_children;
}

LoadedListItem::LoadedListItem(std::string name, AssetType type)
    : Glib::ObjectBase("LoadedListItem")
    , m_parent(nullptr)
{
    m_name.assign(name.c_str());
    m_stdname = name;
    m_type = type;
}

LoadedListItem::~LoadedListItem() {

}