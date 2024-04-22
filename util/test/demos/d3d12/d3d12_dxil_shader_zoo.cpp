/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2024- Baldur Karlsson
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

static std::string ComputeShader = R"EOSHADER(

cbuffer blah : register(b0)
{
  uint4 mult;
};

RWStructuredBuffer<uint4> bufIn : register(u0);
RWStructuredBuffer<uint4> bufOut : register(u1);

[numthreads(1,1,1)]
void main()
{
  float f32 = (float)mult.x;
  double f64 = (double)mult.y;
f32 = asin(f32);
f32 = acos(f32);
f32 = atan(f32);
f32 = ceil(f32);
f32 = clamp(f32, (float)mult.z, (float)mult.w);
f32 = sin(f32);
f32 = cos(f32);
f32 = tan(f32);
f32 = sinh(f32);
f32 = cosh(f32);
f32 = tanh(f32);

  bufOut[0].x += bufIn[0].x * mult.x;
  bufOut[0].y += bufIn[0].y * mult.y;
  bufOut[0].z += bufIn[0].z * mult.z;
  bufOut[0].w += bufIn[0].w * mult.w;
// asfloat()

bufOut[0].z += asuint(f32);
bufOut[0].w += asuint(f32);
/*
all()
AllMemoryBarrier()
AllMemoryBarrierWithGroupSync()
any()
atan2()
CheckAccessFullyMapped()
clip()
countbits()
cross()
D3DCOLORtoUBYTE4()
ddx()
ddx_coarse()
ddx_fine()
ddy()
ddy_coarse()
ddy_fine()
degrees()
determinant()
DeviceMemoryBarrier()
DeviceMemoryBarrierWithGroupSync()
distance()
dot()
dst()
errorf()
EvaluateAttributeCentroid()
EvaluateAttributeAtSample()
EvaluateAttributeSnapped()
exp()
exp2()
f16tof32()
f32tof16()
faceforward()
firstbithigh()
firstbitlow()
floor()
fma()
fmod()
frac()
frexp()
fwidth()
GetRenderTargetSampleCount()
GetRenderTargetSamplePosition()
GroupMemoryBarrier()
GroupMemoryBarrierWithGroupSync()
InterlockedAdd()
InterlockedAnd()
InterlockedCompareExchange()
InterlockedCompareStore()
InterlockedExchange()
InterlockedMax()
InterlockedOr()
InterlockedXor()
isfinite()
isinf()
isnan()
ldexp()
length()
lerp()
lit()
log()
log10()
log2()
mad()
max()
min()
modf()
msad4()
mul()
noise()
normalize()
pow()
printf()
Process2DQuadTessFactorsAvg()
Process2DQuadTessFactorsMax()
Process2DQuadTessFactorsMin()
ProcessIsolineTessFactors()
ProcessQuadTessFactorsAvg()
ProcessQuadTessFactorsMax()
ProcessQuadTessFactorsMin()
ProcessTriTessFactorsAvg()
ProcessTriTessFactorsMax()
ProcessTriTessFactorsMin()
radians()
rcp()
reflect()
reversebits()
round()
rsqrt()
sign()
sincos()
smoothstep()
sqrt()
step()
tex1D(s, t)
tex1D(s, t, ddx, ddy)
tex1Dbias()
tex1Dgrad()
tex1Dlod()
tex1Dproj()
tex2D(s, t)
tex2D(s, t, ddx, ddy)
tex2Dbias()
tex2Dgrad()
tex2Dlod()
tex2Dproj()
tex3D(s, t)
tex3D(s, t, ddx, ddy)
tex3Dbias()
tex3Dgrad()
tex3Dlod()
tex3Dproj()
texCUBE(s, t)
texCUBE(s, t, ddx, ddy)
texCUBEbias()
texCUBEgrad()
texCUBElod()
texCUBEproj()
transpose()
trunc()
*/
}

)EOSHADER";

static std::string VertexShader = R"EOSHADER(

struct vertIn
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

v2f main(vertIn IN)
{
	v2f OUT = (v2f)0;

	OUT.pos = float4(IN.pos.xyz, 1);
	OUT.col = IN.col;
	OUT.uv = IN.uv;

	return OUT;
}

)EOSHADER";

static std::string PixelShader = R"EOSHADER(

struct v2f
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv : TEXCOORD0;
};

float4 main(v2f IN) : SV_Target0
{
  if ((IN.pos.x > 180.0) && (IN.pos.x < 185.0) &&
      (IN.pos.y > 155.0) && (IN.pos.y < 165.0))
    return IN.col.xxxx;

	return IN.col;
}

)EOSHADER";

RD_TEST(D3D12_DXIL_Shader_Zoo, D3D12GraphicsTest)
{
  static constexpr const char *Description = "JAKE TODO";

  int main()
  {
    // initialise, create window, create device, etc
    if(!Init())
      return 3;

    ID3DBlobPtr vsblob = Compile(VertexShader, "main", "vs_6_0", false);
    ID3DBlobPtr psblob = Compile(PixelShader, "main", "ps_6_0", false);

    ID3D12RootSignaturePtr sigGraphics = MakeSig({});
    ID3D12PipelineStatePtr psoGraphics =
        MakePSO().RootSig(sigGraphics).InputLayout().VS(vsblob).PS(psblob);

    ID3D12ResourcePtr vertIn = MakeBuffer().Data(DefaultTri);
    vertIn->SetName(L"vertIn");
    ResourceBarrier(vertIn, D3D12_RESOURCE_STATE_COMMON,
                    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    ID3DBlobPtr csblob = Compile(ComputeShader, "main", "cs_6_0", false);
    ID3D12RootSignaturePtr sigCompute = MakeSig({
        uavParam(D3D12_SHADER_VISIBILITY_ALL, 0, 0),
        uavParam(D3D12_SHADER_VISIBILITY_ALL, 0, 1),
        constParam(D3D12_SHADER_VISIBILITY_ALL, 0, 0, 4),
        tableParam(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2, 1, 3),
    });
    ID3D12PipelineStatePtr psoCompute = MakePSO().RootSig(sigCompute).CS(csblob);

    ID3D12ResourcePtr bufIn = MakeBuffer().Size(1024).UAV();
    ID3D12ResourcePtr bufOut = MakeBuffer().Size(1024).UAV();
    bufIn->SetName(L"bufIn");
    bufOut->SetName(L"bufOut");

    ID3D12ResourcePtr rtvtex = MakeTexture(DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 4)
                                   .RTV()
                                   .InitialState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    rtvtex->SetName(L"rtvtex");

    uint32_t a[4] = {111, 111, 111, 111};
    uint32_t b[4] = {222, 222, 222, 222};
    D3D12_RECT rect = {};
    rect.right = 1024;
    rect.bottom = 1;

    D3D12_GPU_DESCRIPTOR_HANDLE bufInGPU =
        MakeUAV(bufIn).Format(DXGI_FORMAT_R32G32B32A32_UINT).CreateGPU(0);
    D3D12_CPU_DESCRIPTOR_HANDLE bufInClearCPU =
        MakeUAV(bufIn).Format(DXGI_FORMAT_R32G32B32A32_UINT).CreateClearCPU(0);
    D3D12_GPU_DESCRIPTOR_HANDLE bufOutGPU =
        MakeUAV(bufOut).Format(DXGI_FORMAT_R32G32B32A32_UINT).CreateGPU(1);
    D3D12_CPU_DESCRIPTOR_HANDLE bufOutClearCPU =
        MakeUAV(bufOut).Format(DXGI_FORMAT_R32G32B32A32_UINT).CreateClearCPU(1);

    D3D12_GPU_VIRTUAL_ADDRESS bufInVA = bufIn->GetGPUVirtualAddress();
    D3D12_GPU_VIRTUAL_ADDRESS bufOutVA = bufOut->GetGPUVirtualAddress();

    while(Running())
    {
      ID3D12GraphicsCommandListPtr cmd = GetCommandBuffer();
      Reset(cmd);

      {
        ID3D12ResourcePtr bb = StartUsingBackbuffer(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv =
            MakeRTV(bb).Format(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).CreateCPU(0);

        ClearRenderTargetView(cmd, rtv, {0.2f, 0.2f, 0.2f, 1.0f});

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        IASetVertexBuffer(cmd, vertIn, sizeof(DefaultA2V), 0);
        cmd->SetPipelineState(psoGraphics);
        cmd->SetGraphicsRootSignature(sigGraphics);

        RSSetViewport(cmd, {0.0f, 0.0f, (float)screenWidth, (float)screenHeight, 0.0f, 1.0f});
        RSSetScissorRect(cmd, {0, 0, screenWidth, screenHeight});
        OMSetRenderTargets(cmd, {rtv}, {});

        setMarker(cmd, "Draw");
        cmd->DrawInstanced(3, 1, 0, 0);

        FinishUsingBackbuffer(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
      }

      {
        cmd->SetDescriptorHeaps(1, &m_CBVUAVSRV.GetInterfacePtr());

        cmd->ClearUnorderedAccessViewUint(bufInGPU, bufInClearCPU, bufIn, a, 1, &rect);
        cmd->ClearUnorderedAccessViewUint(bufOutGPU, bufOutClearCPU, bufOut, b, 1, &rect);

        cmd->SetComputeRootSignature(sigCompute);
        cmd->SetPipelineState(psoCompute);
        cmd->SetComputeRootUnorderedAccessView(0, bufInVA);
        cmd->SetComputeRootUnorderedAccessView(1, bufOutVA);
        cmd->SetComputeRoot32BitConstant(2, 5, 0);
        cmd->SetComputeRoot32BitConstant(2, 6, 1);
        cmd->SetComputeRoot32BitConstant(2, 7, 2);
        cmd->SetComputeRoot32BitConstant(2, 8, 3);
        cmd->SetComputeRootDescriptorTable(3, m_CBVUAVSRV->GetGPUDescriptorHandleForHeapStart());
        setMarker(cmd, "Compute");
        cmd->Dispatch(1, 1, 1);
      }

      cmd->Close();
      Submit({cmd});

      Present();
    }

    return 0;
  }
};

REGISTER_TEST();
