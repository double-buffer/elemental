#pragma once

#include "ElementalTools.h"
#include "SystemMemory.h"

struct GenerateTangentVectorsParameters
{
    ElemToolsVector3* PositionPointer;
    ElemToolsVector3* NormalPointer;
    ElemToolsVector2* TextureCoordinatesPointer;
    ElemToolsVector4* TangentPointer;
    uint32_t VertexSize;
    ReadOnlySpan<uint32_t> IndexData;
};

bool GenerateTangentVectors(const GenerateTangentVectorsParameters* parameters);
