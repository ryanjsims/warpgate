#define GLM_FORCE_XYZW_ONLY

#include "utils/gtk/skeleton.hpp"
#include "utils/sign.h"
#include <unordered_map>

#define _USE_MATH_DEFINES
#include <math.h>

#include <spdlog/spdlog.h>

using namespace warpgate::gtk;

glm::mat4 get_inverse_bind_matrix(warpgate::Bone& bone) {
    warpgate::PackedMat4 packed_inv = bone.inverse_bind_matrix;
    glm::mat4 inverse_bind_matrix(glm::mat4x3(
        packed_inv[0][0], packed_inv[0][1], packed_inv[0][2],
        packed_inv[1][0], packed_inv[1][1], packed_inv[1][2],
        packed_inv[2][0], packed_inv[2][1], packed_inv[2][2],
        packed_inv[3][0], packed_inv[3][1], packed_inv[3][2]
    ));

    return inverse_bind_matrix;
}

Skeleton::Skeleton(std::shared_ptr<DME> dme) {
    std::unordered_map<uint32_t, std::shared_ptr<gtk::Bone>> bones;
    for(uint32_t bone_index = 0; bone_index < dme->bone_count(); bone_index++) {
        warpgate::Bone dme_bone = dme->bone(bone_index);
        uint32_t namehash = dme_bone.namehash;
        auto bone_it = utils::bone_hashmap.find(namehash);
        std::shared_ptr<gtk::Bone> bone;
        if(bone_it != utils::bone_hashmap.end()) {
            bone = std::make_shared<gtk::Bone>(bone_it->second, get_inverse_bind_matrix(dme_bone));
        } else {
            bone = std::make_shared<gtk::Bone>(std::to_string(namehash), get_inverse_bind_matrix(dme_bone));
        }
        bones[namehash] = bone;
        m_skeleton.push_back(bone); // preserve bone index
    }
    for(auto it = bones.begin(); it != bones.end(); it++) {
        std::unordered_map<uint32_t, uint32_t>::iterator parent_iter;
        uint32_t parent = 0;
        if((parent_iter = utils::bone_hierarchy.find(it->first)) != utils::bone_hierarchy.end()) {
            parent = parent_iter->second;
        }
        std::unordered_map<uint32_t, std::shared_ptr<gtk::Bone>>::iterator parent_index;
        while(parent != 0 && (parent_index = bones.find(parent)) == bones.end()) {
            parent = utils::bone_hierarchy.at(parent);
        }
        if(parent != 0) {
            // Bone has parent in the skeleton, add it to its parent's children
            bones[parent]->add_child(it->second);
            it->second->set_parent(bones[parent]);
        } else if(parent == 0) {
            // Bone is the root of the skeleton or we don't know its hierarchy
            m_roots.push_back(it->second);
        }
    }
    for(auto it = bones.begin(); it != bones.end(); it++) {
        it->second->update_tail();
    }
    for(auto it = m_roots.begin(); it != m_roots.end(); it++) {
        reorient(*it);
    }

    for(auto it = m_roots.begin(); it != m_roots.end(); it++) {
        update_global_transform(*it);
    }
}

Skeleton::Skeleton(std::shared_ptr<mrn::SkeletonData> skeleton_data) {

}

Skeleton::~Skeleton() {}

void Skeleton::draw() {
    glBindVertexArray(Bone::vao);
    glBindBuffer(GL_ARRAY_BUFFER, Bone::vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Bone::indices);
    Bone::program->use();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    for(auto it = m_skeleton.begin(); it != m_skeleton.end(); it++) {
        (*it)->draw();
    }
}

void Skeleton::reorient(std::shared_ptr<Bone> root) {
    for(uint32_t child_index = 0; child_index < root->children().size(); child_index++) {
        reorient(root->children()[child_index]);
        glm::vec3 parent_translation = root->get_position();
        glm::quat parent_rotation = root->get_rotation();

        glm::vec3 child_translation = root->children()[child_index]->get_position();
        glm::quat child_rotation = root->children()[child_index]->get_rotation();
        
        child_translation -= parent_translation;
        
        child_rotation = glm::inverse(parent_rotation) * child_rotation;
        child_translation = utils::sign(parent_rotation) * glm::inverse(parent_rotation) * child_translation;
    
        root->children()[child_index]->m_translation = child_translation;
        root->children()[child_index]->m_rotation = child_rotation;
    }
}

void Skeleton::update_global_transform(std::shared_ptr<Bone> root) {
    for(uint32_t child_index = 0; child_index < root->children().size(); child_index++) {
        glm::mat4 local_translation = glm::translate(glm::identity<glm::mat4>(), root->children()[child_index]->get_position());
        glm::mat4 local_rotation = glm::toMat4(root->children()[child_index]->get_rotation());

        auto child = root->children()[child_index];
        child->m_global_transform_matrix = root->m_global_transform_matrix * local_translation * local_rotation;
        
        update_global_transform(child);
    }
}