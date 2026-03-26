/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "vk_test.h"

std::string pixel = R"EOSHADER(

#version 460 core

struct v2f
{
	vec4 pos;
	vec4 col;
	vec4 uv;
};

layout(location = 0) in v2f vertIn;
layout(location = 0, index = 0) out vec4 Color;
layout(binding = 2) uniform sampler2D inTex;

void main()
{
	Color = vertIn.col;
  Color += texture(inTex, vertIn.uv.xy);
}

)EOSHADER";

const std::string compute = R"EOSHADER(

#version 450 core

layout(binding = 0) uniform inbuftype {
  uvec4 data[];
} inbuf;

layout(binding = 1, std430) buffer outbuftype {
  uvec4 data[];
} outbuf;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void main()
{
  outbuf.data[0] += inbuf.data[0];
}

)EOSHADER";

RD_TEST(VK_Resource_Usage, VulkanGraphicsTest)
{
  static constexpr const char *Description = "Test resource usage in a variety of scenarios.";

  VkPhysicalDeviceNestedCommandBufferFeaturesEXT nestedFeats = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NESTED_COMMAND_BUFFER_FEATURES_EXT,
  };

  VkPhysicalDeviceNestedCommandBufferPropertiesEXT nestedProps = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NESTED_COMMAND_BUFFER_PROPERTIES_EXT,
  };

  VkPhysicalDeviceDescriptorBufferFeaturesEXT descBufFeats = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
  };

  VkPhysicalDeviceDescriptorBufferPropertiesEXT descBufProps = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT,
  };

  float sqSize;
  VkViewport viewPort;

  void NextTest()
  {
    viewPort.x += sqSize;

    if(viewPort.x + sqSize >= (float)screenWidth)
    {
      viewPort.x = 0.0f;
      viewPort.y += sqSize;
    }
  }

  void Prepare(int argc, char **argv)
  {
    devExts.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    optDevExts.push_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
    optDevExts.push_back(VK_EXT_NESTED_COMMAND_BUFFER_EXTENSION_NAME);

    VulkanGraphicsTest::Prepare(argc, argv);

    if(!Avail.empty())
      return;

    static VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufaddrFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR,
    };

    getPhysFeatures2(&bufaddrFeatures);

    if(!bufaddrFeatures.bufferDeviceAddress)
      Avail = "feature 'bufferDeviceAddress' not available";

    bufaddrFeatures.pNext = (void *)devInfoNext;
    devInfoNext = &bufaddrFeatures;

    if(hasExt(VK_EXT_NESTED_COMMAND_BUFFER_EXTENSION_NAME))
    {
      getPhysFeatures2(&nestedFeats);

      if((nestedFeats.nestedCommandBuffer) && (nestedFeats.nestedCommandBufferRendering))
      {
        nestedFeats.pNext = (void *)devInfoNext;
        devInfoNext = &nestedFeats;
      }
    }

    if(hasExt(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME))
    {
      getPhysFeatures2(&descBufFeats);
      if(descBufFeats.descriptorBuffer && descBufFeats.descriptorBufferCaptureReplay)
      {
        descBufFeats.pNext = (void *)devInfoNext;
        devInfoNext = &descBufFeats;
      }
    }
  }

  size_t DescSize(VkDescriptorType type)
  {
    if(type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
      return descBufProps.uniformBufferDescriptorSize;
    else if(type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
      return descBufProps.storageBufferDescriptorSize;
    else if(type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
      return descBufProps.storageImageDescriptorSize;
    else if(type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
      return descBufProps.combinedImageSamplerDescriptorSize;

    return 0;
  }

  void FillDescriptor(VkDescriptorSetLayout layout, uint32_t bind, VkDescriptorType type,
                      VkDeviceSize range, VkDeviceAddress dataAddress, byte *const descMem)
  {
    VkDescriptorAddressInfoEXT buf = {VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
    buf.address = dataAddress;
    buf.range = range;
    buf.format = VK_FORMAT_UNDEFINED;

    VkDescriptorGetInfoEXT get = {VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
    get.type = type;
    get.data.pStorageBuffer = &buf;

    VkDeviceSize bindOffset;
    vkGetDescriptorSetLayoutBindingOffsetEXT(device, layout, bind, &bindOffset);
    void *dst = descMem + bindOffset;

    vkGetDescriptorEXT(device, &get, DescSize(type), dst);
  }

  void FillDescriptor(VkDescriptorSetLayout layout, uint32_t bind, VkDescriptorType type,
                      VkSampler sampler, VkImageView view, byte *const descMem)
  {
    VkDescriptorImageInfo im;
    im.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    im.imageView = view;
    im.sampler = sampler;

    VkDescriptorGetInfoEXT get = {VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
    get.type = type;
    get.data.pCombinedImageSampler = &im;

    VkDeviceSize bindOffset;
    vkGetDescriptorSetLayoutBindingOffsetEXT(device, layout, bind, &bindOffset);
    void *dst = descMem + bindOffset;

    vkGetDescriptorEXT(device, &get, DescSize(type), dst);
  }

  int main()
  {
    vmaBDA = true;

    // initialise, create window, create context, etc
    if(!Init())
      return 3;

    bool nestedSecondaries = hasExt(VK_EXT_NESTED_COMMAND_BUFFER_EXTENSION_NAME);
    if(nestedSecondaries)
    {
      getPhysFeatures2(&nestedFeats);
      if(!nestedFeats.nestedCommandBuffer)
        nestedSecondaries = false;
      if(!nestedFeats.nestedCommandBufferRendering)
        nestedSecondaries = false;
      getPhysProperties2(&nestedProps);
      if(nestedProps.maxCommandBufferNestingLevel < 5)
        nestedSecondaries = false;
    }

    bool descBuffer = hasExt(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
    if(descBuffer)
    {
      getPhysFeatures2(&descBufFeats);
      if(!descBufFeats.descriptorBuffer)
        descBuffer = false;
      if(!descBufFeats.descriptorBufferCaptureReplay)
        descBuffer = false;
      getPhysProperties2(&descBufProps);
    }

    if(nestedSecondaries)
      TEST_LOG("Running tests with nested secondaries");
    if(descBuffer)
      TEST_LOG("Running tests with descriptor buffer");

    vkh::RenderPassCreator renderPassCreateInfo;
    renderPassCreateInfo.attachments.push_back(vkh::AttachmentDescription(
        mainWindow->format, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
        VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_SAMPLE_COUNT_1_BIT));
    renderPassCreateInfo.addSubpass({VkAttachmentReference({0, VK_IMAGE_LAYOUT_GENERAL})},
                                    VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED);
    VkRenderPass renderPass = createRenderPass(renderPassCreateInfo);
    setName(renderPass, "Main Render Pass");

    VkPipelineLayout noDescSetPipeLayout = createPipelineLayout(vkh::PipelineLayoutCreateInfo());
    setName(noDescSetPipeLayout, "No Descriptor Set Pipeline Layout");

    vkh::GraphicsPipelineCreateInfo pipeCreateInfo;

    pipeCreateInfo.layout = noDescSetPipeLayout;
    pipeCreateInfo.renderPass = renderPass;

    pipeCreateInfo.vertexInputState.vertexBindingDescriptions = {vkh::vertexBind(0, DefaultA2V)};
    pipeCreateInfo.vertexInputState.vertexAttributeDescriptions = {
        vkh::vertexAttr(0, 0, DefaultA2V, pos),
        vkh::vertexAttr(1, 0, DefaultA2V, col),
        vkh::vertexAttr(2, 0, DefaultA2V, uv),
    };

    pipeCreateInfo.stages = {
        CompileShaderModule(VKDefaultVertex, ShaderLang::glsl, ShaderStage::vert, "main"),
        CompileShaderModule(VKDefaultPixel, ShaderLang::glsl, ShaderStage::frag, "main"),
    };

    VkPipeline noDescSetPipe = createGraphicsPipeline(pipeCreateInfo);
    setName(noDescSetPipe, "No Descriptor Set Pipeline");

    VkDescriptorSetLayout descSetLayout =
        createDescriptorSetLayout(vkh::DescriptorSetLayoutCreateInfo({
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        }));
    setName(descSetLayout, "Descriptor Set Layout");

    VkPipelineLayout descSetPipeLayout =
        createPipelineLayout(vkh::PipelineLayoutCreateInfo({descSetLayout}));
    setName(descSetPipeLayout, "Descriptor Set Pipeline Layout");

    pipeCreateInfo.layout = descSetPipeLayout;
    pipeCreateInfo.stages = {
        CompileShaderModule(VKDefaultVertex, ShaderLang::glsl, ShaderStage::vert, "main"),
        CompileShaderModule(pixel, ShaderLang::glsl, ShaderStage::frag, "main"),
    };
    VkPipeline descSetPipe = createGraphicsPipeline(pipeCreateInfo);
    setName(descSetPipe, "Descriptor Set Pipeline");

    VkDescriptorSetLayout descBuffLayout = VK_NULL_HANDLE;
    VkPipelineLayout descBuffPipeLayout = VK_NULL_HANDLE;
    VkPipeline descBuffPipe = VK_NULL_HANDLE;
    if(descBuffer)
    {
      descBuffLayout = createDescriptorSetLayout(vkh::DescriptorSetLayoutCreateInfo(
          {
              {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
          },
          VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT));
      setName(descBuffLayout, "Descriptor Buffer Layout");

      descBuffPipeLayout = createPipelineLayout(vkh::PipelineLayoutCreateInfo({descBuffLayout}));
      setName(descBuffPipeLayout, "Descriptor Buffer Pipeline Layout");

      pipeCreateInfo.layout = descBuffPipeLayout;
      pipeCreateInfo.flags |= VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
      descBuffPipe = createGraphicsPipeline(pipeCreateInfo);
      setName(descBuffPipe, "Descriptor Buffer Pipeline");
    }

    VkDescriptorSetLayout compDescSetLayout =
        createDescriptorSetLayout(vkh::DescriptorSetLayoutCreateInfo({
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
            {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
        }));
    setName(compDescSetLayout, "Compute Descriptor Set Layout");

    VkPipelineLayout compDescSetPipeLayout =
        createPipelineLayout(vkh::PipelineLayoutCreateInfo({compDescSetLayout}));
    setName(compDescSetPipeLayout, "Compute Pipeline Layout");

    VkPipeline compDescSetPipe = createComputePipeline(vkh::ComputePipelineCreateInfo(
        compDescSetPipeLayout,
        CompileShaderModule(compute, ShaderLang::glsl, ShaderStage::comp, "main")));
    setName(compDescSetPipe, "Compute Descriptor Set Pipeline");

    VkPipelineLayout compDescBuffPipeLayout = VK_NULL_HANDLE;
    VkPipeline compDescBuffPipe = VK_NULL_HANDLE;
    if(descBuffer)
    {
      compDescBuffPipeLayout = createPipelineLayout(vkh::PipelineLayoutCreateInfo({descBuffLayout}));
      setName(compDescSetPipeLayout, "Compute Descriptor Buffeer Pipeline Layout");

      compDescBuffPipe = createComputePipeline(vkh::ComputePipelineCreateInfo(
          compDescBuffPipeLayout,
          CompileShaderModule(compute, ShaderLang::glsl, ShaderStage::comp, "main"),
          VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT));
      setName(compDescBuffPipe, "Compute Descriptor Buffer Pipeline");
    }

    AllocatedBuffer vb(
        this,
        vkh::BufferCreateInfo(sizeof(DefaultTri),
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
        VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_CPU_TO_GPU}));
    setName(vb.buffer, "Vertex Buffer");

    vb.upload(DefaultTri);

    AllocatedBuffer ib(
        this,
        vkh::BufferCreateInfo(sizeof(uint16_t) * 3,
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
        VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_CPU_TO_GPU}));
    setName(ib.buffer, "Index Buffer");
    {
      uint16_t *dst = NULL;
      VmaAllocationInfo alloc_info;
      vmaGetAllocationInfo(ib.allocator, ib.alloc, &alloc_info);
      vkMapMemory(device, alloc_info.deviceMemory, alloc_info.offset, VK_WHOLE_SIZE, 0,
                  (void **)&dst);
      memset(dst, 0, sizeof(uint16_t) * 3);
      dst[0] = 0;
      dst[1] = 1;
      dst[2] = 2;
      vkUnmapMemory(device, alloc_info.deviceMemory);
    }

    VkDescriptorSet descSet = allocateDescriptorSet(descSetLayout);
    setName(descSet, "Descriptor Set");

    VkDescriptorSet compDescSet = allocateDescriptorSet(compDescSetLayout);
    setName(compDescSet, "Compute Descriptor Set");

    AllocatedImage offimg(
        this,
        vkh::ImageCreateInfo(4, 4, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
                             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
        VmaAllocationCreateInfo(
            {VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_MEMORY_USAGE_GPU_ONLY}));
    setName(offimg.image, "Offscreen Image");
    VkImageView offimgRTV = createImageView(vkh::ImageViewCreateInfo(
        offimg.image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT));
    setName(offimgRTV, "Offscreen Image RTV");

    AllocatedImage offimgMS(
        this,
        vkh::ImageCreateInfo(4, 4, 0, VK_FORMAT_R16G16B16A16_SFLOAT,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, VK_SAMPLE_COUNT_4_BIT),
        VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_GPU_ONLY}));
    setName(offimgMS.image, "Offscreen MSAA Image");

    VkSampler linearSampler = createSampler(vkh::SamplerCreateInfo(VK_FILTER_LINEAR));
    setName(linearSampler, "Linear Sampler");

    vkh::updateDescriptorSets(
        device, {
                    vkh::WriteDescriptorSet(descSet, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            {vkh::DescriptorImageInfo(
                                                offimgRTV, VK_IMAGE_LAYOUT_GENERAL, linearSampler)}),
                });

    AllocatedBuffer compBufIn(
        this,
        vkh::BufferCreateInfo(1024, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR |
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
        VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_CPU_TO_GPU}));
    setName(compBufIn.buffer, "Compute Buffer In");
    {
      VmaAllocationInfo alloc_info;
      vmaGetAllocationInfo(compBufIn.allocator, compBufIn.alloc, &alloc_info);
      byte *dst = NULL;
      vkMapMemory(device, alloc_info.deviceMemory, alloc_info.offset, VK_WHOLE_SIZE, 0,
                  (void **)&dst);
      memset(dst, 0xDE, 1024);
      vkUnmapMemory(device, alloc_info.deviceMemory);
    }

    AllocatedBuffer compBufOut(
        this,
        vkh::BufferCreateInfo(1024, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR |
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
        VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_CPU_TO_GPU}));

    setName(compBufOut.buffer, "Compute Buffer Out");

    VkDescriptorBufferBindingInfoEXT descBuffBind = {};
    AllocatedBuffer descBuf;
    if(descBuffer)
    {
      descBuf = AllocatedBuffer(
          this,
          vkh::BufferCreateInfo(0x1000, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR |
                                            VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
                                            VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT),
          VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_CPU_TO_GPU}));

      setName(descBuf.buffer, "Descriptor Buffer");
      VmaAllocationInfo alloc_info;
      vmaGetAllocationInfo(descBuf.allocator, descBuf.alloc, &alloc_info);
      byte *descBufMem = NULL;
      vkMapMemory(device, alloc_info.deviceMemory, alloc_info.offset, VK_WHOLE_SIZE, 0,
                  (void **)&descBufMem);
      memset(descBufMem, 0xCC, 0x1000);

      VkDeviceAddress compBufInBDA = compBufIn.address;
      FillDescriptor(descBuffLayout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024, compBufInBDA,
                     descBufMem);
      VkDeviceAddress compBufOutBDA = compBufOut.address;
      FillDescriptor(descBuffLayout, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024, compBufOutBDA,
                     descBufMem);
      FillDescriptor(descBuffLayout, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, linearSampler,
                     offimgRTV, descBufMem);
      vkUnmapMemory(device, alloc_info.deviceMemory);

      descBuffBind = {
          VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
          NULL,
          descBuf.address,
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR |
              VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
              VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
      };
    }

    vkh::updateDescriptorSets(
        device, {
                    vkh::WriteDescriptorSet(compDescSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                            {vkh::DescriptorBufferInfo(compBufIn.buffer)}),
                    vkh::WriteDescriptorSet(compDescSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            {vkh::DescriptorBufferInfo(compBufOut.buffer)}),
                });

    sqSize = float(screenHeight) / 4.0f;

    while(Running())
    {
      viewPort = {0.0f, 0.0f, sqSize, sqSize, 0.0f, 1.0f};

      VkCommandBuffer secCmd = GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
      vkBeginCommandBuffer(
          secCmd, vkh::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
                                                  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
                                              vkh::CommandBufferInheritanceInfo(renderPass, 0)));

      pushMarker(secCmd, "No Descriptor Set");
      {
        vkCmdSetScissor(secCmd, 0, 1, &mainWindow->scissor);
        vkh::cmdBindVertexBuffers(secCmd, 0, {vb.buffer}, {0});
        vkCmdBindIndexBuffer(secCmd, ib.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindPipeline(secCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, noDescSetPipe);

        // Vertex draw
        setMarker(secCmd, "Vertex Draw");
        vkCmdSetViewport(secCmd, 0, 1, &viewPort);
        vkCmdDraw(secCmd, 3, 1, 0, 0);
        NextTest();
        // Indexed draw
        setMarker(secCmd, "Indexed Draw");
        vkCmdSetViewport(secCmd, 0, 1, &viewPort);
        vkCmdDrawIndexed(secCmd, 3, 1, 0, 0, 0);
        NextTest();
      }
      popMarker(secCmd);

      vkEndCommandBuffer(secCmd);

      VkCommandBuffer nestedCmd = GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
      if(nestedSecondaries)
      {
        VkCommandBuffer nestedSecCmd = GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        vkBeginCommandBuffer(nestedSecCmd, vkh::CommandBufferBeginInfo(
                                               VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
                                                   VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
                                               vkh::CommandBufferInheritanceInfo(renderPass, 0)));
        vkCmdSetScissor(nestedSecCmd, 0, 1, &mainWindow->scissor);
        vkh::cmdBindVertexBuffers(nestedSecCmd, 0, {vb.buffer}, {0});
        vkCmdBindIndexBuffer(nestedSecCmd, ib.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindPipeline(nestedSecCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, noDescSetPipe);

        // Vertex draw
        setMarker(nestedSecCmd, "Vertex Draw");
        vkCmdSetViewport(nestedSecCmd, 0, 1, &viewPort);
        vkCmdDraw(nestedSecCmd, 3, 1, 0, 0);
        NextTest();
        // Indexed draw
        setMarker(nestedSecCmd, "Indexed Draw");
        vkCmdSetViewport(nestedSecCmd, 0, 1, &viewPort);
        vkCmdDrawIndexed(nestedSecCmd, 3, 1, 0, 0, 0);
        NextTest();
        vkEndCommandBuffer(nestedSecCmd);

        vkBeginCommandBuffer(nestedCmd, vkh::CommandBufferBeginInfo(
                                            VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
                                            vkh::CommandBufferInheritanceInfo(renderPass, 0)));

        vkCmdExecuteCommands(nestedCmd, 1, &nestedSecCmd);
        vkEndCommandBuffer(nestedCmd);
      }

      VkCommandBuffer compSecCmd = GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
      vkBeginCommandBuffer(compSecCmd, vkh::CommandBufferBeginInfo(
                                           VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
                                           vkh::CommandBufferInheritanceInfo(VK_NULL_HANDLE, 0)));
      vkh::cmdBindDescriptorSets(compSecCmd, VK_PIPELINE_BIND_POINT_COMPUTE, compDescSetPipeLayout,
                                 0, {compDescSet}, {});
      vkCmdBindPipeline(compSecCmd, VK_PIPELINE_BIND_POINT_COMPUTE, compDescSetPipe);
      vkCmdDispatch(compSecCmd, 1, 1, 1);
      vkEndCommandBuffer(compSecCmd);

      VkCommandBuffer compNestedSecCmd = GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
      if(nestedSecondaries)
      {
        vkBeginCommandBuffer(
            compNestedSecCmd,
            vkh::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                        vkh::CommandBufferInheritanceInfo(VK_NULL_HANDLE, 0)));
        vkCmdExecuteCommands(compNestedSecCmd, 1, &compSecCmd);
        vkEndCommandBuffer(compNestedSecCmd);
      }

      VkCommandBuffer cmd = GetCommandBuffer();

      vkBeginCommandBuffer(cmd, vkh::CommandBufferBeginInfo());

      VkImage swapimg =
          StartUsingBackbuffer(cmd, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);

      vkCmdClearColorImage(cmd, swapimg, VK_IMAGE_LAYOUT_GENERAL,
                           vkh::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f), 1,
                           vkh::ImageSubresourceRange());

      vkh::cmdPipelineBarrier(
          cmd, {
                   vkh::ImageMemoryBarrier(
                       VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                       VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, swapimg),
               });

      vkh::cmdPipelineBarrier(
          cmd, {
                   vkh::ImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_GENERAL, offimg.image),
               });

      vkCmdClearColorImage(cmd, offimg.image, VK_IMAGE_LAYOUT_GENERAL,
                           vkh::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f), 1,
                           vkh::ImageSubresourceRange());

      vkh::cmdPipelineBarrier(
          cmd, {
                   vkh::ImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_GENERAL, offimgMS.image),
               });

      vkCmdClearColorImage(cmd, offimgMS.image, VK_IMAGE_LAYOUT_GENERAL,
                           vkh::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f), 1,
                           vkh::ImageSubresourceRange());

      // Graphics
      pushMarker(cmd, "Graphics");
      {
        vkCmdBeginRenderPass(
            cmd, vkh::RenderPassBeginInfo(mainWindow->rp, mainWindow->GetFB(), mainWindow->scissor),
            VK_SUBPASS_CONTENTS_INLINE);

        // No Descriptor Set Usage
        pushMarker(cmd, "No Descriptor Set");
        {
          vkCmdSetScissor(cmd, 0, 1, &mainWindow->scissor);
          vkh::cmdBindVertexBuffers(cmd, 0, {vb.buffer}, {0});
          vkCmdBindIndexBuffer(cmd, ib.buffer, 0, VK_INDEX_TYPE_UINT16);
          vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, noDescSetPipe);

          // Vertex draw
          setMarker(cmd, "Vertex Draw");
          vkCmdSetViewport(cmd, 0, 1, &viewPort);
          vkCmdDraw(cmd, 3, 1, 0, 0);
          NextTest();
          // Indexed draw
          setMarker(cmd, "Indexed Draw");
          vkCmdSetViewport(cmd, 0, 1, &viewPort);
          vkCmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
          NextTest();
        }
        popMarker(cmd);

        // Descriptor Set Usage
        pushMarker(cmd, "Descriptor Set");
        {
          vkh::cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, descSetPipeLayout, 0,
                                     {descSet}, {});
          vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, descSetPipe);

          // Vertex draw
          setMarker(cmd, "Vertex Draw");
          vkCmdSetViewport(cmd, 0, 1, &viewPort);
          vkCmdDraw(cmd, 3, 1, 0, 0);
          NextTest();
          // Indexed draw
          setMarker(cmd, "Indexed Draw");
          vkCmdSetViewport(cmd, 0, 1, &viewPort);
          vkCmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
          NextTest();
        }
        popMarker(cmd);

        // Descriptor Buffer
        if(descBuffer)
        {
          pushMarker(cmd, "Descriptor Buffer");
          {
            vkCmdBindDescriptorBuffersEXT(cmd, 1, &descBuffBind);
            uint32_t descBuffSetIndex = 0;
            VkDeviceSize descBuffSetOffset = 0;
            vkCmdSetDescriptorBufferOffsetsEXT(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                               descBuffPipeLayout, 0, 1, &descBuffSetIndex,
                                               &descBuffSetOffset);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, descBuffPipe);
            // Vertex draw
            setMarker(cmd, "Vertex Draw");
            vkCmdSetViewport(cmd, 0, 1, &viewPort);
            vkCmdDraw(cmd, 3, 1, 0, 0);
            NextTest();
            // Indexed draw
            setMarker(cmd, "Indexed Draw");
            vkCmdSetViewport(cmd, 0, 1, &viewPort);
            vkCmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
            NextTest();
          }
          popMarker(cmd);
        }

        vkCmdEndRenderPass(cmd);

        // Secondary Command Buffer
        pushMarker(cmd, "Secondary Command Buffer");
        {
          vkCmdBeginRenderPass(
              cmd, vkh::RenderPassBeginInfo(mainWindow->rp, mainWindow->GetFB(), mainWindow->scissor),
              VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
          vkCmdExecuteCommands(cmd, 1, &secCmd);
          vkCmdEndRenderPass(cmd);

          // Nested Secondary Command Buffer
          if(nestedSecondaries)
          {
            setMarker(cmd, "Nested Secondary Command Buffer");
            vkCmdBeginRenderPass(
                cmd,
                vkh::RenderPassBeginInfo(mainWindow->rp, mainWindow->GetFB(), mainWindow->scissor),
                VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
            vkCmdExecuteCommands(cmd, 1, &nestedCmd);
            vkCmdEndRenderPass(cmd);
          }
        }
        popMarker(cmd);
      }
      popMarker(cmd);

      // Compute
      pushMarker(cmd, "Compute");
      {
        // Descriptor Set Usage
        pushMarker(cmd, "Descriptor Set");
        {
          vkh::cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compDescSetPipeLayout, 0,
                                     {compDescSet}, {});
          vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compDescSetPipe);
          vkCmdDispatch(cmd, 1, 1, 1);
        }
        popMarker(cmd);

        // Descriptor Buffer
        if(descBuffer)
        {
          pushMarker(cmd, "Descriptor Buffer");
          {
            vkCmdBindDescriptorBuffersEXT(cmd, 1, &descBuffBind);
            uint32_t descBuffSetIndex = 0;
            VkDeviceSize descBuffSetOffset = 0;
            vkCmdSetDescriptorBufferOffsetsEXT(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                               compDescBuffPipeLayout, 0, 1, &descBuffSetIndex,
                                               &descBuffSetOffset);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compDescBuffPipe);
            vkCmdDispatch(cmd, 1, 1, 1);
          }
          popMarker(cmd);
        }

        // Secondary Command Buffer
        pushMarker(cmd, "Secondary Command Buffer");
        {
          vkCmdExecuteCommands(cmd, 1, &compSecCmd);

          // Nested Secondary Command Buffer
          if(nestedSecondaries)
          {
            setMarker(cmd, "Nested Secondary Command Buffer");
            vkCmdExecuteCommands(cmd, 1, &compNestedSecCmd);
          }
        }
        popMarker(cmd);
      }
      popMarker(cmd);

      FinishUsingBackbuffer(cmd, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);

      vkEndCommandBuffer(cmd);

      Submit(0, 1, {cmd});

      Present();
    }

    return 0;
  }
};

REGISTER_TEST();
