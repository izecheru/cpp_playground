#include "vulkan_pipeline/vulkan_pipeline.hpp"
#include <fstream>
#include <string>
#include "vulkan_device/vulkan_device.hpp"

auto createShaderModule( VkDevice& device, const std::vector<char>& code ) -> VkShaderModule
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>( code.data() );
  VkShaderModule shaderModule;
  if ( vkCreateShaderModule( device, &createInfo, nullptr, &shaderModule ) )
  {
    throw std::runtime_error( "failed to create shader module" );
  }
  return shaderModule;
}

static auto readFile( const std::string& filePath ) -> std::vector<char>
{
  std::ifstream file( filePath, std::ios::ate | std::ios::binary );

  if ( !file.is_open() )
  {
    throw std::runtime_error( "failed to open file!\n" );
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer( fileSize );
  file.seekg( 0 );
  file.read( buffer.data(), fileSize );
  file.close();

  return buffer;
}

VulkanPipeline::VulkanPipeline( const VulkanPipelineSpecification& spec )
{
  // for ( auto& shaderPath : spec.shaders )
  //{
  //   auto content = readFile( shaderPath.path );
  //   VkShaderStageFlagBits type{ VK_SHADER_STAGE_VERTEX_BIT };
  //   if ( shaderPath.find( "frag" ) != std::string::npos )
  //   {
  //     type = VK_SHADER_STAGE_FRAGMENT_BIT;
  //   }
  //   m_shaderStages.push_back(
  //     ShaderStage{ .type = type, .shader = createShaderModule( VulkanDevice::getLogicalDevice(), content ) } );
  // }

  // std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  // for ( auto& shaderModule : m_shaderStages )
  //{
  //   VkPipelineShaderStageCreateInfo shaderStageInfo{};
  //   shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  //   shaderStageInfo.stage = shaderModule.type;
  //   shaderStageInfo.module = shaderModule.shader;
  //   shaderStageInfo.pName = "main";
  //   shaderStages.push_back( shaderStageInfo );
  // }

  // VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  // vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  // vertexInputInfo.vertexBindingDescriptionCount = 0;
  // vertexInputInfo.vertexAttributeDescriptionCount = 0;

  // VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  // inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  // inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  // inputAssembly.primitiveRestartEnable = VK_FALSE;

  // VkPipelineViewportStateCreateInfo viewportState{};
  // viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  // viewportState.viewportCount = 1;
  // viewportState.scissorCount = 1;

  // VkPipelineRasterizationStateCreateInfo rasterizer{};
  // rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // rasterizer.depthClampEnable = VK_FALSE;
  // rasterizer.rasterizerDiscardEnable = VK_FALSE;
  // rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  // rasterizer.lineWidth = 1.0f;
  // rasterizer.cullMode = VK_CULL_MODE_NONE;
  // rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  // rasterizer.depthBiasEnable = VK_FALSE;

  // VkPipelineMultisampleStateCreateInfo multisampling{};
  // multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  // multisampling.sampleShadingEnable = VK_FALSE;
  // multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  // colorBlendAttachment.colorWriteMask =
  //   VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  // colorBlendAttachment.blendEnable = VK_FALSE;

  // VkPipelineColorBlendStateCreateInfo colorBlending{};
  // colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  // colorBlending.logicOpEnable = VK_FALSE;
  // colorBlending.logicOp = VK_LOGIC_OP_COPY;
  // colorBlending.attachmentCount = 1;
  // colorBlending.pAttachments = &colorBlendAttachment;
  // colorBlending.blendConstants[0] = 0.0f;
  // colorBlending.blendConstants[1] = 0.0f;
  // colorBlending.blendConstants[2] = 0.0f;
  // colorBlending.blendConstants[3] = 0.0f;

  // std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  // VkPipelineDynamicStateCreateInfo dynamicState{};
  // dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  // dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
  // dynamicState.pDynamicStates = dynamicStates.data();

  // VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  // pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  // pipelineLayoutInfo.setLayoutCount = 0;
  // pipelineLayoutInfo.pushConstantRangeCount = 0;

  // if ( vkCreatePipelineLayout( VulkanDevice::getLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_layout ) !=
  //      VK_SUCCESS )
  //{
  //   throw std::runtime_error( "failed to create pipeline layout!" );
  // }

  // VkGraphicsPipelineCreateInfo pipelineInfo{};
  // pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  // pipelineInfo.stageCount = 2;
  // pipelineInfo.pStages = shaderStages.data();
  // pipelineInfo.pVertexInputState = &vertexInputInfo;
  // pipelineInfo.pInputAssemblyState = &inputAssembly;
  // pipelineInfo.pViewportState = &viewportState;
  // pipelineInfo.pRasterizationState = &rasterizer;
  // pipelineInfo.pMultisampleState = &multisampling;
  // pipelineInfo.pColorBlendState = &colorBlending;
  // pipelineInfo.pDynamicState = &dynamicState;
  // pipelineInfo.layout = m_layout;
  // pipelineInfo.renderPass = nullptr;
  // pipelineInfo.subpass = 0;
  // pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  // if ( vkCreateGraphicsPipelines(
  //        VulkanDevice::getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline ) !=
  //      VK_SUCCESS )
  //{
  //   throw std::runtime_error( "failed to create graphics pipeline!" );
  // }
  // for ( auto& m : m_shaderStages )
  //   vkDestroyShaderModule( VulkanDevice::getLogicalDevice(), m.shader, nullptr );
}

VulkanPipeline::~VulkanPipeline()
{
}
