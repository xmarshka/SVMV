#include <SVMV/VulkanShader.hxx>

using namespace SVMV;

VulkanShader::VulkanShader() : type(ShaderType::UNINITIALIZED), shader(nullptr), _device(nullptr)
{
}

VulkanShader::VulkanShader(vk::Device device, ShaderType shaderType, const std::string& file) : _device(device)
{
    auto shaderCode = readFile(RESOURCE_DIR"/shaders/" + file);

    type = shaderType;

    createShaderModule(compileShader(file, shaderCode, convertShaderType(shaderType), false));
}

VulkanShader::~VulkanShader()
{
    _device.destroyShaderModule(shader);
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

std::vector<uint32_t> VulkanShader::compileShader(const std::string& name, const std::string& shaderCode, shaderc_shader_kind shaderKind, bool optimize)
{
    shaderc::CompileOptions options;

    options.SetSourceLanguage(shaderc_source_language_glsl);

    shaderc::SpvCompilationResult result = _shaderCompiler.CompileGlslToSpv(shaderCode, shaderKind, name.c_str());

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error("shaderc: failed to compile shader: " + result.GetErrorMessage());
    }

    return std::vector<uint32_t>(result.cbegin(), result.cend());
}

void VulkanShader::createShaderModule(const std::vector<uint32_t> code)
{
    vk::ShaderModuleCreateInfo info = {};
    info.sType = vk::StructureType::eShaderModuleCreateInfo;
    info.codeSize = code.size() * sizeof(uint32_t);
    info.pCode = code.data();

    shader = _device.createShaderModule(info);
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
    default:
        throw std::runtime_error("shader: failed to convert shader type");
        break;
    }
}
