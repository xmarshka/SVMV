#pragma once

#include <vulkan/vulkan_raii.hpp>
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
            UNDEFINED, VERTEX, FRAGMENT, COMPUTE
        };

    public:
        VulkanShader() = default;
        VulkanShader(const vk::raii::Device& device, const shaderc::Compiler& shaderCompiler, ShaderType type, const std::string& file);

        VulkanShader(const VulkanShader&) = delete;
        VulkanShader& operator=(const VulkanShader&) = delete;

        VulkanShader(VulkanShader&& other) noexcept;
        VulkanShader& operator=(VulkanShader&& other) noexcept;

        ~VulkanShader() = default;

        [[nodiscard]] const vk::raii::ShaderModule& getModule() const noexcept;
        [[nodiscard]] ShaderType getType() const noexcept;

    private:
        std::string readFile(const std::string& file);

        std::vector<uint32_t> compileShader(const shaderc::Compiler& shaderCompiler, const std::string& name, const std::string& shaderCode, shaderc_shader_kind shaderKind, bool optimize);
        void createShaderModule(const vk::raii::Device& device, const std::vector<uint32_t> code);

        shaderc_shader_kind convertShaderType(ShaderType type);

    private:
        vk::raii::ShaderModule _module{ nullptr };
        ShaderType _type{ ShaderType::UNDEFINED };
    };
}