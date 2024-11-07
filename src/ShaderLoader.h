#pragma once

#include "common.h"

class ShaderLoader
{
public:
	static const vk::ShaderModule CreateShader(const std::string& FileName);
	static void DestroyShader(vk::ShaderModule& ShaderModule);

private:
	ShaderLoader();
	~ShaderLoader();
};
