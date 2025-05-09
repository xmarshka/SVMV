#include <SVMV/VulkanShader.hxx>

using namespace SVMV;

VulkanShader::VulkanShader(const vk::raii::Device& device, const shaderc::Compiler& shaderCompiler, ShaderType type, const std::string& file)
{
    auto shaderCode = readFile(RESOURCE_DIR"/shaders/" + file);

    _type = type;

    createShaderModule(device, compileShader(shaderCompiler, file, shaderCode, convertShaderType(_type), false));
}

VulkanShader::VulkanShader(VulkanShader&& other) noexcept
{
    this->_module = std::move(other._module);
    this->_type = other._type;

    other._type = ShaderType::UNDEFINED;
}

VulkanShader& VulkanShader::operator=(VulkanShader&& other) noexcept
{
    if (this != &other)
    {
        this->_module = std::move(other._module);
        this->_type = other._type;

        other._type = ShaderType::UNDEFINED;
    }

    return *this;
}

const vk::raii::ShaderModule& VulkanShader::getModule() const noexcept
{
    return _module;
}

VulkanShader::ShaderType VulkanShader::getType() const noexcept
{
    return _type;
}

std::string VulkanShader::readFile(const std::string& file)
{
    std::stringstream output;
    std::string line;

    std::ifstream fileStream(file);

    if (!fileStream.is_open())
    {
        throw std::runtime_error("ifstream: failed to open file: " + file);
    }

    while (std::getline(fileStream, line))
    {
        output << line;
        output << '\n';
    }

    return output.str();
}

std::vector<uint32_t> VulkanShader::compileShader(const shaderc::Compiler& shaderCompiler, const std::string& name, const std::string& shaderCode, shaderc_shader_kind shaderKind, bool optimize)
{
    shaderc::CompileOptions options;

    options.SetSourceLanguage(shaderc_source_language_glsl);

    shaderc::SpvCompilationResult result = shaderCompiler.CompileGlslToSpv(shaderCode, shaderKind, name.c_str());

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error("shaderc: failed to compile shader: " + result.GetErrorMessage());
    }

    return std::vector<uint32_t>(result.cbegin(), result.cend());
}

void VulkanShader::createShaderModule(const vk::raii::Device& device, const std::vector<uint32_t> code)
{
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.setCodeSize(code.size() * sizeof(uint32_t));
    shaderModuleCreateInfo.setCode(code);

    _module = vk::raii::ShaderModule(device, shaderModuleCreateInfo);
}

shaderc_shader_kind VulkanShader::convertShaderType(ShaderType type)
{
    switch (type)
    {
    case ShaderType::VERTEX:
        return shaderc_shader_kind::shaderc_glsl_vertex_shader;
        break;
    case ShaderType::FRAGMENT:
        return shaderc_shader_kind::shaderc_glsl_fragment_shader;
        break;
    case ShaderType::COMPUTE:
        return shaderc_shader_kind::shaderc_glsl_compute_shader;
        break;
    default:
        throw std::runtime_error("shader: failed to convert shader type");
        break;
    }
}
