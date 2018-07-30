#pragma once

#include "../../vRt_subimpl.inl"

namespace _vt {
    using namespace vrt;

    // destructor of DeviceImage
    inline DeviceImage::~DeviceImage() {
        VRT_ASYNC([=]() {
            vmaDestroyImage(_device->_allocator, _image, _allocation);
        });
    };

    static inline VtResult createDeviceImage(std::shared_ptr<Device> device, const VtDeviceImageCreateInfo& cinfo, std::shared_ptr<DeviceImage>& _vtImage) {
        // result will no fully handled
        VtResult result = VK_ERROR_INITIALIZATION_FAILED;

        auto vtDeviceImage = (_vtImage = std::make_shared<DeviceImage>());
        vtDeviceImage->_device = device; // delegate device by weak_ptr
        vtDeviceImage->_layout = (VkImageLayout)cinfo.layout;

        // init image dimensional type
        vk::ImageType imageType = vk::ImageType::e2D; bool isCubemap = false;
        switch (vk::ImageViewType(cinfo.imageViewType)) {
            case vk::ImageViewType::e1D: imageType = vk::ImageType::e1D; break;
            case vk::ImageViewType::e1DArray: imageType = vk::ImageType::e2D; break;
            case vk::ImageViewType::e2D: imageType = vk::ImageType::e2D; break;
            case vk::ImageViewType::e2DArray: imageType = vk::ImageType::e3D; break;
            case vk::ImageViewType::e3D: imageType = vk::ImageType::e3D; break;
            case vk::ImageViewType::eCube: imageType = vk::ImageType::e3D; isCubemap = true; break;
            case vk::ImageViewType::eCubeArray: imageType = vk::ImageType::e3D; isCubemap = true; break;
        };

        // additional usage
        auto usage = vk::ImageUsageFlags(cinfo.usage) | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;


        // image memory descriptor
#ifdef VRT_ENABLE_VEZ_INTEROP
        auto imageInfo = VezImageCreateInfo{};
#else
        auto imageInfo = VkImageCreateInfo(vk::ImageCreateInfo{});
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.initialLayout = VkImageLayout(vtDeviceImage->_initialLayout);
        imageInfo.sharingMode = VkSharingMode(vk::SharingMode::eExclusive);

#endif


        // unified create structure
        imageInfo.pNext = nullptr;
        imageInfo.imageType = VkImageType(imageType);
        imageInfo.arrayLayers = 1; // unsupported
        imageInfo.tiling = VkImageTiling(vk::ImageTiling::eOptimal);
        imageInfo.extent = VkExtent3D{ cinfo.size.width, cinfo.size.height, cinfo.size.depth * (isCubemap ? 6 : 1) };
        imageInfo.format = cinfo.format;
        imageInfo.mipLevels = cinfo.mipLevels;
        imageInfo.pQueueFamilyIndices = &cinfo.familyIndex;
        imageInfo.queueFamilyIndexCount = 1;
        imageInfo.samples = VkSampleCountFlagBits(vk::SampleCountFlagBits::e1); // at now not supported MSAA
        imageInfo.usage = VkImageUsageFlags(usage);


        // create image with allocation
#ifdef VRT_ENABLE_VEZ_INTEROP
        if ( vezCreateImage(device->_device, VEZ_MEMORY_GPU_ONLY, &imageInfo, &(vtDeviceImage->_image)) == VK_SUCCESS ) { result = VK_SUCCESS; };
#else
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        if ( vmaCreateImage(device->_allocator, &imageInfo, &allocCreateInfo, &(vtDeviceImage->_image), &vtDeviceImage->_allocation, &vtDeviceImage->_allocationInfo) == VK_SUCCESS ) { result = VK_SUCCESS; };
#endif


        // subresource range
        vtDeviceImage->_subresourceRange.levelCount = 1;
        vtDeviceImage->_subresourceRange.layerCount = 1;
        vtDeviceImage->_subresourceRange.baseMipLevel = 0;
        vtDeviceImage->_subresourceRange.baseArrayLayer = 0;
        vtDeviceImage->_subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        // subresource layers
        vtDeviceImage->_subresourceLayers.layerCount = vtDeviceImage->_subresourceRange.layerCount;
        vtDeviceImage->_subresourceLayers.baseArrayLayer = vtDeviceImage->_subresourceRange.baseArrayLayer;
        vtDeviceImage->_subresourceLayers.aspectMask = vtDeviceImage->_subresourceRange.aspectMask;
        vtDeviceImage->_subresourceLayers.mipLevel = vtDeviceImage->_subresourceRange.baseMipLevel;

        vtDeviceImage->_extent = imageInfo.extent;

        // descriptor for usage
        // TODO to add support for V-EZ
        // (unhandled by vtResult)
        vtDeviceImage->_imageView = vk::Device(device->_device).createImageView(vk::ImageViewCreateInfo()
            .setSubresourceRange(vtDeviceImage->_subresourceRange)
            .setViewType(vk::ImageViewType(cinfo.imageViewType))
            .setComponents(vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA))
            .setImage(vtDeviceImage->_image)
            .setFormat(vk::Format(cinfo.format)));

#ifdef VRT_ENABLE_VEZ_INTEROP
        vtDeviceImage->_initialLayout = vtDeviceImage->_layout;
#else
        vtDeviceImage->_staticDsci = VkDescriptorImageInfo{ {}, vtDeviceImage->_imageView, vtDeviceImage->_layout };
#endif
        return result;
    };

};
