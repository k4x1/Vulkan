#include "ShaderLoader.h"

ShaderLoader::ShaderLoader() {}
ShaderLoader::~ShaderLoader() {}

std::vector<char> LoadShaderFile(const std::string& FullFilePath)
{
	std::ifstream File(FullFilePath, std::ios::ate | std::ios::binary);
	if (!File.is_open())
	{
		std::string msg = "Failed to open shader file: " + FullFilePath;
		ERR_EXIT(FullFilePath.c_str(), "Create Shader Failed");
	}

	size_t FileSize = static_cast<size_t>(File.tellg());
	std::vector<char> Buffer(FileSize);
	File.seekg(0);
	File.read(Buffer.data(), FileSize);

	File.close();
	return Buffer;
}

const vk::ShaderModule ShaderLoader::CreateShader(const std::string& FileName)
{
	assert(GVulkanObjects.initialized);
	
	std::string FullFilePath = PATH_SHADERS;
	FullFilePath.append(FileName);	
	auto ShaderData = LoadShaderFile(FullFilePath);

	const auto ShaderModuleReturn = GVulkanObjects.device.createShaderModule(vk::ShaderModuleCreateInfo().setCodeSize(ShaderData.size()).setPCode(reinterpret_cast<const uint32_t*>(ShaderData.data())));
	VERIFY(ShaderModuleReturn.result == vk::Result::eSuccess);

	return ShaderModuleReturn.value;
}

void ShaderLoader::DestroyShader(vk::ShaderModule& ShaderModule)
{
	GVulkanObjects.device.destroyShaderModule(ShaderModule);
}