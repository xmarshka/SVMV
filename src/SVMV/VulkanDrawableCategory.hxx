#pragma once

namespace SVMV
{
    struct VulkanDrawableCategory
    {
        enum class ShaderCategory
        {
            FLAT, _COUNT
        };

        ShaderCategory shaderCategory;
        uint8_t vertexAttributeCategory; // bitfield, where each bit represents a vertex attribute (in order as defined in the Attribute class, starting at LSB, except INDEX) contained in a primitive

        bool operator==(const VulkanDrawableCategory& other) const
        {
            return (shaderCategory == other.shaderCategory) && (vertexAttributeCategory == other.vertexAttributeCategory);
        }
    };
}

template <>
struct std::hash<SVMV::VulkanDrawableCategory>
{
    std::size_t operator()(const SVMV::VulkanDrawableCategory& k) const
    {
        using std::size_t;
        using std::hash;
        using std::string;

        return ((hash<uint32_t>()(static_cast<uint32_t>(k.shaderCategory))
            ^ (hash<uint8_t>()(k.vertexAttributeCategory) << 1)) >> 1);
    }
};
