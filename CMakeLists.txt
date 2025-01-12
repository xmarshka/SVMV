cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)

project(SVMV DESCRIPTION "Simple glTF model viewer" LANGUAGES CXX)

set(HEADER_DIR ${CMAKE_CURRENT_LIST_DIR}/src/${PROJECT_NAME})
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src/${PROJECT_NAME})

set(SVMV_INCLUDES
	${SRC_DIR}/Application.hxx
	${SRC_DIR}/GLFWwindowWrapper.hxx
	${SRC_DIR}/VulkanInitialization.hxx
	${SRC_DIR}/VulkanRenderer.hxx
	${SRC_DIR}/VulkanShader.hxx
	${SRC_DIR}/VulkanScene.hxx
	${SRC_DIR}/VulkanMaterialContext.hxx
	${SRC_DIR}/VulkanDrawable.hxx
	${SRC_DIR}/VulkanBuffer.hxx
	${SRC_DIR}/VulkanImage.hxx
	${SRC_DIR}/VulkanMaterial.hxx
	${SRC_DIR}/VulkanShaderStructures.hxx
	${SRC_DIR}/VulkanGLTFPBRMaterial.hxx
	${SRC_DIR}/VulkanDescriptorWriter.hxx
	${SRC_DIR}/VulkanUtilities.hxx
	${SRC_DIR}/Loader.hxx
	${SRC_DIR}/Scene.hxx
	${SRC_DIR}/Node.hxx
	${SRC_DIR}/Mesh.hxx
	${SRC_DIR}/Primitive.hxx
	${SRC_DIR}/Attribute.hxx
	${SRC_DIR}/Material.hxx
	${SRC_DIR}/Property.hxx
	${SRC_DIR}/Texture.hxx
	${SRC_DIR}/Input.hxx
	${SRC_DIR}/InputHandler.hxx
	${SRC_DIR}/CameraController.hxx)

set(SVMV_SOURCES
	${SRC_DIR}/main.cxx
	${SRC_DIR}/VmaUsage.cxx
	${SRC_DIR}/Application.cxx
	${SRC_DIR}/GLFWwindowWrapper.cxx
	${SRC_DIR}/VulkanInitialization.cxx
	${SRC_DIR}/VulkanRenderer.cxx
	${SRC_DIR}/VulkanShader.cxx
	${SRC_DIR}/VulkanBuffer.cxx
	${SRC_DIR}/VulkanImage.cxx
	${SRC_DIR}/VulkanGLTFPBRMaterial.cxx
	${SRC_DIR}/VulkanDescriptorWriter.cxx
	${SRC_DIR}/VulkanUtilities.cxx
	${SRC_DIR}/Loader.cxx
	${SRC_DIR}/Input.cxx
	${SRC_DIR}/InputHandler.cxx
	${SRC_DIR}/CameraController.cxx)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(tinygltf REQUIRED)

add_executable(${PROJECT_NAME}
	${SVMV_INCLUDES}
	${SVMV_SOURCES})

target_include_directories(${PROJECT_NAME}
	PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(${PROJECT_NAME}
	PUBLIC Vulkan::Vulkan
	PUBLIC Vulkan::shaderc_combined
	PUBLIC GPUOpen::VulkanMemoryAllocator
	PUBLIC glm::glm
	PUBLIC glfw
	PUBLIC vk-bootstrap::vk-bootstrap
	PUBLIC tinygltf::tinygltf)

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS "RESOURCE_DIR=\"${CMAKE_CURRENT_LIST_DIR}/res\"")