#include "TextureLoader.h"
#include "scene.h"

// Library Defines
#define STB_IMAGE_IMPLEMENTATION

// Library Includes
#include <stb_image.h>

//// Structs
struct STBImageData {
    STBImageData()
        : Width(0)
        , Height(0)
        , Components(0)
        , Data(nullptr)
    {
    }
    virtual ~STBImageData()
    {
        if (Data != nullptr) {
            stbi_image_free(Data);
        }
    }
    int Width;
    int Height;
    int Components;
    uint8_t* Data;
};

// Util Functions
std::unique_ptr<STBImageData> STBLoadTexture2D(const std::string& FullFilePath)
{
    // Load the Image data
    std::unique_ptr<STBImageData> ID { new STBImageData };
    // Vulkan coord origin is top-left so textures load the right way up, no need to flip vertically
    stbi_set_flip_vertically_on_load(false);
    ID->Data = stbi_load(FullFilePath.c_str(), &ID->Width, &ID->Height, &ID->Components, 0);

    if (ID->Data == nullptr) {
        std::string msg = "Failed to load texture: " + FullFilePath;
        ERR_EXIT(msg.c_str(), "Load Texture Failure");
    }

    return ID;
}

/// Copies loaded texture data (ID) to mapped texture memory (DestMemory), requires initialized texture_object to specify layout
/// If given texture data is RGB, expands data to RGBA in DestMemory using Alpha = 1.0
void CopyTextureDataToMemory(const STBImageData& ID, void* DestMemory, texture_object& TexObj, const vk::SubresourceLayout& layout)
{
    // This function supports RGB and RGBA textures only
    assert(ID.Components == 3 || ID.Components == 4);

    // Write bitmap data to allocated texture memory
    uint8_t* rgba_mem = static_cast<uint8_t*>(DestMemory);
    uint8_t* image_data = ID.Data;

    if (ID.Components == 3) {
        // Expand RGB to RGBA
        for (uint32_t y = 0; y < TexObj.tex_height; y++) {
            uint8_t* rgba_row = rgba_mem;
            for (uint32_t x = 0; x < TexObj.tex_width; x++) {
                // Copy RGB
                memcpy(rgba_row, image_data, 3);
                // Alpha is always 255
                rgba_row[3] = 255;
                rgba_row += 4;
                image_data += 3;
            }
            rgba_mem += layout.rowPitch;
        }
    } else if (ID.Components == 4) {
        for (uint32_t y = 0; y < TexObj.tex_height; y++) {
            // Copy RGBA
            memcpy(rgba_mem, image_data, 4 * TexObj.tex_width);
            image_data += 4 * TexObj.tex_width;
            rgba_mem += layout.rowPitch;
        }
    }
}

// Implementation: TextureLoader
void TextureLoader::CreateTexture2D(const std::string& FileName, texture_object& TexObj)
{
    CreateTexture2DEx(FileName, TexObj, vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

texture_object TextureLoader::CreateTexture2DBlank(uint32_t TexWidth, uint32_t TexHeight, vk::ImageTiling Tiling, vk::ImageUsageFlags Usage, vk::MemoryPropertyFlags RequiredProps)
{
    assert(GVulkanObjects.initialized);

    texture_object TexObj;
    TexObj.tex_width = TexWidth;
    TexObj.tex_height = TexHeight;

    auto const ImageCreateInfo = vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR8G8B8A8Unorm)
        .setExtent({ TexObj.tex_width, TexObj.tex_height, 1 })
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(Tiling)
        .setUsage(Usage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::ePreinitialized);

    auto Result = GVulkanObjects.device.createImage(&ImageCreateInfo, nullptr, &TexObj.image);
    VERIFY(Result == vk::Result::eSuccess);

    vk::MemoryRequirements mem_reqs;
    GVulkanObjects.device.getImageMemoryRequirements(TexObj.image, &mem_reqs);

    TexObj.mem_alloc.setAllocationSize(mem_reqs.size);
    TexObj.mem_alloc.setMemoryTypeIndex(0);

    auto pass = MemoryTypeFromProperties(mem_reqs.memoryTypeBits, RequiredProps, TexObj.mem_alloc.memoryTypeIndex);
    VERIFY(pass == true);

    Result = GVulkanObjects.device.allocateMemory(&TexObj.mem_alloc, nullptr, &TexObj.mem);
    VERIFY(Result == vk::Result::eSuccess);

    Result = GVulkanObjects.device.bindImageMemory(TexObj.image, TexObj.mem, 0);
    VERIFY(Result == vk::Result::eSuccess);

    TexObj.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    return TexObj;
}

void TextureLoader::CreateTexture2DEx(const std::string& FileName, texture_object& TexObj, vk::ImageTiling Tiling, vk::ImageUsageFlags Usage, vk::MemoryPropertyFlags RequiredProps)
{
    assert(GVulkanObjects.initialized);

	// TODO: check that file exists, throw exception if not

	std::string FullFilePath = PATH_TEXTURES;
	FullFilePath.append(FileName);

    // Load the Image data
    auto ID = STBLoadTexture2D(FullFilePath);

    TexObj = CreateTexture2DBlank(ID->Width, ID->Height, Tiling, Usage, RequiredProps);

    // Fill texture 2D with loaded texture data
    auto const subres = vk::ImageSubresource().setAspectMask(vk::ImageAspectFlagBits::eColor).setMipLevel(0).setArrayLayer(0);
    vk::SubresourceLayout layout;
    GVulkanObjects.device.getImageSubresourceLayout(TexObj.image, &subres, &layout);

    auto data = GVulkanObjects.device.mapMemory(TexObj.mem, 0, TexObj.mem_alloc.allocationSize);
    VERIFY(data.result == vk::Result::eSuccess);

    CopyTextureDataToMemory(*ID, data.value, TexObj, layout);

    GVulkanObjects.device.unmapMemory(TexObj.mem);

    TexObj.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
}


void TextureLoader::CreateBuffer2D(const std::string& FileName, texture_object& TexObj)
{
    assert(GVulkanObjects.initialized);

    std::string FullFilePath = PATH_TEXTURES;
    FullFilePath.append(FileName);

    // Load the Image data
    auto ID = STBLoadTexture2D(FullFilePath);

    TexObj.tex_width = ID->Width;
    TexObj.tex_height = ID->Height;

    auto const buffer_create_info = vk::BufferCreateInfo()
                                        .setSize(TexObj.tex_width * TexObj.tex_height * 4)
                                        .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
                                        .setSharingMode(vk::SharingMode::eExclusive);
    auto Result = GVulkanObjects.device.createBuffer(&buffer_create_info, nullptr, &TexObj.buffer);
    VERIFY(Result == vk::Result::eSuccess);

    vk::MemoryRequirements mem_reqs;
    GVulkanObjects.device.getBufferMemoryRequirements(TexObj.buffer, &mem_reqs);

    TexObj.mem_alloc.setAllocationSize(mem_reqs.size);
    TexObj.mem_alloc.setMemoryTypeIndex(0);

    vk::MemoryPropertyFlags requirements = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    auto pass = MemoryTypeFromProperties(mem_reqs.memoryTypeBits, requirements, TexObj.mem_alloc.memoryTypeIndex);
    VERIFY(pass == true);

    Result = GVulkanObjects.device.allocateMemory(&TexObj.mem_alloc, nullptr, &(TexObj.mem));
    VERIFY(Result == vk::Result::eSuccess);

    Result = GVulkanObjects.device.bindBufferMemory(TexObj.buffer, TexObj.mem, 0);
    VERIFY(Result == vk::Result::eSuccess);

    vk::SubresourceLayout layout;
    layout.rowPitch = TexObj.tex_width * 4;
    auto data = GVulkanObjects.device.mapMemory(TexObj.mem, 0, TexObj.mem_alloc.allocationSize);
    VERIFY(data.result == vk::Result::eSuccess);

    CopyTextureDataToMemory(*ID, data.value, TexObj, layout);

    GVulkanObjects.device.unmapMemory(TexObj.mem);
}

void TextureLoader::CreateOptimalTexture2DFromBuffer(texture_object& source_texture, texture_object& dest_texture)
{
    assert(GVulkanObjects.initialized);

    // Create destination texture 2D
    auto Tiling = vk::ImageTiling::eOptimal;
    auto Usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    auto RequiredProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
    dest_texture = CreateTexture2DBlank(source_texture.tex_width, source_texture.tex_height, Tiling, Usage, RequiredProps);

    SetImageLayout(dest_texture.image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized,
        vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits(), vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer);
 
    auto const subresource = vk::ImageSubresourceLayers()
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setMipLevel(0)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

    auto const copy_region = vk::BufferImageCopy()
        .setBufferOffset(0)
        .setBufferRowLength(source_texture.tex_width)
        .setBufferImageHeight(source_texture.tex_height)
        .setImageSubresource(subresource)
        .setImageOffset({ 0, 0, 0 })
        .setImageExtent({ source_texture.tex_width, source_texture.tex_height, 1 });
    
    // Dispatch copy command (async)
    GVulkanObjects.cmd.copyBufferToImage(source_texture.buffer, dest_texture.image, vk::ImageLayout::eTransferDstOptimal, 1, &copy_region);

    SetImageLayout(dest_texture.image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eTransferDstOptimal,
        dest_texture.imageLayout, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader);
}

void TextureLoader::SetImageLayout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask, vk::PipelineStageFlags src_stages, vk::PipelineStageFlags dest_stages)
{
    auto DstAccessMask = [](vk::ImageLayout const& layout) {
        vk::AccessFlags flags;

        switch (layout) {
        case vk::ImageLayout::eTransferDstOptimal:
            // Make sure anything that was copying from this image has
            // completed
            flags = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            flags = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            flags = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            // Make sure any Copy or CPU writes to image are flushed
            flags = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            flags = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            flags = vk::AccessFlagBits::eMemoryRead;
            break;
        default:
            break;
        }

        return flags;
    };

    GVulkanObjects.cmd.pipelineBarrier(src_stages, dest_stages, vk::DependencyFlagBits(), {}, {},
        vk::ImageMemoryBarrier()
            .setSrcAccessMask(srcAccessMask)
            .setDstAccessMask(DstAccessMask(newLayout))
            .setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image)
            .setSubresourceRange(vk::ImageSubresourceRange(aspectMask, 0, 1, 0, 1)));
}
