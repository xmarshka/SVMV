#pragma once

#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.hpp>

#include <fstream>
#include <sstream>

namespace SVMV
{
    class VulkanShader
    {
    public:
        enum class ShaderType
        {
            UNINITIALIZED, VERTEX, FRAGMENT
        };

    public:
        vk::ShaderModule shader;
        ShaderType type;

    private:
        vk::Device _device;
        shaderc::Compiler _shaderCompiler;

    public:
        VulkanShader();
        VulkanShader(vk::Device device, ShaderType type, const std::string& file);

        ~VulkanShader();

    private:
        std::string readFile(const std::string& file);

        std::vector<uint32_t> compileShader(const std::string& name, const std::string& shaderCode, shaderc_shader_kind shaderKind, bool optimize);
        void createShaderModule(const std::vector<uint32_t> code);

        shaderc_shader_kind convertShaderType(ShaderType type);
    };
}