/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2022 Baldur Karlsson
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

RD_TEST(VK_Pipeline_State, VulkanGraphicsTest)
{
  static constexpr const char *Description = "Tests related to pipeline state objects";

  void Prepare(int argc, char **argv)
  {
    devExts.push_back(VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME);

    VulkanGraphicsTest::Prepare(argc, argv);

    if(!Avail.empty())
      return;

    static VkPhysicalDevicePipelineCreationCacheControlFeatures _cacheControlFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES,
    };

    getPhysFeatures2(&_cacheControlFeatures);

    if(!_cacheControlFeatures.pipelineCreationCacheControl)
      Avail = "Pipeline Creation feature 'pipelineCacheControl' not available";

    devInfoNext = &_cacheControlFeatures;
  }

  int main()
  {
    // initialise, create window, create context, etc
    if(!Init())
      return 3;

    VkPipelineLayout layout = createPipelineLayout(vkh::PipelineLayoutCreateInfo());

    vkh::GraphicsPipelineCreateInfo pipeCreateInfo;

    pipeCreateInfo.layout = layout;
    pipeCreateInfo.renderPass = mainWindow->rp;

    pipeCreateInfo.vertexInputState.vertexBindingDescriptions = {vkh::vertexBind(0, DefaultA2V)};
    pipeCreateInfo.vertexInputState.vertexAttributeDescriptions = {
        vkh::vertexAttr(0, 0, DefaultA2V, pos), vkh::vertexAttr(1, 0, DefaultA2V, col),
        vkh::vertexAttr(2, 0, DefaultA2V, uv),
    };

    pipeCreateInfo.stages = {
        CompileShaderModule(VKDefaultVertex, ShaderLang::glsl, ShaderStage::vert, "main"),
        CompileShaderModule(VKDefaultPixel, ShaderLang::glsl, ShaderStage::frag, "main"),
    };

    VkPipelineCacheCreateInfo pipeCacheCreateInfo;
    pipeCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeCacheCreateInfo.pNext = NULL;
    pipeCacheCreateInfo.flags = 0;
    pipeCacheCreateInfo.initialDataSize = 0;
    pipeCacheCreateInfo.pInitialData = NULL;

    VkPipelineCache pipelineCache;
    CHECK_VKR(vkCreatePipelineCache(device, &pipeCacheCreateInfo, NULL, &pipelineCache));

    vkh::GraphicsPipelineCreateInfo pipeCreateInfo2(pipeCreateInfo);
    pipeCreateInfo2.flags |= VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT;

    VkGraphicsPipelineCreateInfo infos[2];
    infos[0] = *(const VkGraphicsPipelineCreateInfo *)pipeCreateInfo;
    infos[1] = *(const VkGraphicsPipelineCreateInfo *)pipeCreateInfo2;
    VkPipeline graphicsPipes[2];
    CHECK_VKR(vkCreateGraphicsPipelines(device, pipelineCache, 2, infos, NULL, graphicsPipes));
    pipes.push_back(graphicsPipes[0]);
    pipes.push_back(graphicsPipes[1]);
    VkPipeline pipe = graphicsPipes[1];
    ;
    AllocatedBuffer vb(
        this, vkh::BufferCreateInfo(sizeof(DefaultTri), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT),
        VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_CPU_TO_GPU}));

    vb.upload(DefaultTri);

    AllocatedImage offimg(this, vkh::ImageCreateInfo(4, 4, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                     VK_IMAGE_USAGE_TRANSFER_DST_BIT),
                          VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_GPU_ONLY}));

    AllocatedImage offimgMS(
        this, vkh::ImageCreateInfo(4, 4, 0, VK_FORMAT_R16G16B16A16_SFLOAT,
                                   VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, VK_SAMPLE_COUNT_4_BIT),
        VmaAllocationCreateInfo({0, VMA_MEMORY_USAGE_GPU_ONLY}));

    while(Running())
    {
      VkCommandBuffer cmd = GetCommandBuffer();

      vkBeginCommandBuffer(cmd, vkh::CommandBufferBeginInfo());

      VkImage swapimg =
          StartUsingBackbuffer(cmd, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);

      vkCmdClearColorImage(cmd, swapimg, VK_IMAGE_LAYOUT_GENERAL,
                           vkh::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f), 1,
                           vkh::ImageSubresourceRange());

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

      vkCmdBeginRenderPass(
          cmd, vkh::RenderPassBeginInfo(mainWindow->rp, mainWindow->GetFB(), mainWindow->scissor),
          VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
      vkCmdSetViewport(cmd, 0, 1, &mainWindow->viewport);
      vkCmdSetScissor(cmd, 0, 1, &mainWindow->scissor);
      vkh::cmdBindVertexBuffers(cmd, 0, {vb.buffer}, {0});
      vkCmdDraw(cmd, 3, 1, 0, 0);

      vkCmdEndRenderPass(cmd);

      FinishUsingBackbuffer(cmd, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);

      vkEndCommandBuffer(cmd);

      Submit(0, 1, {cmd});

      Present();
    }

    vkDestroyPipelineCache(device, pipelineCache, NULL);
    return 0;
  }
};

REGISTER_TEST();
