#pragma once

#include "Elemental.h"

struct DirectX12TextureData
{
};

struct DirectX12TextureDataFull
{
};

DirectX12TextureData* GetDirectX12TextureData(ElemTexture texture);
DirectX12TextureDataFull* GetDirectX12TextureDataFull(ElemTexture texture);

void InitDirectX12TextureMemory();
