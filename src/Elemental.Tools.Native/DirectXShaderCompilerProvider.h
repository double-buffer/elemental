#pragma once
#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"

#include "dxcapi.h"
#include "d3d12shader.h"

class DirectXShaderCompilerProvider : ShaderCompilerProvider
{
public:
    DirectXShaderCompilerProvider();
    ~DirectXShaderCompilerProvider() override;

    ShaderLanguage GetShaderLanguage() override;
    void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount) override;

    bool IsCompilerInstalled() override;
    ShaderCompilerResult CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi) override;

private:
    HMODULE _dxcompilerDll = nullptr;
    DxcCreateInstanceProc _createInstanceFunc = nullptr;
};

enum class DxilRootSignatureVersion : uint32_t {
  Version_1 = 1,
  Version_1_0 = 1,
  Version_1_1 = 2
};
enum class DxilRootSignatureCompilationFlags {
  None = 0x0,
  LocalRootSignature = 0x1,
  GlobalRootSignature = 0x2,
};
enum class DxilRootSignatureFlags : uint32_t {
  None = 0,
  AllowInputAssemblerInputLayout = 0x1,
  DenyVertexShaderRootAccess = 0x2,
  DenyHullShaderRootAccess = 0x4,
  DenyDomainShaderRootAccess = 0x8,
  DenyGeometryShaderRootAccess = 0x10,
  DenyPixelShaderRootAccess = 0x20,
  AllowStreamOutput = 0x40,
  LocalRootSignature = 0x80,
  DenyAmplificationShaderRootAccess = 0x100,
  DenyMeshShaderRootAccess = 0x200,
  CBVSRVUAVHeapDirectlyIndexed = 0x400,
  SamplerHeapDirectlyIndexed = 0x800,
  AllowLowTierReservedHwCbLimit = 0x80000000,
  ValidFlags = 0x80000fff
};
enum class DxilRootParameterType {
  DescriptorTable = 0,
  Constants32Bit = 1,
  CBV = 2,
  SRV = 3,
  UAV = 4,
  MaxValue = 4
};
enum class DxilFilter {
  // TODO: make these names consistent with code convention
  MIN_MAG_MIP_POINT = 0,
  MIN_MAG_POINT_MIP_LINEAR = 0x1,
  MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
  MIN_POINT_MAG_MIP_LINEAR = 0x5,
  MIN_LINEAR_MAG_MIP_POINT = 0x10,
  MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
  MIN_MAG_LINEAR_MIP_POINT = 0x14,
  MIN_MAG_MIP_LINEAR = 0x15,
  ANISOTROPIC = 0x55,
  COMPARISON_MIN_MAG_MIP_POINT = 0x80,
  COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
  COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
  COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
  COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
  COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
  COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
  COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
  COMPARISON_ANISOTROPIC = 0xd5,
  MINIMUM_MIN_MAG_MIP_POINT = 0x100,
  MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
  MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
  MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
  MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
  MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
  MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
  MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
  MINIMUM_ANISOTROPIC = 0x155,
  MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
  MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
  MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
  MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
  MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
  MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
  MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
  MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
  MAXIMUM_ANISOTROPIC = 0x1d5
};
enum class DxilShaderVisibility {
  All = 0,
  Vertex = 1,
  Hull = 2,
  Domain = 3,
  Geometry = 4,
  Pixel = 5,
  Amplification = 6,
  Mesh = 7,
  MaxValue = 7
};
enum class DxilStaticBorderColor {
  TransparentBlack = 0,
  OpaqueBlack = 1,
  OpaqueWhite = 2,
  OpaqueBlackUint = 3,
  OpaqueWhiteUint = 4
};
enum class DxilTextureAddressMode {
  Wrap = 1,
  Mirror = 2,
  Clamp = 3,
  Border = 4,
  MirrorOnce = 5
};

struct DxilContainerRootDescriptor1
{
  uint32_t ShaderRegister;
  uint32_t RegisterSpace;
  uint32_t Flags;
};
struct DxilContainerDescriptorRange
{
  uint32_t RangeType;
  uint32_t NumDescriptors;
  uint32_t BaseShaderRegister;
  uint32_t RegisterSpace;
  uint32_t OffsetInDescriptorsFromTableStart;
};
struct DxilContainerDescriptorRange1
{
  uint32_t RangeType;
  uint32_t NumDescriptors;
  uint32_t BaseShaderRegister;
  uint32_t RegisterSpace;
  uint32_t Flags;
  uint32_t OffsetInDescriptorsFromTableStart;
};
struct DxilContainerRootDescriptorTable
{
  uint32_t NumDescriptorRanges;
  uint32_t DescriptorRangesOffset;
};
struct DxilContainerRootParameter
{
  uint32_t ParameterType;
  uint32_t ShaderVisibility;
  uint32_t PayloadOffset;
};
struct DxilContainerRootSignatureDesc
{
  uint32_t Version;
  uint32_t NumParameters;
  uint32_t RootParametersOffset;
  uint32_t NumStaticSamplers;
  uint32_t StaticSamplersOffset;
  uint32_t Flags;
};

struct DxilRootConstants {
  uint32_t ShaderRegister;
  uint32_t RegisterSpace;
  uint32_t Num32BitValues;
};