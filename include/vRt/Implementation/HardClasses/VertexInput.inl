#pragma once

#include "../../vRt_subimpl.inl"

namespace _vt {
    using namespace vt;

    // TODO - add support for auto-creation of buffers in "VtVertexInputCreateInfo" from pointers and counts
    // also, planned to add support of offsets in buffers 
    inline VtResult createVertexInputSet(std::shared_ptr<Device> _vtDevice, VtVertexInputCreateInfo& info, std::shared_ptr<VertexInputSet>& _vtVertexInput) {
        VtResult result = VK_SUCCESS;
        auto& vtVertexInput = (_vtVertexInput = std::make_shared<VertexInputSet>());
        vtVertexInput->_device = _vtDevice;

        std::vector<vk::PushConstantRange> constRanges = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eCompute, 0u, strided<uint32_t>(4))
        };
        std::vector<vk::DescriptorSetLayout> dsLayouts = {
            vk::DescriptorSetLayout(_vtDevice->_descriptorLayoutMap["vertexInputSet"]),
        };
        auto dsc = vk::Device(*_vtDevice).allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(_vtDevice->_descriptorPool).setPSetLayouts(&dsLayouts[0]).setDescriptorSetCount(1));
        vtVertexInput->_descriptorSet = dsc[0];

        // 
        VtDeviceBufferCreateInfo bfi;
        bfi.familyIndex = _vtDevice->_mainFamilyIndex;
        bfi.usageFlag = VkBufferUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer);
        bfi.bufferSize = sizeof(uint32_t) * 8;
        bfi.format = VK_FORMAT_UNDEFINED;

        // planned add external buffer support
        createDeviceBuffer(_vtDevice, bfi, vtVertexInput->_uniformBlockBuffer);
        _vtDevice->_deviceBuffersPtrs.push_back(vtVertexInput->_uniformBlockBuffer); // pin buffer with device

                                                                                     // set primitive count (will loaded to "_uniformBlockBuffer" by cmdUpdateBuffer)
        vtVertexInput->_uniformBlock.primitiveCount = info.primitiveCount;
        vtVertexInput->_uniformBlock.verticeAccessor = info.verticeAccessor;
        vtVertexInput->_uniformBlock.indiceAccessor = info.indiceAccessor;
        vtVertexInput->_uniformBlock.materialID = info.materialID;

        // write descriptors
        auto _write_tmpl = vk::WriteDescriptorSet(vtVertexInput->_descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer);
        std::vector<vk::WriteDescriptorSet> writes = {
            vk::WriteDescriptorSet(_write_tmpl).setDstBinding(0).setDescriptorType(vk::DescriptorType::eUniformTexelBuffer).setPTexelBufferView(&vk::BufferView(vtVertexInput->_dataSourceBuffer->_bufferView)),
            vk::WriteDescriptorSet(_write_tmpl).setDstBinding(1).setPBufferInfo(&vk::DescriptorBufferInfo(vtVertexInput->_bBufferRegionBindings->_descriptorInfo())),
            vk::WriteDescriptorSet(_write_tmpl).setDstBinding(2).setPBufferInfo(&vk::DescriptorBufferInfo(vtVertexInput->_bBufferViews->_descriptorInfo())),
            vk::WriteDescriptorSet(_write_tmpl).setDstBinding(3).setPBufferInfo(&vk::DescriptorBufferInfo(vtVertexInput->_bBufferAccessors->_descriptorInfo())),
            vk::WriteDescriptorSet(_write_tmpl).setDstBinding(4).setPBufferInfo(&vk::DescriptorBufferInfo(vtVertexInput->_bBufferAttributeBindings->_descriptorInfo())),
            vk::WriteDescriptorSet(_write_tmpl).setDstBinding(5).setPBufferInfo(&vk::DescriptorBufferInfo(vtVertexInput->_uniformBlockBuffer->_descriptorInfo())),
        };
        vk::Device(*_vtDevice).updateDescriptorSets(_write_tmpl, {});

        return result;
    };



};