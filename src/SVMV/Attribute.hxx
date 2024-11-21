//#pragma once
//
//#include <memory>
//#include <vector>
//
//namespace SVMV
//{
//    class Attribute
//    {
//    public:
//        enum class AttributeType
//        {
//            POSITION, COLOR, TEXCOORD, NORMAL, TANGENT, BINORMAL, _COUNT
//        };
//
//    public:
//        std::vector<uint8_t> data;
//        size_t count;
//        size_t components;
//        size_t padding;
//        size_t componentSize; // in bytes
//
//        AttributeType type;
//
//        Attribute(size_t count, size_t components, size_t padding, size_t componentSize, AttributeType type) : count(count), components(components), padding(padding), componentSize(componentSize), type(type) {}
//    };
//}

//TODO: unused