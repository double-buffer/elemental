#pragma once

enum class DxilRootParameterType 
{
  DescriptorTable = 0,
  Constants32Bit = 1,
  CBV = 2,
  SRV = 3,
  UAV = 4,
  MaxValue = 4
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

struct DxilRootConstants 
{
  uint32_t ShaderRegister;
  uint32_t RegisterSpace;
  uint32_t Num32BitValues;
};