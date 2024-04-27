/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Baldur Karlsson
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

#include "metal_test.h"

#include <simd/simd.h>

RD_TEST(Metal_Simple_Triangle, MetalGraphicsTest)
{
  static constexpr const char *Description =
      "Just draws a simple triangle, using normal pipeline. Basic test that can be used "
      "for any dead-simple tests that don't require any particular API use";

  int main()
  {
    if(!Init())
      return 3;

    MTL::RenderPipelineState *rpState = CreateDefaultPipeline();

    MTL::Buffer *verticesBuffer =
        m_device->newBuffer(DefaultTri, sizeof(DefaultTri), MTL::ResourceStorageModeShared);

    while(Running())
    {
      if(!m_autoreleasePool)
        m_autoreleasePool = NS::AutoreleasePool::alloc()->init();
      MTL::CommandBuffer *cmdBuffer = GetCommandBuffer();

      MTL::RenderCommandEncoder *encoder = CreateRenderPass(m_mainWindow->BackBuffer(), cmdBuffer);
      encoder->setRenderPipelineState(rpState);
      encoder->setVertexBuffer(verticesBuffer, 0, 0);
      encoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0),
                              NS::UInteger(3));
      encoder->endEncoding();

      cmdBuffer->commit();
      Present();
      m_autoreleasePool->drain();
      m_autoreleasePool = NULL;
    }

    rpState->release();
    verticesBuffer->release();
    return 0;
  }
};

REGISTER_TEST();
