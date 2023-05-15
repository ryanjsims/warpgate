/*
 * Vulkan Example - Basic indexed triangle rendering
 *
 * Note:
 *	This is a "pedal to the metal" example to show off how to get Vulkan up and displaying something
 *	Contrary to the other examples, this one won't make use of helper functions or initializers
 *	Except in a few cases (swap chain setup e.g.)
 *
 * Copyright (C) 2016-2017 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 *
 * Some of the code was modified so that it can be used to draw inside HikoGUI.
 * Copyright (C) 2022 by Take Vos
 */

#include "utils/vulkan/model_renderer.hpp"
#include "hikogui/file/URL.hpp"
#include "hikogui/file/file_view.hpp"
#include "hikogui/module.hpp"
#include "utils/materials_3.h"
#include <format>
#include <vector>
#include <exception>
#include <cassert>
#include <iostream>

#define DIFFUSE_BINDING 1
#define SPECULAR_BINDING 2
#define NORMAL_BINDING 3
#define DETAIL_MASK_BINDING 4
#define DETAIL_CUBE_BINDING 5

#define VK_CHECK_RESULT(f) \
    do { \
        VkResult res = (f); \
        if (res != VK_SUCCESS) { \
            std::cerr << std::format("Vulkan error {}", hi::to_underlying(res)) << std::endl; \
            std::terminate(); \
        } \
    } while (false)

[[nodiscard]] constexpr static bool operator==(VkRect2D const& lhs, VkRect2D const& rhs) noexcept
{
    return lhs.offset.x == rhs.offset.x and lhs.offset.y == rhs.offset.y and lhs.extent.width == rhs.extent.width and
        lhs.extent.height == rhs.extent.height;
}

[[nodiscard]] constexpr VkRect2D operator&(VkRect2D const& lhs, VkRect2D const& rhs) noexcept
{
    auto left = std::max(lhs.offset.x, rhs.offset.x);
    auto right = std::min(lhs.offset.x + lhs.extent.width, rhs.offset.x + rhs.extent.width);
    auto top = std::max(lhs.offset.y, rhs.offset.y);
    auto bottom = std::min(lhs.offset.y + lhs.extent.height, rhs.offset.y + rhs.extent.height);

    auto width = hi::narrow_cast<int32_t>(right) > left ? right - left : uint32_t{0};
    auto height = hi::narrow_cast<int32_t>(bottom) > top ? bottom - top : uint32_t{0};
    return {VkOffset2D{left, top}, VkExtent2D{width, height}};
}

ModelRenderer::ModelRenderer(VmaAllocator allocator, VkDevice device, VkQueue queue, uint32_t queueFamilyIndex) :
    allocator(allocator), device(device), queue(queue), queueFamilyIndex(queueFamilyIndex), m_camera({0.0f, 1.0f, 2.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f})
{
    loadMaterials();
    createCommandPool();
    //createVertexBuffer(model);
    createUniformBuffer();

    //createTextureImage(albedo);
    //createTextureSampler();
    createDescriptorPool();
    createDescriptorSetLayout();
    //createDescriptorSet();
    
}

ModelRenderer::~ModelRenderer()
{
    if (hasSwapchain) {
        teardownForLostSwapchain();
    }

    destroyDescriptorSet();
    destroyDescriptorSetLayout();
    destroyDescriptorPool();
    destroyTextureSamplers();
    destroyTextureImages();
    destroyUniformBuffer();
    destroyVertexBuffer();
    destroyCommandPool();
    unloadMaterials();
}

void ModelRenderer::loadMaterials() {
    std::ifstream materials(std::filesystem::path(hi::URL{"resource:materials.json"}));
    if(materials.bad()) {
        spdlog::error("Could not find materials.json in the resources directory!");
        return;
    }
    materials.seekg(0, SEEK_END);
    size_t length = materials.tellg();
    materials.seekg(0, SEEK_SET);
    spdlog::info("Loaded materials.json, size {}", length);

    m_materials = nlohmann::json::parse(materials);
    materials.close();
    spdlog::info("Parsed materials.json successfully");
}

void ModelRenderer::unloadMaterials() {
    //m_materials.~basic_json();
}

void ModelRenderer::createCommandPool()
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
}

void ModelRenderer::destroyCommandPool()
{
    vkDestroyCommandPool(device, cmdPool, nullptr);
}

void ModelRenderer::loadModel(std::shared_ptr<warpgate::DME> model, std::unordered_map<uint32_t, gli::texture> textures) {
    std::shared_ptr<warpgate::vulkan::Model> wgModel = std::make_shared<warpgate::vulkan::Model>();
    wgModel->data = model;
    createVertexBuffers(wgModel);

    std::shared_ptr<const warpgate::DMAT> dmat = wgModel->data->dmat();
    for(uint32_t material_index = 0; material_index < dmat->material_count(); material_index++) {
        std::array<int32_t, 6> imageInfoIndices = {-1, -1, -1, -1, -1, -1};
        for(uint32_t param_index = 0; param_index < dmat->material(material_index)->param_count(); param_index++) {
            const warpgate::Parameter &parameter = dmat->material(material_index)->parameter(param_index);
            switch(dmat->material(material_index)->parameter(param_index).type()) {
            case warpgate::Parameter::D3DXParamType::TEXTURE1D:
            case warpgate::Parameter::D3DXParamType::TEXTURE2D:
            case warpgate::Parameter::D3DXParamType::TEXTURE3D:
            case warpgate::Parameter::D3DXParamType::TEXTURE:
            case warpgate::Parameter::D3DXParamType::TEXTURECUBE:
                break;
            default:
                continue;
            }
            std::string texture_semantic = warpgate::Parameter::semantic_texture_type(parameter.semantic_hash());
            if(texture_semantic == "Diffuse") {
                imageInfoIndices[DIFFUSE_BINDING] = static_cast<int32_t>(imageInfos.size());
            } else if(texture_semantic == "Normal") {
                imageInfoIndices[NORMAL_BINDING] = static_cast<int32_t>(imageInfos.size());
            } else if(texture_semantic == "Specular") {
                imageInfoIndices[SPECULAR_BINDING] = static_cast<int32_t>(imageInfos.size());
            } else if(texture_semantic == "Detail Cube") {
                imageInfoIndices[DETAIL_CUBE_BINDING] = static_cast<int32_t>(imageInfos.size());
            } else if(texture_semantic == "Detail Select") {
                imageInfoIndices[DETAIL_MASK_BINDING] = static_cast<int32_t>(imageInfos.size());
            } else {
                continue;
            }
            
            createTextureImage(textures[parameter.get<uint32_t>(parameter.data_offset())]);
            createTextureSampler();
        }

        createDescriptorSet(wgModel->meshes[material_index], imageInfoIndices);

        uint32_t materialDefinition = dmat->material(material_index)->definition();
        std::string inputLayoutName = m_materials["materialDefinitions"][std::to_string(materialDefinition)]["drawStyles"][0]["inputLayout"];
        std::shared_ptr<const warpgate::Mesh> mesh = wgModel->data->mesh(material_index);
        std::unordered_map<uint32_t, uint32_t> vertex_strides;
        for(uint32_t stream_index = 0; stream_index < mesh->vertex_stream_count(); stream_index++){
            vertex_strides[stream_index] = mesh->bytes_per_vertex(stream_index);
        }

        spdlog::info("Using input layout {}", inputLayoutName);

        wgModel->meshes[material_index].pipeline = createPipeline(pipelineLayout, m_materials["inputLayouts"][inputLayoutName], vertex_strides);
    }

    m_models.push_back(wgModel);

    models_changed = true;
}

void ModelRenderer::createVertexBuffers(std::shared_ptr<warpgate::vulkan::Model> model)
{
    for(uint32_t index = 0; index < model->data->mesh_count(); index++) {
        std::shared_ptr<const warpgate::Mesh> mesh = model->data->mesh(index);

        warpgate::vulkan::Mesh curr_mesh;

        // Setup indices
        std::span<uint8_t> vertexIndexData = mesh->index_data();
        uint32_t vertexIndexDataSize = hi::narrow_cast<uint32_t>(vertexIndexData.size());
        curr_mesh.index_count = mesh->index_count();
        curr_mesh.index_size = mesh->index_size() & 0xFF;
        spdlog::info("Index size: {}", curr_mesh.index_size);

        // Create the Vertex-index buffer inside the GPU
        VkBufferCreateInfo vertexIndexBufferCreateInfo = {};
        vertexIndexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexIndexBufferCreateInfo.size = vertexIndexDataSize;
        vertexIndexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo vertexIndexBufferAllocationInfo = {};
        vertexIndexBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        vertexIndexBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VK_CHECK_RESULT(vmaCreateBuffer(
            allocator,
            &vertexIndexBufferCreateInfo,
            &vertexIndexBufferAllocationInfo,
            &curr_mesh.indices,
            &curr_mesh.indicesAllocation,
            nullptr));

        // Copy index data to a buffer visible to the host
        {
            void *data = nullptr;
            VK_CHECK_RESULT(vmaMapMemory(allocator, curr_mesh.indicesAllocation, &data));
            memcpy(data, vertexIndexData.data(), vertexIndexDataSize);
            vmaUnmapMemory(allocator, curr_mesh.indicesAllocation);
        }
        for(uint32_t stream = 0; stream < mesh->vertex_stream_count(); stream++) {
            VkBuffer streamBuffer;
            VmaAllocation streamAllocation;

            // Setup vertices
            std::span<uint8_t> vertexData = mesh->vertex_stream(stream);
            uint32_t vertexDataSize = hi::narrow_cast<uint32_t>(vertexData.size());

            // Create the Vertex buffer inside the GPU
            VkBufferCreateInfo vertexBufferCreateInfo = {};
            vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vertexBufferCreateInfo.size = vertexDataSize;
            vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            VmaAllocationCreateInfo vertexBufferAllocationInfo = {};
            vertexBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            vertexBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

            VK_CHECK_RESULT(vmaCreateBuffer(
                allocator, &vertexBufferCreateInfo, &vertexBufferAllocationInfo, &streamBuffer, &streamAllocation, nullptr));

            // Copy vertex data to a buffer visible to the host
            {
                void *data = nullptr;
                VK_CHECK_RESULT(vmaMapMemory(allocator, streamAllocation, &data));
                memcpy(data, vertexData.data(), vertexDataSize);
                vmaUnmapMemory(allocator, streamAllocation);
            }

            curr_mesh.vertexStreams.push_back(streamBuffer);
            curr_mesh.vertexStreamAllocations.push_back(streamAllocation);
        }
        model->meshes.push_back(curr_mesh);
    }
}

void ModelRenderer::destroyVertexBuffer()
{
    vmaDestroyBuffer(allocator, m_grid->meshes[0].vertexStreams[0], m_grid->meshes[0].vertexStreamAllocations[0]);
    vmaDestroyBuffer(allocator, m_grid->meshes[0].indices, m_grid->meshes[0].indicesAllocation);
    for(auto model : m_models) {
        for(auto mesh_it = model->meshes.begin(); mesh_it != model->meshes.end(); mesh_it++) {
            vmaDestroyBuffer(allocator, mesh_it->indices, mesh_it->indicesAllocation);
            for(uint32_t stream = 0; stream < mesh_it->vertexStreams.size(); stream++) {
                vmaDestroyBuffer(allocator, mesh_it->vertexStreams[stream], mesh_it->vertexStreamAllocations[stream]);
            }
        }
    }
}

void ModelRenderer::createUniformBuffer()
{
    // Vertex shader uniform buffer block
    VkBufferCreateInfo uniformBufferCreateInfo = {};
    uniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniformBufferCreateInfo.size = sizeof(Uniform);
    uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo uniformBufferAllocationInfo = {};
    uniformBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    uniformBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK_RESULT(vmaCreateBuffer(
        allocator, &uniformBufferCreateInfo, &uniformBufferAllocationInfo, &uniformBuffer, &uniformBufferAllocation, nullptr));

    // Store information in the uniform's descriptor that is used by the descriptor set.
    uniformBufferInfo.buffer = uniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(Uniform);

    // Vertex shader uniform buffer block
    VkBufferCreateInfo gridUniformBufferCreateInfo = {};
    gridUniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    gridUniformBufferCreateInfo.size = sizeof(GridUniform);
    gridUniformBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo gridUniformBufferAllocationInfo = {};
    gridUniformBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    gridUniformBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK_RESULT(vmaCreateBuffer(
        allocator, &gridUniformBufferCreateInfo, &gridUniformBufferAllocationInfo, &gridUniformBuffer, &gridUniformBufferAllocation, nullptr));

    // Store information in the uniform's descriptor that is used by the descriptor set.
    gridUniformBufferInfo.buffer = gridUniformBuffer;
    gridUniformBufferInfo.offset = 0;
    gridUniformBufferInfo.range = sizeof(GridUniform);
}

void ModelRenderer::destroyUniformBuffer()
{
    vmaDestroyBuffer(allocator, uniformBuffer, uniformBufferAllocation);
    vmaDestroyBuffer(allocator, gridUniformBuffer, gridUniformBufferAllocation);
}

void ModelRenderer::createDescriptorPool()
{
    // We need to tell the API the number of max. requested descriptors per type
    std::array<VkDescriptorPoolSize, 2> typeCounts;
    // This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
    typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    typeCounts[0].descriptorCount = 10;
    // For additional types you need to add new entries in the type count list
    // E.g. for two combined image samplers :
    typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    typeCounts[1].descriptorCount = 10;

    // Create the global descriptor pool
    // All descriptors used in this example are allocated from this pool
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(typeCounts.size());
    descriptorPoolInfo.pPoolSizes = typeCounts.data();
    // Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result
    // in an error)
    descriptorPoolInfo.maxSets = 16;

    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void ModelRenderer::destroyDescriptorPool()
{
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void ModelRenderer::createDescriptorSetLayout()
{
    // Setup layout of descriptors used in this example
    // Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
    // So every shader binding should map to one descriptor set layout binding

    // Binding 0: Uniform buffer (Vertex shader)
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    // Binding 1: Diffuse Texture Sampler (Fragment shader)
    VkDescriptorSetLayoutBinding diffuseBinding = {};
    diffuseBinding.binding = DIFFUSE_BINDING;
    diffuseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    diffuseBinding.descriptorCount = 1;
    diffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    diffuseBinding.pImmutableSamplers = nullptr;

    // Binding 2: Specular Texture Sampler (Fragment shader)
    VkDescriptorSetLayoutBinding specularBinding = {};
    specularBinding.binding = SPECULAR_BINDING;
    specularBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    specularBinding.descriptorCount = 1;
    specularBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    specularBinding.pImmutableSamplers = nullptr;

    // Binding 3: Normal Texture Sampler (Fragment shader)
    VkDescriptorSetLayoutBinding normalBinding = {};
    normalBinding.binding = NORMAL_BINDING;
    normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalBinding.descriptorCount = 1;
    normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    normalBinding.pImmutableSamplers = nullptr;

    // Binding 4: Detail Mask Texture Sampler (Fragment shader)
    VkDescriptorSetLayoutBinding detailMaskBinding = {};
    detailMaskBinding.binding = DETAIL_MASK_BINDING;
    detailMaskBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    detailMaskBinding.descriptorCount = 1;
    detailMaskBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    detailMaskBinding.pImmutableSamplers = nullptr;

    // Binding 5: Detail Cube Texture Sampler (Fragment shader)
    VkDescriptorSetLayoutBinding detailCubeBinding = {};
    detailCubeBinding.binding = DETAIL_CUBE_BINDING;
    detailCubeBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    detailCubeBinding.descriptorCount = 1;
    detailCubeBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    detailCubeBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 6> bindings = { uboBinding, diffuseBinding, specularBinding, normalBinding, detailMaskBinding, detailCubeBinding };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
    descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pNext = nullptr;
    descriptorLayout.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptorLayout.pBindings = bindings.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

    // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set
    // layout In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that
    // could be reused
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = nullptr;
    pPipelineLayoutCreateInfo.setLayoutCount = 1;
    pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

void ModelRenderer::destroyDescriptorSetLayout()
{
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void ModelRenderer::createTextureImage(gli::texture texture)
{
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VkBufferCreateInfo stagingBufferCreateInfo = {};
    stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferCreateInfo.size = texture.size();
    stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stagingBufferAllocationCreateInfo = {};
    stagingBufferAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingBufferAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VK_CHECK_RESULT(vmaCreateBuffer(
        allocator,
        &stagingBufferCreateInfo,
        &stagingBufferAllocationCreateInfo,
        &stagingBuffer,
        &stagingAllocation,
        nullptr
    ));

    {
        void* data = nullptr;
        VK_CHECK_RESULT(vmaMapMemory(allocator, stagingAllocation, &data));
        memcpy(data, texture.data(), texture.size());
        vmaUnmapMemory(allocator, stagingAllocation);
    }

    gli::target target = texture.target();
    VkImageViewType vkTarget = VK_IMAGE_VIEW_TYPE_2D;
    switch(target) {
        case gli::target::TARGET_1D:
            vkTarget = VK_IMAGE_VIEW_TYPE_1D;
            break;
        case gli::target::TARGET_2D:
            vkTarget = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case gli::target::TARGET_3D:
            vkTarget = VK_IMAGE_VIEW_TYPE_3D;
            break;
        case gli::target::TARGET_1D_ARRAY:
            vkTarget = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;
        case gli::target::TARGET_2D_ARRAY:
            vkTarget = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        case gli::target::TARGET_CUBE:
            vkTarget = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case gli::target::TARGET_CUBE_ARRAY:
            vkTarget = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            break;
    }

    VkImage textureImage;
    VmaAllocation textureImageAllocation;
    createImage(
        texture.extent().x, 
        texture.extent().y,
        vkTarget,
        VK_SAMPLE_COUNT_1_BIT, 
        (VkFormat)texture.format(), 
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_AUTO,
        textureImageAllocation,
        textureImage
    );
    textureImages.push_back(textureImage);
    textureImageAllocations.push_back(textureImageAllocation);

    transitionImageLayout(textureImage, (VkFormat)texture.format(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkTarget);
    copyBufferToImage(stagingBuffer, textureImage, texture.extent().x, texture.extent().y, vkTarget);
    transitionImageLayout(textureImage, (VkFormat)texture.format(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkTarget);

    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

    VkImageView textureImageView;
    createImageView(textureImage, vkTarget, (VkFormat)texture.format(), VK_IMAGE_ASPECT_COLOR_BIT, textureImageView);
    textureImageViews.push_back(textureImageView);
}

void ModelRenderer::destroyTextureImages()
{
    for(uint32_t i = 0; i < textureImages.size(); i++) {
        vkDestroyImageView(device, textureImageViews[i], nullptr);
        vmaDestroyImage(allocator, textureImages[i], textureImageAllocations[i]);
    }
}

void ModelRenderer::createTextureSampler() {
    VkSamplerCreateInfo textureSamplerCreateInfo = {};
    textureSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    textureSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    textureSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    textureSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    textureSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    textureSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    textureSamplerCreateInfo.anisotropyEnable = VK_TRUE;
    textureSamplerCreateInfo.maxAnisotropy = 16;
    textureSamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    textureSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    textureSamplerCreateInfo.compareEnable = VK_FALSE;
    textureSamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    textureSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    textureSamplerCreateInfo.mipLodBias = 0.0f;
    textureSamplerCreateInfo.minLod = 0.0f;
    textureSamplerCreateInfo.maxLod = 0.0f;

    VkSampler textureSampler;
    VK_CHECK_RESULT(vkCreateSampler(device, &textureSamplerCreateInfo, nullptr, &textureSampler));

    VkDescriptorImageInfo imageInfo;

    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageViews[imageInfos.size()];
    imageInfo.sampler = textureSampler;

    textureSamplers.push_back(textureSampler);
    imageInfos.push_back(imageInfo);
}

void ModelRenderer::destroyTextureSamplers() {
    for(VkSampler textureSampler : textureSamplers) {
        vkDestroySampler(device, textureSampler, nullptr);
    }
}

void ModelRenderer::createDescriptorSet(warpgate::vulkan::Mesh &mesh, std::array<int32_t, 6> imageInfoIndices)
{
    // Allocate a new descriptor set from the global descriptor pool
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &mesh.descriptorSet));

    // Update the descriptor set determining the shader binding points
    // For every binding point used in a shader there needs to be one
    // descriptor set matching that binding point

    std::array<VkWriteDescriptorSet, 6> writeDescriptorSets = {};

    // Binding 0 : Uniform buffer
    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = mesh.descriptorSet;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].pBufferInfo = &uniformBufferInfo;
    // Binds this uniform buffer to binding point 0
    writeDescriptorSets[0].dstBinding = 0;


    for(uint32_t i = DIFFUSE_BINDING; i <= DETAIL_CUBE_BINDING; i++) {
        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        if(imageInfoIndices[i] < 0) {
            continue;
        }
        // Binding 1 - 5 : Texture Samplers
        writeDescriptorSets[i].dstSet = mesh.descriptorSet;
        writeDescriptorSets[i].descriptorCount = 1;
        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[i].pImageInfo = &imageInfos[imageInfoIndices[i]];
        // Binds this texture sampler to a defined binding point
        writeDescriptorSets[i].dstBinding = i;
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void ModelRenderer::destroyDescriptorSet()
{
    for(auto model : m_models) {
        for(auto& mesh : model->meshes) {
            vkFreeDescriptorSets(device, descriptorPool, 1, &mesh.descriptorSet);
        }
    }
}

void ModelRenderer::buildForNewSwapchain(std::vector<VkImageView> const& imageViews, VkExtent2D imageSize, VkFormat imageFormat)
{
    assert(not hasSwapchain);

    auto colorImageFormat = imageFormat;
    auto depthImageFormat = VK_FORMAT_D24_UNORM_S8_UINT;

    createRenderPass(colorImageFormat, depthImageFormat);
    createMSAARenderTarget(imageSize, colorImageFormat);
    createDepthStencilImage(imageSize, depthImageFormat);
    createFrameBuffers(imageViews, imageSize);
    createCommandBuffers();
    createFences();

    {
        m_grid = std::make_shared<warpgate::vulkan::Model>();
        warpgate::vulkan::Mesh mesh;
        mesh.pipeline = 0;

        // Binding 0: Uniform buffer for matrices
        VkDescriptorSetLayoutBinding uboBinding = {};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        uboBinding.pImmutableSamplers = nullptr;

        // Binding 1: Uniform buffer for clip planes
        VkDescriptorSetLayoutBinding ubo2Binding = {};
        ubo2Binding.binding = 1;
        ubo2Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo2Binding.descriptorCount = 1;
        ubo2Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        ubo2Binding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboBinding, ubo2Binding};

        VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
        descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayout.pNext = nullptr;
        descriptorLayout.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorLayout.pBindings = bindings.data();

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &gridDescriptorSetLayout));

        // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set
        // layout In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that
        // could be reused
        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &gridDescriptorSetLayout;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &gridPipelineLayout));

        // Allocate a new descriptor set from the global descriptor pool
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &gridDescriptorSetLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &mesh.descriptorSet));

        // Update the descriptor set determining the shader binding points
        // For every binding point used in a shader there needs to be one
        // descriptor set matching that binding point

        std::array<VkWriteDescriptorSet, 2> writeDescriptorSets = {};

        // Binding 0 : Uniform buffer
        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].dstSet = mesh.descriptorSet;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].pBufferInfo = &uniformBufferInfo;
        // Binds this uniform buffer to binding point 0
        writeDescriptorSets[0].dstBinding = 0;

        // Binding 1 : Uniform buffer
        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].dstSet = mesh.descriptorSet;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[1].pBufferInfo = &gridUniformBufferInfo;
        // Binds this uniform buffer to binding point 1
        writeDescriptorSets[1].dstBinding = 1;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

        std::vector<float> vertices = {
            1.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            1, 0, 3
        };

        VkBuffer vertexBuffer;
        VmaAllocation vertexAllocation;

        // Create the Vertex buffer inside the GPU
        VkBufferCreateInfo vertexBufferCreateInfo = {};
        vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferCreateInfo.size = vertices.size() * sizeof(float);
        vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo vertexBufferAllocationInfo = {};
        vertexBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        vertexBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VK_CHECK_RESULT(vmaCreateBuffer(
            allocator, &vertexBufferCreateInfo, &vertexBufferAllocationInfo, &vertexBuffer, &vertexAllocation, nullptr));

        // Copy vertex data to a buffer visible to the host
        {
            void *data = nullptr;
            VK_CHECK_RESULT(vmaMapMemory(allocator, vertexAllocation, &data));
            memcpy(data, vertices.data(), vertices.size() * sizeof(float));
            vmaUnmapMemory(allocator, vertexAllocation);
        }

        mesh.vertexStreams.push_back(vertexBuffer);
        mesh.vertexStreamAllocations.push_back(vertexAllocation);

        // Create the Vertex-index buffer inside the GPU
        VkBufferCreateInfo vertexIndexBufferCreateInfo = {};
        vertexIndexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexIndexBufferCreateInfo.size = indices.size() * sizeof(uint32_t);
        vertexIndexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo vertexIndexBufferAllocationInfo = {};
        vertexIndexBufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        vertexIndexBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VK_CHECK_RESULT(vmaCreateBuffer(
            allocator,
            &vertexIndexBufferCreateInfo,
            &vertexIndexBufferAllocationInfo,
            &mesh.indices,
            &mesh.indicesAllocation,
            nullptr));

        // Copy index data to a buffer visible to the host
        {
            void *data = nullptr;
            VK_CHECK_RESULT(vmaMapMemory(allocator, mesh.indicesAllocation, &data));
            memcpy(data, indices.data(), indices.size() * sizeof(uint32_t));
            vmaUnmapMemory(allocator, mesh.indicesAllocation);
        }

        mesh.index_count = static_cast<uint32_t>(indices.size());
        mesh.index_size = sizeof(uint32_t);
        
        m_grid->meshes.push_back(mesh);

        std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
            {
                .binding = 0,
                .stride = 12,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        };

        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
            // Attribute location 0: Position
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0
            }
        };

        pipelines[0] = createPipeline(
            gridPipelineLayout, //todo: make this use samplers for multiple textures
            renderPass,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_NONE,
            1.0f, VK_TRUE, VK_TRUE,
            vertexInputBindings,
            vertexInputAttributes,
            hi::URL{"resource:shaders/grid.vert.spv"},
            hi::URL{"resource:shaders/grid.frag.spv"}
        );
    }

    for(auto model : m_models) {
        for(uint32_t mesh_index = 0; mesh_index < model->meshes.size(); mesh_index++){
            uint32_t materialDefinition = model->data->dmat()->material(mesh_index)->definition();
            std::string inputLayoutName = m_materials["materialDefinitions"][std::to_string(materialDefinition)]["drawStyles"][0]["inputLayout"];
            std::shared_ptr<const warpgate::Mesh> mesh = model->data->mesh(mesh_index);
            std::unordered_map<uint32_t, uint32_t> vertex_strides;
            for(uint32_t stream_index = 0; stream_index < mesh->vertex_stream_count(); stream_index++){
                vertex_strides[stream_index] = mesh->bytes_per_vertex(stream_index);
            }

            model->meshes[mesh_index].pipeline = createPipeline(pipelineLayout, m_materials["inputLayouts"][inputLayoutName], vertex_strides);
        }
    }
    //createPipeline();

    hasSwapchain = true;
    previousRenderArea = {};
    previousViewPort = {};
}

void ModelRenderer::teardownForLostSwapchain()
{
    assert(hasSwapchain);

    for (auto& fence : queueCompleteFences) {
        VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    }

    hasSwapchain = false;

    destroyPipelines();
    destroyFences();
    destroyCommandBuffers();
    destroyFrameBuffers();
    destroyDepthStencilImage();
    destroyMSAARenderTarget();
    destroyRenderPass();
}

void ModelRenderer::createRenderPass(VkFormat colorFormat, VkFormat depthFormat)
{
    // This example will use a single render pass with one subpass

    // Descriptors for the attachments used by this renderpass
    std::array<VkAttachmentDescription, 3> attachments = {};

    // Color attachment
    // In HikoGUI we reuse the previous drawn swap-chain image, therefor:
    // - initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    //
    // Although the loadOp is VK_ATTACHMENT_LOAD_OP_CLEAR, it only clears the renderArea/scissor rectangle.
    // The initialLayout makes sure that the previous image is reused.
    attachments[0].format = colorFormat; // Use the color format selected by the swapchain
    attachments[0].samples = VK_SAMPLE_COUNT_8_BIT; // Use 8 bit multi sampling in this example
    attachments[0].loadOp =
        VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear this attachment at the start of the render pass.
    attachments[0].storeOp =
        VK_ATTACHMENT_STORE_OP_STORE; // Keep its contents after the render pass is finished (for displaying it)
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We don't use stencil, so don't care for load
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Same for store
    attachments[0].initialLayout =
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Reuse the previous draw image, so the layout is already in present mode.
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout to which the attachment is transitioned when the
                                                                  // render pass is finished As we want to present the color
                                                                  // buffer to the swapchain, we transition to PRESENT_KHR
    // Depth attachment
    attachments[1].format = depthFormat; // A proper depth format is selected in the example base
    attachments[1].samples = VK_SAMPLE_COUNT_8_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear depth at start of first subpass
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // We don't need depth after render pass has finished
                                                               // (DONT_CARE may result in better performance)
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // No Stencil
    attachments[1].initialLayout =
        VK_IMAGE_LAYOUT_UNDEFINED; // Layout at render pass start. Initial doesn't matter, so we use undefined
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Transition to depth/stencil attachment


    // Resolve attachment
    attachments[2].format = colorFormat;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Keep its contents after the render pass is finished (for displaying it)
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We don't use stencil, so don't care for load
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Same for store
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout to which the attachment is transitioned when the
                                                                  // render pass is finished As we want to present the color
                                                                  // buffer to the swapchain, we transition to PRESENT_KHR

    // Setup attachment references
    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0; // Attachment 0 is color
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Attachment layout used as color during the subpass

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1; // Attachment 1 is color
    depthReference.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Attachment used as depth/stencil used during the subpass

    // Setup attachment references
    VkAttachmentReference resolveReference = {};
    resolveReference.attachment = 2; // Attachment 2 is color
    resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Attachment layout used as color during the subpass

    // Setup a single subpass reference
    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1; // Subpass uses one color attachment
    subpassDescription.pColorAttachments = &colorReference; // Reference to the color attachment in slot 0
    subpassDescription.pDepthStencilAttachment = &depthReference; // Reference to the depth attachment in slot 1
    subpassDescription.inputAttachmentCount = 0; // Input attachments can be used to sample from contents of a previous subpass
    subpassDescription.pInputAttachments = nullptr; // (Input attachments not used by this example)
    subpassDescription.preserveAttachmentCount =
        0; // Preserved attachments can be used to loop (and preserve) attachments through subpasses
    subpassDescription.pPreserveAttachments = nullptr; // (Preserve attachments not used by this example)
    subpassDescription.pResolveAttachments = &resolveReference; // Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

    // Setup subpass dependencies
    // These will add the implicit attachment layout transitions specified by the attachment descriptions
    // The actual usage layout is preserved through the layout specified in the attachment reference
    // Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described
    // by srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set) Note: VK_SUBPASS_EXTERNAL is a
    // special constant that refers to all commands executed outside of the actual renderpass)
    std::array<VkSubpassDependency, 2> dependencies;

    // First dependency at the start of the renderpass
    // Does the transition from final to initial layout
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // Producer of the dependency
    dependencies[0].dstSubpass = 0; // Consumer is our single subpass that will wait for the execution dependency
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Match our pWaitDstStageMask when we vkQueueSubmit
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // is a loadOp stage for color attachments
    dependencies[0].srcAccessMask = 0; // semaphore wait already does memory dependency for us
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // is a loadOp CLEAR access mask for color attachments
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Second dependency at the end the renderpass
    // Does the transition from the initial to the final layout
    // Technically this is the same as the implicit subpass dependency, but we are gonna state it explicitly here
    dependencies[1].srcSubpass = 0; // Producer of the dependency is our single subpass
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL; // Consumer are all commands outside of the renderpass
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // is a storeOp stage for color attachments
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Do not block any subsequent work
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // is a storeOp `STORE` access mask for color attachments
    dependencies[1].dstAccessMask = 0;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the actual renderpass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount =
        hi::narrow_cast<uint32_t>(attachments.size()); // Number of attachments used by this render pass
    renderPassInfo.pAttachments = attachments.data(); // Descriptions of the attachments used by the render pass
    renderPassInfo.subpassCount = 1; // We only use one subpass in this example
    renderPassInfo.pSubpasses = &subpassDescription; // Description of that subpass
    renderPassInfo.dependencyCount = hi::narrow_cast<uint32_t>(dependencies.size()); // Number of subpass dependencies
    renderPassInfo.pDependencies = dependencies.data(); // Subpass dependencies used by the render pass

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void ModelRenderer::destroyRenderPass()
{
    vkDestroyRenderPass(device, renderPass, nullptr);
}

void ModelRenderer::createMSAARenderTarget(VkExtent2D imageSize, VkFormat format) {
    createImage(
        imageSize.width, imageSize.height,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_SAMPLE_COUNT_8_BIT, format, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
        VMA_MEMORY_USAGE_AUTO,
        msaaImageAllocation, msaaImage
    );

    createImageView(msaaImage, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT, msaaImageView);
    //transitionImageLayout(msaaImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_VIEW_TYPE_2D);
}

void ModelRenderer::destroyMSAARenderTarget() {
    vkDestroyImageView(device, msaaImageView, nullptr);
    vmaDestroyImage(allocator, msaaImage, msaaImageAllocation);
}

void ModelRenderer::createDepthStencilImage(VkExtent2D imageSize, VkFormat format)
{
    createImage(
        imageSize.width, imageSize.height,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_SAMPLE_COUNT_8_BIT,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_AUTO,
        depthImageAllocation, depthImage
    );

    createImageView(depthImage, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView);
}

void ModelRenderer::destroyDepthStencilImage()
{
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(allocator, depthImage, depthImageAllocation);
}

// Create a frame buffer for each swap chain image
// Note: Override of virtual function in the base class and called from within ModelRendererBase::prepare
void ModelRenderer::createFrameBuffers(std::vector<VkImageView> const& swapChainImageViews, VkExtent2D imageSize)
{
    // Create a frame buffer for every image in the swapchain
    assert(frameBuffers.size() <= swapChainImageViews.size());
    frameBuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < frameBuffers.size(); i++) {
        std::array<VkImageView, 3> attachments;
        attachments[0] = msaaImageView; // Color attachment is the view of the swapchain image
        attachments[1] = depthImageView; // Depth/Stencil attachment is the same for all frame buffers
        attachments[2] = swapChainImageViews[i];

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // All frame buffers use the same renderpass setup
        frameBufferCreateInfo.renderPass = renderPass;
        frameBufferCreateInfo.attachmentCount = hi::narrow_cast<uint32_t>(attachments.size());
        frameBufferCreateInfo.pAttachments = attachments.data();
        frameBufferCreateInfo.width = imageSize.width;
        frameBufferCreateInfo.height = imageSize.height;
        frameBufferCreateInfo.layers = 1;
        // Create the framebuffer
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
}

void ModelRenderer::destroyFrameBuffers()
{
    for (auto const& frameBuffer : frameBuffers) {
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }
}

void ModelRenderer::createCommandBuffers()
{
    // Create one command buffer for each swap chain image and reuse for rendering
    drawCmdBuffers.resize(frameBuffers.size());

    auto cmdBufAllocateInfo = VkCommandBufferAllocateInfo{};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = cmdPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = hi::narrow_cast<uint32_t>(drawCmdBuffers.size());

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
}

void ModelRenderer::destroyCommandBuffers()
{
    vkFreeCommandBuffers(device, cmdPool, hi::narrow_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
}

void ModelRenderer::createFences()
{
    // Fences (Used to check draw command buffer completion)
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create in signaled state so we don't wait on first render of each command buffer
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    queueCompleteFences.resize(drawCmdBuffers.size());
    for (auto& fence : queueCompleteFences) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    }
}

void ModelRenderer::destroyFences()
{
    for (auto fence : queueCompleteFences) {
        vkDestroyFence(device, fence, nullptr);
    }
}

void ModelRenderer::createPipeline()
{
    // Create the graphics pipeline used in this example
    // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
    // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
    // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used
    // is)

    // Vertex input binding
    // This renderer uses two vertex input bindings (see vkCmdBindVertexBuffers)
    // Currently hard coded for "Character" inputLayout
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
        {
            .binding = 0,
            .stride = 20,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        },
        {
            .binding = 1,
            .stride = 20,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    // Input attribute bindings describe shader attribute locations and memory layouts
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        // Attribute location 0: Position
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        },
        // Attribute location 1: BlendWeight
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .offset = 12
        },
        // Attribute location 2: BlendIndices
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R8G8B8A8_UINT,
            .offset = 16
        },

        // Attribute location 3: Tangent
        {
            .location = 3,
            .binding = 1,
            .format = VK_FORMAT_R8G8B8A8_SNORM,
            .offset = 0
        },
        // Attribute location 4: Binormal
        {
            .location = 4,
            .binding = 1,
            .format = VK_FORMAT_R8G8B8A8_SNORM,
            .offset = 4
        },
        // Attribute location 5: Texcoord0
        {
            .location = 5,
            .binding = 1,
            .format = VK_FORMAT_R16G16_SFLOAT,
            .offset = 8
        },
        // Attribute location 6: Texcoord1
        {
            .location = 6,
            .binding = 1,
            .format = VK_FORMAT_R16G16_SFLOAT,
            .offset = 12
        },
        // Attribute location 7: Texcoord2
        {
            .location = 7,
            .binding = 1,
            .format = VK_FORMAT_R16G16_SFLOAT,
            .offset = 16
        }
    };

    pipelines[2201291178] = createPipeline(
        pipelineLayout,
        renderPass,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        1.0f, VK_TRUE, VK_TRUE,
        vertexInputBindings,
        vertexInputAttributes,
        hi::URL{"resource:shaders/character.vert.spv"},
        hi::URL{"resource:shaders/character.frag.spv"}
    );
}

VkFormat getFormat(std::string type, std::string usage) {
    if(type == "Float1") {
        return VK_FORMAT_R32_SFLOAT;
    } else if(type == "Float2") {
        return VK_FORMAT_R32G32_SFLOAT;
    } else if(type == "Float3") {
        return VK_FORMAT_R32G32B32_SFLOAT;
    } else if(type == "Float4") {
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    } else if(type == "D3dcolor") {
        if(usage == "BlendIndices") {
            return VK_FORMAT_R8G8B8A8_UINT;
        } else if(usage == "Normal" || usage == "Tangent") {
            return VK_FORMAT_R8G8B8A8_SNORM;
        } else if(usage == "Color") {
            return VK_FORMAT_R8G8B8A8_UNORM;
        } else {
            return VK_FORMAT_R8G8B8A8_UNORM;
        }
    } else if(type == "ubyte4n") {
        if(usage == "Binormal") {
            return VK_FORMAT_R8G8B8A8_SNORM;
            // not really sure but we'll see
        } else if(usage == "Normal" || usage == "Tangent" || usage == "Position") {
            return VK_FORMAT_R8G8B8A8_SNORM;
        } else if(usage == "Color") {
            return VK_FORMAT_R8G8B8A8_UNORM;
        } else {
            return VK_FORMAT_R8G8B8A8_UNORM;
        }
    } else if(type == "Float16_2" || type == "float16_2") {
        return VK_FORMAT_R16G16_SFLOAT;
    } else if(type == "Short2") {
        if(usage == "Texcoord") {
            return VK_FORMAT_R16G16_UNORM;
        } else {
            return VK_FORMAT_R16G16_SNORM;
        }
    } else if(type == "Short4") {
        return VK_FORMAT_R16G16B16A16_UNORM;
    }

    return VK_FORMAT_UNDEFINED;
}

uint32_t ModelRenderer::createPipeline(VkPipelineLayout pipe_layout, nlohmann::json inputLayout, std::unordered_map<uint32_t, uint32_t> model_strides) {
    /**
     * inputLayout has structure:
    {
        "name": "NULL",
        "sizes": {
            "0": 12
        },
        "hash": 1152478178,
        "entries": [
            {
                "stream": 0,
                "type": "Float3",
                "usage": "Position",
                "usageIndex": 0
            }
        ]
    }
     */

    std::vector<VkVertexInputBindingDescription> vertexInputBindings;
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;

    std::unordered_map<uint32_t, uint32_t> stream_strides;
    uint32_t location = 0;

    for(nlohmann::json entry : inputLayout["entries"]) {
        if(stream_strides.find(entry["stream"]) == stream_strides.end()) {
            stream_strides[entry["stream"]] = 0;
        }
        if(stream_strides[entry["stream"]] == model_strides[entry["stream"]]) {
            continue;
        }

        VkVertexInputAttributeDescription inputAttributeDescription = {
            .location = location,
            .binding = entry["stream"],
            .format = getFormat(entry["type"], entry["usage"]),
            .offset = stream_strides[entry["stream"]]
        };

        stream_strides[entry["stream"]] += warpgate::utils::materials3::sizes[entry["type"]];
        vertexInputAttributes.push_back(inputAttributeDescription);
        location++;
    }

    uint32_t stride_sum = 0;
    for(auto it = stream_strides.begin(); it != stream_strides.end(); it++) {
        vertexInputBindings.push_back({
            .binding = it->first,
            .stride = it->second,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        });
        stride_sum += it->second;
    }

    if(pipelines.find(inputLayout["hash"] + stride_sum) == pipelines.end()) {
        pipelines[inputLayout["hash"] + stride_sum] = createPipeline(
            pipe_layout, //todo: make this use samplers for multiple textures
            renderPass,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_NONE,
            1.0f, VK_TRUE, VK_TRUE,
            vertexInputBindings,
            vertexInputAttributes,
            hi::URL{"resource:shaders/" + inputLayout["name"].get<std::string>() + ".vert.spv"},
            hi::URL{"resource:shaders/" + inputLayout["name"].get<std::string>() + ".frag.spv"}
        );
    }
    return inputLayout["hash"] + stride_sum;
}

VkPipeline ModelRenderer::createPipeline(
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    VkPrimitiveTopology topology,
    VkPolygonMode polygonMode,
    VkCullModeFlags cullMode,
    float rasterizationLineWidth,
    VkBool32 depthTestEnable,
    VkBool32 depthWriteEnable,
    std::vector<VkVertexInputBindingDescription> vertexInputBindings,
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
    std::filesystem::path vertexShaderPath,
    std::filesystem::path fragmentShaderPath
) {
    // Create the graphics pipeline used in this example
    // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
    // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
    // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used
    // is)

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
    pipelineCreateInfo.layout = layout;
    // Renderpass this pipeline is attached to
    pipelineCreateInfo.renderPass = renderPass;

    // Construct the different states making up the pipeline

    // Input assembly state describes how primitives are assembled
    // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = topology;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = polygonMode;
    rasterizationState.cullMode = cullMode;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.lineWidth = rasterizationLineWidth;

    // Color blend state describes how blend factors are calculated (if used)
    // We need one blend attachment state per color attachment (even if blending is not used)
    VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
    blendAttachmentState[0].colorWriteMask = 0xf;
    blendAttachmentState[0].blendEnable = VK_TRUE;
    blendAttachmentState[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState[0].colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachmentState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState[0].alphaBlendOp = VK_BLEND_OP_ADD;
    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = blendAttachmentState;

    // Viewport state sets the number of viewports and scissor used in this pipeline
    // Note: This is actually overridden by the dynamic states (see below)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Enable dynamic states
    // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command
    // buffer To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their
    // actual states are set later on in the command buffer. For this example we will set the viewport and scissor using
    // dynamic states
    std::vector<VkDynamicState> dynamicStateEnables;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = hi::narrow_cast<uint32_t>(dynamicStateEnables.size());

    // Depth and stencil state containing depth and stencil compare and test operations
    // We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = depthTestEnable;
    depthStencilState.depthWriteEnable = depthTestEnable;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = depthStencilState.back;

    // Multi sampling state
    // This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the
    // pipeline
    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_8_BIT;
    multisampleState.sampleShadingEnable = VK_FALSE;

    // Vertex input state used for pipeline creation
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
    vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    // Shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // Vertex shader
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Set pipeline stage for this shader
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    // Load binary SPIR-V shader
    shaderStages[0].module = loadSPIRVShader(vertexShaderPath);
    // Main entry point for the shader
    shaderStages[0].pName = "main";
    assert(shaderStages[0].module != VK_NULL_HANDLE);

    // Fragment shader
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Set pipeline stage for this shader
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Load binary SPIR-V shader
    shaderStages[1].module = loadSPIRVShader(fragmentShaderPath);
    // Main entry point for the shader
    shaderStages[1].pName = "main";
    assert(shaderStages[1].module != VK_NULL_HANDLE);

    // Set pipeline shader stage info
    pipelineCreateInfo.stageCount = hi::narrow_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();

    // Assign the pipeline states to the pipeline creation info structure
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;

    VkPipeline result;

    // Create rendering pipeline using the specified states
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &result));

    // Shader modules are no longer needed once the graphics pipeline has been created
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);

    return result;
}

void ModelRenderer::destroyPipelines()
{
    for(auto it = pipelines.begin(); it != pipelines.end(); it++) {
        vkDestroyPipeline(device, it->second, nullptr);
    }
}

// Get a new command buffer from the command pool
// If begin is true, the command buffer is also started so we can start adding commands
VkCommandBuffer ModelRenderer::getCommandBuffer(bool begin)
{
    VkCommandBuffer cmdBuffer;

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = cmdPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

    // If requested, also start the new command buffer
    if (begin) {
        auto cmdBufInfo = VkCommandBufferBeginInfo{};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }

    return cmdBuffer;
}

// End the command buffer and submit it to the queue
// Uses a fence to ensure command buffer has finished executing before deleting it
void ModelRenderer::flushCommandBuffer(VkCommandBuffer commandBuffer)
{
    assert(commandBuffer != VK_NULL_HANDLE);

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));

    // Submit to the queue
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, 100'000'000'000));

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}

void ModelRenderer::buildCommandBuffers(VkRect2D renderArea, VkRect2D viewPort)
{
    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear
    // values for both
    VkClearValue clearValues[2];
    clearValues[0].color = {{0.051f, 0.051f, 0.051f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = frameBuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

        // Start the first sub pass specified in our default render pass setup by the base class
        // This will clear the color and depth attachment
        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Update dynamic viewport state
        VkViewport viewport = {};
        viewport.x = hi::narrow_cast<float>(viewPort.offset.x);
        viewport.y = hi::narrow_cast<float>(viewPort.offset.y);
        viewport.height = hi::narrow_cast<float>(viewPort.extent.height);
        viewport.width = hi::narrow_cast<float>(viewPort.extent.width);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        // We are not allowed to draw outside of the renderArea, nor outside of the viewPort
        VkRect2D scissor = renderArea & viewPort;
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        // Bind descriptor sets describing shader binding points
        vkCmdBindDescriptorSets(
            drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout, 0, 1, &m_grid->meshes[0].descriptorSet, 0, nullptr);
        
        // Bind the rendering pipeline
        // The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states
        // specified at pipeline creation time
        vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[m_grid->meshes[0].pipeline]);

        // Bind model vertex buffer (contains position and colors)
        std::vector<VkDeviceSize> offsets(m_grid->meshes[0].vertexStreams.size(), 0ull);
        vkCmdBindVertexBuffers(
            drawCmdBuffers[i],
            0,
            static_cast<uint32_t>(m_grid->meshes[0].vertexStreams.size()),
            m_grid->meshes[0].vertexStreams.data(),
            offsets.data()
        );

        // Bind model index buffer
        vkCmdBindIndexBuffer(drawCmdBuffers[i], m_grid->meshes[0].indices, 0, VK_INDEX_TYPE_UINT32);

        // Draw indexed triangle
        vkCmdDrawIndexed(drawCmdBuffers[i], m_grid->meshes[0].index_count, 1, 0, 0, 1);

        if(models_changed) {
            models_changed = false;
        }

        for(auto model : m_models) {
            for(auto& mesh : model->meshes) {
                // Bind descriptor sets describing shader binding points
                vkCmdBindDescriptorSets(
                    drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mesh.descriptorSet, 0, nullptr);
                
                // Bind the rendering pipeline
                // The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states
                // specified at pipeline creation time
                vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[mesh.pipeline]);

                // Bind model vertex buffer (contains position and colors)
                std::vector<VkDeviceSize> mesh_offsets(mesh.vertexStreams.size(), 0ull);
                vkCmdBindVertexBuffers(
                    drawCmdBuffers[i],
                    0,
                    static_cast<uint32_t>(mesh.vertexStreams.size()),
                    mesh.vertexStreams.data(),
                    mesh_offsets.data()
                );

                // Bind model index buffer
                vkCmdBindIndexBuffer(drawCmdBuffers[i], mesh.indices, 0, mesh.index_size == 4 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);

                // Draw indexed triangle
                vkCmdDrawIndexed(drawCmdBuffers[i], mesh.index_count, 1, 0, 0, 1);
            }
        }

        vkCmdEndRenderPass(drawCmdBuffers[i]);

        // Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}

void ModelRenderer::draw(uint32_t currentBuffer, VkSemaphore presentCompleteSemaphore, VkSemaphore renderCompleteSemaphore)
{
    // Use a fence to wait until the command buffer has finished execution before using it again
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &queueCompleteFences[currentBuffer], VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(vkResetFences(device, 1, &queueCompleteFences[currentBuffer]));

    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // The submit info structure specifies a command buffer queue submission batch
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask =
        &waitStageMask; // Pointer to the list of pipeline stages that the semaphore waits will occur at
    submitInfo.waitSemaphoreCount = 1; // One wait semaphore
    submitInfo.signalSemaphoreCount = 1; // One signal semaphore
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer]; // Command buffers(s) to execute in this batch (submission)
    submitInfo.commandBufferCount = 1; // One command buffer

    // SRS - on other platforms use original bare code with local semaphores/fences for illustrative purposes
    submitInfo.pWaitSemaphores =
        &presentCompleteSemaphore; // Semaphore(s) to wait upon before the submitted command buffer starts executing
    submitInfo.pSignalSemaphores = &renderCompleteSemaphore; // Semaphore(s) to be signaled when command buffers have completed

    // Submit to the graphics queue passing a wait fence
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, queueCompleteFences[currentBuffer]));
}

// Vulkan loads its shaders from an immediate binary representation called SPIR-V
// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
// This function loads such a shader from a binary file and returns a shader module structure
VkShaderModule ModelRenderer::loadSPIRVShader(std::filesystem::path filename)
{
    auto view = hi::file_view(filename);
    auto span = as_span<uint32_t const>(view);

    // Create a new shader module that will be used for pipeline creation
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = hi::narrow_cast<uint32_t>(span.size() * sizeof(uint32_t));
    moduleCreateInfo.pCode = span.data();

    VkShaderModule shaderModule;
    VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

    return shaderModule;
}

void ModelRenderer::updateUniformBuffers(Uniform const& uniform, GridUniform const& gridUniform)
{
    // Map uniform buffer and update it
    void *data;

    VK_CHECK_RESULT(vmaMapMemory(allocator, uniformBufferAllocation, &data));
    memcpy(data, &uniform, sizeof(Uniform));
    vmaUnmapMemory(allocator, uniformBufferAllocation);

    VK_CHECK_RESULT(vmaMapMemory(allocator, gridUniformBufferAllocation, &data));
    memcpy(data, &gridUniform, sizeof(GridUniform));
    vmaUnmapMemory(allocator, gridUniformBufferAllocation);
}

warpgate::vulkan::Camera &ModelRenderer::camera() {
    return m_camera;
}

void ModelRenderer::calculateMatrices(VkRect2D viewPort) {
    clip_planes.near_plane = 0.01f;
    clip_planes.far_plane = 100.0f;
    matrices.projectionMatrix = glm::scale(
        glm::perspective(glm::radians(74.0f), (float)viewPort.extent.width / (float)viewPort.extent.height, clip_planes.near_plane, clip_planes.far_plane),
        {1.0f, -1.0f, 1.0f}
    );
    matrices.viewMatrix = m_camera.get_view();
    matrices.modelMatrix = glm::identity<glm::mat4>();
    matrices.invProjectionMatrix = glm::inverse(matrices.projectionMatrix);
    matrices.invViewMatrix = glm::inverse(matrices.viewMatrix);
}

void ModelRenderer::render(
    uint32_t currentBuffer,
    VkSemaphore presentCompleteSemaphore,
    VkSemaphore renderCompleteSemaphore,
    VkRect2D renderArea,
    VkRect2D viewPort)
{
    assert(hasSwapchain);

    if (previousViewPort != viewPort) {
        // Setup a default look-at camera

        calculateMatrices(viewPort);

        // Values not set here are initialized in the base class constructor
        // Pass matrices to the shaders
        // clang-format off
        // auto uniform = Uniform{
        //     reflect<'x', 'y', 'z'>(projection_m),
        //     reflect<'x', 'Y', 'z'>(model_m),
        //     reflect<'x', 'y', 'Z'>(view_m)};
        // clang-format on
        //updateUniformBuffers(matrices);
    } else {
        matrices.viewMatrix = m_camera.get_view();
        matrices.invViewMatrix = glm::inverse(matrices.viewMatrix);
    }
    updateUniformBuffers(matrices, clip_planes);

    if (previousRenderArea != renderArea || previousViewPort != viewPort || models_changed) {
        buildCommandBuffers(renderArea, viewPort);
    }

    draw(currentBuffer, presentCompleteSemaphore, renderCompleteSemaphore);

    previousRenderArea = renderArea;
    previousViewPort = viewPort;
}

void ModelRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageViewType viewType) {
    VkCommandBuffer commandBuffer = getCommandBuffer(true);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = viewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } /*else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
        barrier.dstAccessMask = VK_ACCESS_NONE_KHR;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_
    }*/ else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    flushCommandBuffer(commandBuffer);
}

void ModelRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkImageViewType viewType) {
    VkCommandBuffer commandBuffer = getCommandBuffer(true);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = viewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    flushCommandBuffer(commandBuffer);
}

void ModelRenderer::createImage(
    uint32_t width, uint32_t height, VkImageViewType viewType,
    VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VmaMemoryUsage vmaUsage, VmaAllocation& allocation, VkImage& image
) {
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = viewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;
    imageCreateInfo.samples = numSamples;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = usage;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = nullptr;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.flags = viewType == VK_IMAGE_VIEW_TYPE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

    VmaAllocationCreateInfo imageAllocationCreateInfo = {};
    imageAllocationCreateInfo.usage = vmaUsage;

    VK_CHECK_RESULT(vmaCreateImage(
        allocator, &imageCreateInfo, &imageAllocationCreateInfo, &image, &allocation, nullptr));
}


void ModelRenderer::createImageView(VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlagBits aspect, VkImageView& imageView) {
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = viewType;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components = VkComponentMapping{};
    imageViewCreateInfo.subresourceRange = VkImageSubresourceRange{(uint32_t)aspect, 0, 1, 0, 1};

    VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView));
}