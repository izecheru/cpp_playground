#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct ShaderStage
{
  VkShaderStageFlagBits type;
  VkShaderModule shader{ VK_NULL_HANDLE };
};

/**
 * @brief Struct for vertex attribs
 */
struct VertexPipelineSpecification
{
  uint32_t binding;
  uint32_t location;
  uint32_t format;
  uint32_t offset;
};

struct ShaderPipelineSpeficiation
{
  VertexPipelineSpecification vertexSpec;
  std::string path;
};

struct VulkanPipelineSpecification
{
  std::vector<ShaderPipelineSpeficiation> shaders;
};

class VulkanPipeline
{
public:
  VulkanPipeline( const VulkanPipelineSpecification& spec );
  ~VulkanPipeline();

private:
  std::vector<ShaderStage> m_shaderStages;
  VkPipelineLayout m_layout;
  VkPipeline m_graphicsPipeline;
};