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

#include "d3d12_test.h"

RD_TEST(D3D12_Resource_Usage, D3D12GraphicsTest)
{
  static constexpr const char *Description = "Test resource usage in a variety of scenarios.";

  std::string vertex = R"EOSHADER(

struct vertin
{
	float3 pos : POSITION;
	float4 col : COLOR0;
	float2 uv : TEXCOORD0;
};

struct v2f
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv : TEXCOORD0;
};

struct Data
{
  float4 data;
};

ByteAddressBuffer rootSRV : register(t0, space0);
RWStructuredBuffer<Data> rootUAV : register(u0, space0);
ConstantBuffer<Data> rootCBV : register(b0, space0);

v2f main(vertin IN)
{
	v2f OUT = (v2f)0;

    int ZERO = floor(abs(IN.pos.x)/(abs(IN.pos.y)+1.0e-6f));
    int ONE = ZERO + 1;

	OUT.pos = float4(IN.pos.xyz, 1);
	OUT.col = IN.col;
	OUT.uv = IN.uv;

    OUT.col += rootCBV.data;
    OUT.col += asfloat(rootSRV.Load(ONE));
    OUT.col += rootUAV[ONE].data;
    OUT.col /= 100.0;

	return OUT;
}

)EOSHADER";

  std::string pixel = R"EOSHADER(

struct v2f
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv : TEXCOORD0;
};

Texture2D<float4> smiley : register(t0, space0); // GPU = 100
Texture2D<float4> unusedTexture : register(t1, space0); // GPU = 101

RWBuffer<float4> buffer : register(u0, space0); // GPU = 200
RWBuffer<float4> unusedBuffer : register(u1, space0); // GPU = 201

struct Data
{
  float4 data;
};
ConstantBuffer<Data> cbv : register(b0, space0); // GPU = 300
ConstantBuffer<Data> unusedCBV : register(b1, space0); // GPU = 301

SamplerState heapSampler : register(s0, space0); // GPU = 0
SamplerState unusedHeapSampler : register(s1, space0); // GPU = 1

SamplerState staticSampler : register(s0, space1);
SamplerState unusedStaticSampler : register(s1, space1);

float4 main(v2f IN) : SV_Target0
{
    float4 col;

    col = IN.col;
    col *= smiley.Sample(staticSampler, IN.uv);
    col *= smiley.Sample(heapSampler, IN.uv + IN.uv);
    col *= buffer[IN.uv.x];
    col *= buffer[IN.uv.y];
    col += cbv.data;
    col /= 128.0;

	return col;
}

)EOSHADER";

  std::string compute = R"EOSHADER(

Texture2D<float4> smiley : register(t0, space0); // GPU = 100
Texture2D<float4> unusedTexture : register(t1, space0); // GPU = 101

RWBuffer<float4> buffer : register(u0, space0); // GPU = 200
RWBuffer<float4> unusedBuffer : register(u1, space0); // GPU = 201

struct Data
{
  float4 data;
};
ConstantBuffer<Data> cbv : register(b0, space0); // GPU = 300
ConstantBuffer<Data> unusedCBV : register(b1, space0); // GPU = 301

[numthreads(1,1,1)]
void main(int3 gid : SV_GroupID)
{
  int ZERO = floor(gid.x/(gid.x+1.0e-6f));
  int ONE = ZERO + 1;

  buffer[ZERO].x = ONE;
  buffer[ONE].x = ZERO;
  buffer[ZERO] += cbv.data;
  buffer[ONE] += smiley.Load(int3(ONE, ZERO, 0));
}

)EOSHADER";

  float sqSize;
  Vec2f viewXY;

  void NextTest()
  {
    viewXY.x += sqSize;

    if(viewXY.x + sqSize >= (float)screenWidth)
    {
      viewXY.x = 0.0f;
      viewXY.y += sqSize;
    }
  }

  int main()
  {
    // initialise, create window, create device, etc
    if(!Init())
      return 3;

    ID3DBlobPtr vsblob = Compile(vertex, "main", "vs_6_0");
    ID3DBlobPtr psblob = Compile(pixel, "main", "ps_6_0");
    ID3DBlobPtr csblob = Compile(compute, "main", "cs_6_0");

    ID3D12ResourcePtr vb = MakeBuffer().Data(DefaultTri);
    setName(vb, "Vertex Buffer");
    ID3D12ResourcePtr unusedVb = MakeBuffer().Data(DefaultTri);
    setName(unusedVb, "Unused Vertex Buffer");
    std::vector<uint16_t> idxData;
    idxData.resize(16);
    for(size_t i = 0; i < 16; ++i)
      idxData[i] = (uint16_t)i;
    ID3D12ResourcePtr ib = MakeBuffer().Data(idxData);
    setName(ib, "Index Buffer");
    for(size_t i = 0; i < 16; ++i)
      idxData[i] = 0xFFFF;
    ID3D12ResourcePtr unusedIb = MakeBuffer().Data(idxData);
    setName(unusedIb, "Unused Index Buffer");

    DefaultA2V eiTri[3];
    for(int i = 0; i < 3; ++i)
    {
      eiTri[i] = DefaultTri[i];
      eiTri[i].pos.x *= 1.5f;
      eiTri[i].pos.y *= 1.5f;
      eiTri[i].uv.x = 1.0f - eiTri[i].uv.x;
      eiTri[i].uv.y = 1.0f - eiTri[i].uv.y;
      eiTri[i].col.x *= 0.1f;
      eiTri[i].col.y *= 0.1f;
      eiTri[i].col.z *= 0.1f;
    }
    ID3D12ResourcePtr eiVB = MakeBuffer().Data(eiTri);
    setName(eiVB, "EI Vertex Buffer");

    for(size_t i = 0; i < 16; ++i)
      idxData[i] = (uint16_t)((i + 1) % 3);
    ID3D12ResourcePtr eiIB = MakeBuffer().Data(idxData);
    setName(eiIB, "EI Index Buffer");

    D3D12_STATIC_SAMPLER_DESC staticSamp = {};
    staticSamp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    staticSamp.ShaderRegister = 0;
    staticSamp.RegisterSpace = 1;

    D3D12_STATIC_SAMPLER_DESC samplers[] = {staticSamp, staticSamp};
    samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
    samplers[1].ShaderRegister = 1;

    ID3D12RootSignaturePtr sig = MakeSig(
        {
            // ByteAddressBuffer rootSRV : register(t0, space0);
            srvParam(D3D12_SHADER_VISIBILITY_VERTEX, 0, 0),
            // RWStructuredBuffer<Data> rootUAV : register(u0, space0);
            uavParam(D3D12_SHADER_VISIBILITY_VERTEX, 0, 0),
            // ConstantBuffer<Data> rootCBV : register(b0, space0);
            cbvParam(D3D12_SHADER_VISIBILITY_VERTEX, 0, 0),
            // Texture2D<float4> smiley : register(t0, space0); // GPU = 100
            // Texture2D<float4> unusedTexture : register(t1, space0); // GPU = 101
            tableParam(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 0, 2, 100),
            // RWBuffer<float4> buffer : register(u0, space0); // GPU = 200
            // RWBuffer<float4> unusedBuffer : register(u1, space0); // GPU = 201
            tableParam(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 0, 1, 200),
            // ConstantBuffer<Data> cbv : register(b0, space0); // GPU = 300
            // ConstantBuffer<Data> unusedCBV : register(b1, space0); // GPU = 301
            tableParam(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 0, 1, 300),
            // SamplerState heapSampler : register(s0, space0); // GPU = 0
            // SamplerState unusedHeapSampler : register(s1, space0); // GPU = 1
            tableParam(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 0, 2, 0),
        },
        // SamplerState staticSampler : register(s0, space1);
        // SamplerState unusedStaticSampler : register(s1, space1);
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, 2, samplers);
    setName(sig, "Default Root Signature");

    ID3D12PipelineStatePtr pso = MakePSO().RootSig(sig).InputLayout().VS(vsblob).PS(psblob);
    setName(pso, "Default PSO");

    ID3D12RootSignaturePtr computeSig = MakeSig({
        // Texture2D<float4> smiley : register(t0, space0); // GPU = 100
        // Texture2D<float4> unusedTexture : register(t1, space0); // GPU = 101
        tableParam(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 0, 2, 100),
        // RWBuffer<float4> buffer : register(u0, space0); // GPU = 200
        // RWBuffer<float4> unusedBuffer : register(u1, space0); // GPU = 201
        tableParam(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 0, 1, 200),
        // ConstantBuffer<Data> cbv : register(b0, space0); // GPU = 300
        // ConstantBuffer<Data> unusedCBV : register(b1, space0); // GPU = 301
        tableParam(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 0, 1, 300),
    });
    setName(computeSig, "Compute Root Signature");
    ID3D12PipelineStatePtr computePSO = MakePSO().RootSig(computeSig).CS(csblob);
    setName(computePSO, "Compute PSO");

    ResourceBarrier(vb, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    Texture rgba8;
    LoadXPM(SmileyTexture, rgba8);

    ID3D12ResourcePtr smiley = MakeTexture(DXGI_FORMAT_R8G8B8A8_TYPELESS, 48, 48)
                                   .Mips(1)
                                   .InitialState(D3D12_RESOURCE_STATE_COPY_DEST);
    setName(smiley, "Smiley Texture");
    ID3D12ResourcePtr unusedSRV = MakeTexture(DXGI_FORMAT_R8G8B8A8_TYPELESS, 48, 48)
                                      .Mips(1)
                                      .InitialState(D3D12_RESOURCE_STATE_COPY_DEST);
    setName(unusedSRV, "Unused SRV");

    ID3D12ResourcePtr uploadBuf = MakeBuffer().Size(1024 * 1024).Upload();
    setName(uploadBuf, "Upload Buffer");
    {
      D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};

      D3D12_RESOURCE_DESC desc = smiley->GetDesc();

      dev->GetCopyableFootprints(&desc, 0, 1, 0, &layout, NULL, NULL, NULL);

      byte *srcptr = (byte *)rgba8.data.data();
      byte *mapptr = NULL;
      uploadBuf->Map(0, NULL, (void **)&mapptr);

      ID3D12GraphicsCommandListPtr cmd = GetCommandBuffer();

      Reset(cmd);

      {
        D3D12_TEXTURE_COPY_LOCATION dst, src;

        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.pResource = smiley;
        dst.SubresourceIndex = 0;

        byte *dstptr = mapptr + layout.Offset;

        for(UINT row = 0; row < rgba8.height; row++)
        {
          memcpy(dstptr, srcptr, rgba8.width * sizeof(uint32_t));
          srcptr += rgba8.width * sizeof(uint32_t);
          dstptr += layout.Footprint.RowPitch;
        }

        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.pResource = uploadBuf;
        src.PlacedFootprint = layout;

        cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);

        D3D12_RESOURCE_BARRIER b = {};
        b.Transition.pResource = smiley;
        b.Transition.Subresource = 0;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        cmd->ResourceBarrier(1, &b);
      }

      cmd->Close();

      uploadBuf->Unmap(0, NULL);

      Submit({cmd});
      GPUSync();
    }

    // Texture2D<float4> smiley : register(t0, space0); GPU = 100
    MakeSRV(smiley).Format(DXGI_FORMAT_R8G8B8A8_UNORM).CreateGPU(100);
    MakeSRV(unusedSRV).Format(DXGI_FORMAT_R8G8B8A8_UNORM).CreateGPU(101);

    // RWBuffer<float4> buffer : register(u0, space0); // GPU = 200
    // RWBuffer<float4> unusedBuffer : register(u0, space0); // GPU = 201
    ID3D12ResourcePtr uav = MakeBuffer().Size(1024).UAV();
    uav->SetName(L"UAV");
    D3D12ViewCreator uavView = MakeUAV(uav).Format(DXGI_FORMAT_R32G32B32A32_FLOAT).NumElements(16);
    uavView.CreateGPU(200);
    ID3D12ResourcePtr unusedUAV = MakeBuffer().Size(1024).UAV();
    unusedUAV->SetName(L"Unused UAV");
    D3D12ViewCreator unusedUAVView =
        MakeUAV(unusedUAV).Format(DXGI_FORMAT_R32G32B32A32_FLOAT).NumElements(16);
    unusedUAVView.CreateGPU(201);

    // struct Data
    //{
    //   float4 data;
    // };
    // ConstantBuffer<Data> cbv : register(b0, space0); // GPU = 300
    // ConstantBuffer<Data> unusedCBV : register(b0, space0); // GPU = 301
    struct Data
    {
      Vec4f data;
    };
    Data datas[16];
    datas[0].data = {16.0f, 32.0f, 64.0f, 128.0f};
    ID3D12ResourcePtr cbv = MakeBuffer().Size(16 * sizeof(Data)).Data(datas).Upload();
    setName(cbv, "CBV");
    D3D12ViewCreator cbvView = MakeCBV(cbv).SizeBytes(256);
    D3D12_GPU_DESCRIPTOR_HANDLE cbvGPU = cbvView.CreateGPU(300);
    ID3D12ResourcePtr unusedCBV = MakeBuffer().Size(16 * sizeof(Data)).Data(datas).Upload();
    setName(unusedCBV, "Unused CBV");
    D3D12ViewCreator unusedCBVView = MakeCBV(unusedCBV).SizeBytes(256);
    unusedCBVView.CreateGPU(301);

    // SamplerState heapSampler : register(s0, space0); // GPU = 0
    UINT increment = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    D3D12_CPU_DESCRIPTOR_HANDLE samplerStart = m_Sampler->GetCPUDescriptorHandleForHeapStart();
    D3D12_SAMPLER_DESC pointSamplerDesc = {};
    pointSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    pointSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    dev->CreateSampler(&pointSamplerDesc, {samplerStart.ptr});
    // SamplerState unusedHeapSampler : register(s1, space0); // GPU = 1
    dev->CreateSampler(&pointSamplerDesc, {samplerStart.ptr + (increment * 1)});

    // ByteAddressBuffer rootSRV : register(t0, space0);
    ID3D12ResourcePtr rootSRV = MakeBuffer().Data(datas);
    setName(rootSRV, "Root SRV");

    // RWStructuredBuffer<Data> rootUAV : register(u0, space0);
    ID3D12ResourcePtr rootUAV = MakeBuffer().Data(datas);
    setName(rootUAV, "Root UAV");

    // ConstantBuffer<Data> rootCBV : register(b0, space0);
    ID3D12ResourcePtr rootCBV = MakeBuffer().Data(datas);
    setName(rootCBV, "Root CBV");

    datas[0].data = {4.0f, 2.0f, 1.0f, 9.0f};
    ID3D12ResourcePtr eiRootSRV = MakeBuffer().Data(datas);
    setName(eiRootSRV, "EI Root SRV");

    datas[0].data = {16.0f, 64.0f, 8.0f, 8.0f};
    ID3D12ResourcePtr eiRootUAV = MakeBuffer().Data(datas);
    setName(eiRootUAV, "EI Root UAV");

    datas[0].data = {64.0f, 16.0f, 32.0f, 64.0f};
    ID3D12ResourcePtr eiRootCBV = MakeBuffer().Data(datas);
    setName(eiRootCBV, "EI Root CBV");

    sqSize = float(screenHeight) / 2.0f;

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 1;
    desc.NumDescriptors = 128;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ID3D12FencePtr fence;
    dev->CreateFence(0, D3D12_FENCE_FLAG_SHARED, __uuidof(ID3D12Fence), (void **)&fence);
    setName(fence, "Default Fence");
    setName(queue, "Test Queue");

    ID3D12CommandSignaturePtr eiSig =
        MakeCommandSig(sig, {vbArg(0), ibArg(), srvArg(0), uavArg(1), cbvArg(2), drawIndexedArg()});

    struct EIArgs
    {
      D3D12_VERTEX_BUFFER_VIEW vb;
      D3D12_INDEX_BUFFER_VIEW ib;
      D3D12_GPU_VIRTUAL_ADDRESS srv;
      D3D12_GPU_VIRTUAL_ADDRESS uav;
      D3D12_GPU_VIRTUAL_ADDRESS cbv;
      D3D12_DRAW_INDEXED_ARGUMENTS draw;
    } eiArgs;

    eiArgs.vb.BufferLocation = eiVB->GetGPUVirtualAddress();
    eiArgs.vb.SizeInBytes = UINT(eiVB->GetDesc().Width);
    eiArgs.vb.StrideInBytes = sizeof(DefaultA2V);
    eiArgs.ib.BufferLocation = eiIB->GetGPUVirtualAddress();
    eiArgs.ib.SizeInBytes = UINT(eiIB->GetDesc().Width);
    eiArgs.ib.Format = DXGI_FORMAT_R16_UINT;
    eiArgs.srv = eiRootSRV->GetGPUVirtualAddress();
    eiArgs.uav = eiRootUAV->GetGPUVirtualAddress();
    eiArgs.cbv = eiRootCBV->GetGPUVirtualAddress();
    eiArgs.draw.IndexCountPerInstance = 3;
    eiArgs.draw.InstanceCount = 1;
    eiArgs.draw.StartIndexLocation = 0;
    eiArgs.draw.BaseVertexLocation = 0;
    eiArgs.draw.StartInstanceLocation = 0;

    std::vector<char> eiArgsData;
    eiArgsData.resize(sizeof(EIArgs));

    char *ptr = eiArgsData.data();
    memcpy(ptr, &eiArgs.vb, sizeof(D3D12_VERTEX_BUFFER_VIEW));
    ptr += sizeof(D3D12_VERTEX_BUFFER_VIEW);
    memcpy(ptr, &eiArgs.ib, sizeof(D3D12_VERTEX_BUFFER_VIEW));
    ptr += sizeof(D3D12_INDEX_BUFFER_VIEW);
    memcpy(ptr, &eiArgs.srv, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    ptr += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    memcpy(ptr, &eiArgs.uav, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    ptr += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    memcpy(ptr, &eiArgs.cbv, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    ptr += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    memcpy(ptr, &eiArgs.draw, sizeof(D3D12_DRAW_ARGUMENTS));
    ptr += sizeof(D3D12_DRAW_ARGUMENTS);

    ID3D12ResourcePtr eiArgBuf = MakeBuffer().Upload().Data(eiArgsData);
    setName(eiArgBuf, "EI Argument Buffer");

    while(Running())
    {
      UINT64 counter = 1;

      viewXY.x = 0.0f;
      viewXY.y = 0.0f;

      {
        ID3D12GraphicsCommandListPtr startCmd = GetCommandBuffer();
        setName(startCmd, "Helper Command List: Start");

        Reset(startCmd);
        setMarker(startCmd, "Resource Usage: Start");
        startCmd->Close();
        Submit({startCmd});
      }

      ID3D12GraphicsCommandListPtr cmd = GetCommandBuffer();
      setName(cmd, "Main Command List");

      Reset(cmd);
      ID3D12ResourcePtr bb = StartUsingBackbuffer(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
      setName(bb, "BackBuffer");

      D3D12_CPU_DESCRIPTOR_HANDLE rtv =
          MakeRTV(bb).Format(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).CreateCPU(0);

      ClearRenderTargetView(cmd, rtv, {0.2f, 0.2f, 0.2f, 1.0f});

      cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      D3D12_VERTEX_BUFFER_VIEW vbs[2] = {};
      vbs[0].BufferLocation = vb->GetGPUVirtualAddress();
      vbs[0].SizeInBytes = sizeof(DefaultTri);
      vbs[0].StrideInBytes = sizeof(DefaultA2V);
      vbs[1].BufferLocation = unusedVb->GetGPUVirtualAddress();
      vbs[1].SizeInBytes = sizeof(DefaultTri);
      vbs[1].StrideInBytes = sizeof(DefaultA2V);
      cmd->IASetVertexBuffers(0, 2, vbs);

      IASetIndexBuffer(cmd, unusedIb, DXGI_FORMAT_R16_UINT, 0);

      ID3D12DescriptorHeap *heaps[] = {
          m_CBVUAVSRV.GetInterfacePtr(),
          m_Sampler.GetInterfacePtr(),
      };
      cmd->SetDescriptorHeaps(2, heaps);

      cmd->SetPipelineState(pso);
      cmd->SetGraphicsRootSignature(sig);
      cmd->SetGraphicsRootShaderResourceView(0, rootSRV->GetGPUVirtualAddress());
      cmd->SetGraphicsRootUnorderedAccessView(1, rootUAV->GetGPUVirtualAddress());
      cmd->SetGraphicsRootConstantBufferView(2, rootCBV->GetGPUVirtualAddress());
      cmd->SetGraphicsRootDescriptorTable(3, m_CBVUAVSRV->GetGPUDescriptorHandleForHeapStart());
      cmd->SetGraphicsRootDescriptorTable(4, m_CBVUAVSRV->GetGPUDescriptorHandleForHeapStart());
      cmd->SetGraphicsRootDescriptorTable(5, m_CBVUAVSRV->GetGPUDescriptorHandleForHeapStart());
      cmd->SetGraphicsRootDescriptorTable(6, m_Sampler->GetGPUDescriptorHandleForHeapStart());

      OMSetRenderTargets(cmd, {rtv}, {});
      RSSetScissorRect(cmd, {0, 0, screenWidth, screenHeight});
      RSSetViewport(cmd, {viewXY.x, viewXY.y, sqSize, sqSize, 0.0f, 1.0f});

      setMarker(cmd, "Draw");
      cmd->DrawInstanced(3, 1, 0, 0);
      NextTest();
      RSSetViewport(cmd, {viewXY.x, viewXY.y, sqSize, sqSize, 0.0f, 1.0f});

      IASetIndexBuffer(cmd, ib, DXGI_FORMAT_R16_UINT, 0);
      cmd->DrawIndexedInstanced(3, 1, 0, 0, 0);
      NextTest();
      RSSetViewport(cmd, {viewXY.x, viewXY.y, sqSize, sqSize, 0.0f, 1.0f});

      setMarker(cmd, "Indirect");
      cmd->ExecuteIndirect(eiSig, 1, eiArgBuf, 0, NULL, 0);
      NextTest();
      RSSetViewport(cmd, {viewXY.x, viewXY.y, sqSize, sqSize, 0.0f, 1.0f});

      setMarker(cmd, "Draw After Indirect");
      cmd->SetGraphicsRootShaderResourceView(0, rootSRV->GetGPUVirtualAddress());
      cmd->SetGraphicsRootUnorderedAccessView(1, rootUAV->GetGPUVirtualAddress());
      cmd->SetGraphicsRootConstantBufferView(2, rootCBV->GetGPUVirtualAddress());
      cmd->IASetVertexBuffers(0, 2, vbs);
      IASetIndexBuffer(cmd, ib, DXGI_FORMAT_R16_UINT, 0);
      cmd->DrawIndexedInstanced(3, 1, 0, 0, 0);
      NextTest();
      RSSetViewport(cmd, {viewXY.x, viewXY.y, sqSize, sqSize, 0.0f, 1.0f});

      cmd->SetDescriptorHeaps(1, heaps);
      cmd->SetPipelineState(computePSO);
      cmd->SetComputeRootSignature(computeSig);
      cmd->SetComputeRootDescriptorTable(0, m_CBVUAVSRV->GetGPUDescriptorHandleForHeapStart());
      cmd->SetComputeRootDescriptorTable(1, m_CBVUAVSRV->GetGPUDescriptorHandleForHeapStart());
      cmd->SetComputeRootDescriptorTable(2, m_CBVUAVSRV->GetGPUDescriptorHandleForHeapStart());
      setMarker(cmd, "Dispatch");
      cmd->Dispatch(1, 1, 1);

      FinishUsingBackbuffer(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);

      queue->Signal(fence, counter);
      counter++;

      cmd->Close();

      Submit({cmd});

      {
        ID3D12GraphicsCommandListPtr endCmd = GetCommandBuffer();
        setName(endCmd, "Helper Command List: Start");

        Reset(endCmd);
        setMarker(endCmd, "Resource Usage: End");
        endCmd->Close();
        Submit({endCmd});
      }
      Present();
    }

    return 0;
  }
};

REGISTER_TEST();
