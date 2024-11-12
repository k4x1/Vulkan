#include "DemoScene.h"
#include "ShaderLoader.h"
#include "DemoCube.h"
#include "ModelLoader.h"

static uint32_t FindMemoryType(vk::PhysicalDevice PhysDevice, uint32_t TypeFilter, vk::MemoryPropertyFlags Properties)
{
    vk::PhysicalDeviceMemoryProperties MemProperties;
    PhysDevice.getMemoryProperties(&MemProperties);

    for (uint32_t i = 0; i < MemProperties.memoryTypeCount; i++) {
        if (TypeFilter & (1 << i) && (MemProperties.memoryTypes[i].propertyFlags & Properties) == Properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find a suitable memory type");
}

void DemoScene::cleanup_scene()
{
    device.destroyBuffer(vertex_buffer);
    device.freeMemory(vertex_buffer_memory);

    device.destroyBuffer(mesh_vertex_buffer);
    device.freeMemory(mesh_vertex_buffer_memory);
}

void DemoScene::init_scene()
{
    // Cube initialization
    vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
        .setSize(sizeof(VertexStandard) * demo_cube.size())
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer);

    auto result = device.createBuffer(&bufferInfo, nullptr, &vertex_buffer);
    VERIFY(result == vk::Result::eSuccess);

    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(vertex_buffer);
    vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(FindMemoryType(gpu, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

    result = device.allocateMemory(&allocInfo, nullptr, &vertex_buffer_memory);
    VERIFY(result == vk::Result::eSuccess);

    result = device.bindBufferMemory(vertex_buffer, vertex_buffer_memory, 0);
    VERIFY(result == vk::Result::eSuccess);

    void* VertexData;
    result = device.mapMemory(vertex_buffer_memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags(), &VertexData);
    VERIFY(result == vk::Result::eSuccess);

    memcpy(VertexData, demo_cube.data(), sizeof(VertexStandard) * demo_cube.size());
    device.unmapMemory(vertex_buffer_memory);

    // Load the model using ModelLoader
    std::vector<VertexStandard> meshVertices = ModelLoader::LoadModel("resources/Models/AncientEmpire/SM_Prop_Statue_01.obj", device, gpu, mesh_vertex_buffer, mesh_vertex_buffer_memory);
    this->meshVertices = meshVertices;
    mesh_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1000.0f, -1000.0f)); 
    // Setup scene data
    spin_speed = 40.0f;
    spin_control = 120.0f;

    projection_matrix = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);
    view_matrix = glm::lookAt(eye, origin, up);
    model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-0, -0, -500.0f));


    // Flip Y-coordinate to convert from OpenGL to Vulkan:
    projection_matrix[1][1] *= -1.0f;
}

void DemoScene::create_graphics_pipelines()
{
    auto attributeDescriptions = VertexStandard::GetAttributeDescriptions();
    auto bindingDescriptions = VertexStandard::GetBindingDescription();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescriptions;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    auto const inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo().setTopology(vk::PrimitiveTopology::eTriangleList);

    auto const viewportInfo = vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);

    auto const rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(VK_FALSE)
        .setRasterizerDiscardEnable(VK_FALSE)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setDepthBiasEnable(VK_FALSE)
        .setLineWidth(1.0f);

    auto const multisampleInfo = vk::PipelineMultisampleStateCreateInfo();

    auto const stencilOp = vk::StencilOpState().setFailOp(vk::StencilOp::eKeep).setPassOp(vk::StencilOp::eKeep).setCompareOp(vk::CompareOp::eAlways);

    auto const depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
        .setDepthTestEnable(VK_TRUE)
        .setDepthWriteEnable(VK_TRUE)
        .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
        .setDepthBoundsTestEnable(VK_FALSE)
        .setStencilTestEnable(VK_FALSE)
        .setFront(stencilOp)
        .setBack(stencilOp);

    std::array<vk::PipelineColorBlendAttachmentState, 1> const colorBlendAttachments = {
        vk::PipelineColorBlendAttachmentState().setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
    };

    auto const colorBlendInfo = vk::PipelineColorBlendStateCreateInfo().setAttachments(colorBlendAttachments);

    std::array<vk::DynamicState, 2> const dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    auto const dynamicStateInfo = vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamicStates);

    // PipelineCache allows pipeline construction to be reused between pipelines and app runs.
    // Pass the same pipeline cache when creating multiple related pipelines.
    vk::PipelineCacheCreateInfo const pipelineCacheInfo;
    auto result = device.createPipelineCache(&pipelineCacheInfo, nullptr, &pipelineCache);
    VERIFY(result == vk::Result::eSuccess);

    vk::ShaderModule vert_shader_module = ShaderLoader::CreateShader("textured.vert.spv");
    vk::ShaderModule frag_shader_module = ShaderLoader::CreateShader("textured.frag.spv");

    std::array<vk::PipelineShaderStageCreateInfo, 2> const shaderStageInfo = {
        vk::PipelineShaderStageCreateInfo().setStage(vk::ShaderStageFlagBits::eVertex).setModule(vert_shader_module).setPName("main"),
        vk::PipelineShaderStageCreateInfo().setStage(vk::ShaderStageFlagBits::eFragment).setModule(frag_shader_module).setPName("main")
    };

    auto pipline_return = device.createGraphicsPipelines(pipelineCache, vk::GraphicsPipelineCreateInfo().setStages(shaderStageInfo).setPVertexInputState(&vertexInputInfo).setPInputAssemblyState(&inputAssemblyInfo).setPViewportState(&viewportInfo).setPRasterizationState(&rasterizationInfo).setPMultisampleState(&multisampleInfo).setPDepthStencilState(&depthStencilInfo).setPColorBlendState(&colorBlendInfo).setPDynamicState(&dynamicStateInfo).setLayout(pipeline_layout).setRenderPass(render_pass));
    VERIFY(pipline_return.result == vk::Result::eSuccess);
    pipeline = pipline_return.value.at(0);

    device.destroyShaderModule(frag_shader_module);
    device.destroyShaderModule(vert_shader_module);
}

void DemoScene::populate_command_buffer(const vk::CommandBuffer& commandBuffer, const FrameResources& frame, uint32_t width, uint32_t height)
{
    vk::ClearValue const clearValues[2] = { vk::ClearColorValue(std::array<float, 4>({ { 0.2f, 0.2f, 0.2f, 0.2f } })),
        vk::ClearDepthStencilValue(1.0f, 0u) };

    commandBuffer.beginRenderPass(vk::RenderPassBeginInfo()
        .setRenderPass(render_pass)
        .setFramebuffer(frame.framebuffer)
        .setRenderArea(vk::Rect2D(vk::Offset2D{}, vk::Extent2D(width, height)))
        .setClearValueCount(2)
        .setPClearValues(clearValues),
        vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    // Bind descriptor sets
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, frame.descriptor_set, {});

    // Set viewport
    commandBuffer.setViewport(0, vk::Viewport().setX(0.0f).setY(0.0f).setWidth(static_cast<float>(width)).setHeight(static_cast<float>(height)).setMinDepth(0.0f).setMaxDepth(1.0f));

    // Set scissor
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D{}, vk::Extent2D(width, height)));

    // Draw the cube
    vk::Buffer cubeVertexBuffers[] = { vertex_buffer };
    vk::DeviceSize cubeOffsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, cubeVertexBuffers, cubeOffsets);
    commandBuffer.draw(static_cast<uint32_t>(demo_cube.size()), 1, 0, 0);

    // Update uniform data for the mesh
    uniform_data.model = mesh_model_matrix;
    uniform_data.viewproj = projection_matrix * view_matrix;
    memcpy(frame.uniform_memory_ptr, &uniform_data, sizeof(UBO_Textured));

    // Draw the loaded mesh
    vk::Buffer meshVertexBuffers[] = { mesh_vertex_buffer };
    vk::DeviceSize meshOffsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, meshVertexBuffers, meshOffsets);
    commandBuffer.draw(static_cast<uint32_t>(meshVertices.size()), 1, 0, 0);

    commandBuffer.endRenderPass();
}

std::pair<void*, size_t> DemoScene::create_uniform_data()
{
    uniform_data.model = model_matrix;
    uniform_data.viewproj = projection_matrix * view_matrix;

    UBO_Textured mesh_uniform_data;
    mesh_uniform_data.model = mesh_model_matrix;
    mesh_uniform_data.viewproj = projection_matrix * view_matrix;

    return std::make_pair(&uniform_data, sizeof uniform_data);
}

void DemoScene::new_frame() {
    controls.on_new_frame();
}

void DemoScene::update(float dt, void* uniform_memory_ptr)
{
    // Process input
    if (glfwGetKey(window_handle, GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window_handle, true);
    }
    auto update_control = [this](int& out_control_state, int keycode) {
        if (glfwGetKey(this->window_handle, keycode) == GLFW_PRESS) {
            out_control_state++;
        }
        else if (out_control_state > 0) {
            out_control_state = -1;
        }
        };
    update_control(controls.left, GLFW_KEY_LEFT);
    update_control(controls.right, GLFW_KEY_RIGHT);
    update_control(controls.space, GLFW_KEY_SPACE);

    // Handle inputs
    if (controls.space == -1) {
        pause = !pause;
    }
    if (!pause) {
        if (controls.left > 0) {
            spin_speed -= spin_control * dt;
        }
        if (controls.right > 0) {
            spin_speed += spin_control * dt;
        }
    }

    // Recalculate projection matrix to handle window resize
    projection_matrix = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 10000.0f);
    // GLM projection is OpenGL format, flip Y to convert to Vulkan
    projection_matrix[1][1] *= -1.0f;

    glm::mat4 VP = projection_matrix * view_matrix;

    // Rotate around the Y axis
    model_matrix = glm::rotate(model_matrix, glm::radians(spin_speed) * dt, glm::vec3(0.0f, 1.0f, 0.0f));

    // Orthonormalize rotation part of matrix (optional)
    glm::vec3 xAxis = glm::normalize(glm::vec3(model_matrix[0]));
    glm::vec3 yAxis = glm::normalize(glm::vec3(model_matrix[1]));
    glm::vec3 zAxis = glm::normalize(glm::vec3(model_matrix[2]));

    model_matrix[0] = glm::vec4(xAxis, 0.0f);
    model_matrix[1] = glm::vec4(yAxis, 0.0f);
    model_matrix[2] = glm::vec4(zAxis, 0.0f);

    uniform_data.model = model_matrix;
    uniform_data.viewproj = VP;

    // Update mapped memory with uniform_data
    memcpy(uniform_memory_ptr, &uniform_data, sizeof(UBO_Textured));
}