#include "TangentSpaceGenerator.h"

int32_t MikkTSpaceGetNumFaces(const SMikkTSpaceContext* context) 
{
    auto parameters = (GenerateTangentVectorsParameters*)context->m_pUserData;
    return parameters->IndexData.Length / 3;
}

int32_t MikkTSpaceGetNumVerticesOfFace(const SMikkTSpaceContext* context, int32_t face) 
{
    return 3;
}

void MikkTSpaceGetPosition(const SMikkTSpaceContext* context, float* outputPosition, int32_t faceIndex, int32_t vertexIndex) 
{
    auto parameters = (GenerateTangentVectorsParameters*)context->m_pUserData;

    auto vertexGlobalIndex = parameters->IndexData[faceIndex * 3 + vertexIndex];
    auto position = (ElemToolsVector3*)((uint8_t*)parameters->PositionPointer + vertexGlobalIndex * parameters->VertexSize);

    outputPosition[0] = position->X;
    outputPosition[1] = position->Y;
    outputPosition[2] = position->Z;
}

void MikkTSpaceGetNormal(const SMikkTSpaceContext* context, float* outputNormal, int32_t faceIndex, int32_t vertexIndex) 
{
    auto parameters = (GenerateTangentVectorsParameters*)context->m_pUserData;

    auto vertexGlobalIndex = parameters->IndexData[faceIndex * 3 + vertexIndex];
    auto normal = (ElemToolsVector3*)((uint8_t*)parameters->NormalPointer + vertexGlobalIndex * parameters->VertexSize);

    outputNormal[0] = normal->X;
    outputNormal[1] = normal->Y;
    outputNormal[2] = normal->Z;
}

void MikkTSpaceGetTextureCoordinates(const SMikkTSpaceContext* context, float* outputTextureCoordinates, int32_t faceIndex, int32_t vertexIndex) 
{
    auto parameters = (GenerateTangentVectorsParameters*)context->m_pUserData;

    auto vertexGlobalIndex = parameters->IndexData[faceIndex * 3 + vertexIndex];
    auto textureCoordinates = (ElemToolsVector2*)((uint8_t*)parameters->TextureCoordinatesPointer + vertexGlobalIndex * parameters->VertexSize);

    outputTextureCoordinates[0] = textureCoordinates->X;
    outputTextureCoordinates[1] = textureCoordinates->Y;
}

void MikkTSpaceSetTangent(const SMikkTSpaceContext* context, const float* tangent, float sign, int32_t faceIndex, int32_t vertexIndex) 
{
    auto parameters = (GenerateTangentVectorsParameters*)context->m_pUserData;

    auto vertexGlobalIndex = parameters->IndexData[faceIndex * 3 + vertexIndex];
    auto outputTangent = (ElemToolsVector4*)((uint8_t*)parameters->TangentPointer + vertexGlobalIndex * parameters->VertexSize);

    outputTangent->X = tangent[0];
    outputTangent->Y = tangent[1];
    outputTangent->Z = tangent[2];
    outputTangent->W = sign;
}

bool GenerateTangentVectors(const GenerateTangentVectorsParameters* parameters)
{
    SMikkTSpaceInterface mikkTSpaceInterface
    {
        .m_getNumFaces = MikkTSpaceGetNumFaces,
        .m_getNumVerticesOfFace = MikkTSpaceGetNumVerticesOfFace,
        .m_getPosition = MikkTSpaceGetPosition,
        .m_getNormal = MikkTSpaceGetNormal,
        .m_getTexCoord = MikkTSpaceGetTextureCoordinates,
        .m_setTSpaceBasic = MikkTSpaceSetTangent
    };

    SMikkTSpaceContext mikkTSpaceContext
    {
        .m_pInterface = &mikkTSpaceInterface,
        .m_pUserData = (void*)parameters
    };

    return genTangSpaceDefault(&mikkTSpaceContext);
}
