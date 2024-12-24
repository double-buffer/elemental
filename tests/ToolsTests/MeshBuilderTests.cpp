#include "ToolsTests.h"
#include "utest.h"

// TODO: Add check when passing only vertex buffer for mod 3 
// TODO: Test cone and bounding box and spheres

struct TestMeshVector2
{
    float X, Y;
};

struct TestMeshVector3
{
    float X, Y, Z;
};

struct TestMeshVertex
{
    TestMeshVector3 Position;
    TestMeshVector3 Normal;
    TestMeshVector2 TextureCoordinates;
};

ElemVertexBuffer TestBuildVertexBuffer(TestMeshVertex* destination, uint32_t count)
{
    float globalVertexCounter = 0u;

    for (uint32_t i = 0; i < count; i++)
    {
        destination[i] =
        {
            .Position = { globalVertexCounter++, globalVertexCounter++, globalVertexCounter++ },
            .Normal = { globalVertexCounter++, globalVertexCounter++, globalVertexCounter++ },
            .TextureCoordinates = { globalVertexCounter++, globalVertexCounter++ }
        };
    }
    
    return
    {
        .Data = { .Items = (uint8_t*)destination, .Length = (uint32_t)(count * sizeof(TestMeshVertex)) },
        .VertexSize = sizeof(TestMeshVertex),
        .VertexCount = count
    };
}

UTEST(MeshBuilder, CheckVertexBuffer) 
{
    // Arrange
    const uint32_t vertexCount = 210;
    TestMeshVertex vertexList[vertexCount];
    auto vertexBuffer = TestBuildVertexBuffer(vertexList, vertexCount);

    // Act
    auto result = ElemBuildMeshlets(vertexBuffer, NULL);

    // Assert
    ASSERT_FALSE(result.HasErrors);
    ASSERT_EQ_MSG(result.VertexBuffer.Data.Length / result.VertexBuffer.VertexSize, vertexCount, "Vertex Count is not correct.");

    for (uint32_t i = 0; i < vertexCount; i++)
    {
        auto found = false;
        auto testVertex = vertexList[i];

        for (uint32_t j = 0; j < result.VertexBuffer.Data.Length / result.VertexBuffer.VertexSize; j++)
        {
            auto resultVertex = (TestMeshVertex*)&result.VertexBuffer.Data.Items[j * result.VertexBuffer.VertexSize];

            if (testVertex.Position.X == resultVertex->Position.X &&
                testVertex.Position.Y == resultVertex->Position.Y &&
                testVertex.Position.Z == resultVertex->Position.Z &&
                testVertex.Normal.X == resultVertex->Normal.X &&
                testVertex.Normal.Y == resultVertex->Normal.Y &&
                testVertex.Normal.Z == resultVertex->Normal.Z &&
                testVertex.TextureCoordinates.X == resultVertex->TextureCoordinates.X &&
                testVertex.TextureCoordinates.Y == resultVertex->TextureCoordinates.Y)
            {
                found = true;
                break;
            }
        }

        ASSERT_TRUE(found);
    }
}

UTEST(MeshBuilder, CheckVertexBuffer_NoDuplicates) 
{
    // Arrange
    const uint32_t vertexCount = 210;
    TestMeshVertex vertexList[vertexCount];
    auto vertexBuffer = TestBuildVertexBuffer(vertexList, vertexCount - 3);

    for (uint32_t i = 0; i < 3; i++)
    {
        vertexList[vertexCount - 3 + i] = vertexList[vertexCount - 6 + i];
    }

    vertexBuffer.Data.Length += vertexBuffer.VertexSize * 3;

    // Act
    auto result = ElemBuildMeshlets(vertexBuffer, NULL);

    // Assert
    ASSERT_FALSE(result.HasErrors);
    ASSERT_EQ_MSG(result.VertexBuffer.Data.Length / result.VertexBuffer.VertexSize, vertexCount - 3, "Vertex Count from vertexbuffer length is not correct.");
    ASSERT_EQ_MSG(result.VertexBuffer.VertexCount, vertexCount - 3, "Vertex Count is not correct.");
}

UTEST(MeshBuilder, CheckMeshletVertexIndexBuffer) 
{
    // Arrange
    const uint32_t vertexCount = 210;
    TestMeshVertex vertexList[vertexCount];
    auto vertexBuffer = TestBuildVertexBuffer(vertexList, vertexCount - 3);

    // Act
    auto result = ElemBuildMeshlets(vertexBuffer, NULL);

    // Assert
    ASSERT_FALSE(result.HasErrors);

    for (uint32_t i = 0; i < result.Meshlets.Length; i++)
    {
        ElemMeshlet meshlet = result.Meshlets.Items[i];

        if (i < result.Meshlets.Length - 1) 
        {
            ElemMeshlet nextMeshlet = result.Meshlets.Items[i + 1];
            ASSERT_NE_MSG(meshlet.VertexIndexOffset + meshlet.VertexIndexCount - 1, nextMeshlet.VertexIndexOffset, "Error Not Last");
        }
        else 
        {
            ASSERT_NE_MSG(meshlet.VertexIndexOffset + meshlet.VertexIndexCount - 1, result.MeshletVertexIndexBuffer.Length, "Error");
        }
    }
}

UTEST(MeshBuilder, CheckMeshletTriangleIndexBuffer) 
{
    // Arrange
    const uint32_t vertexCount = 210;
    TestMeshVertex vertexList[vertexCount];
    auto vertexBuffer = TestBuildVertexBuffer(vertexList, vertexCount - 3);

    // Act
    auto result = ElemBuildMeshlets(vertexBuffer, NULL);

    // Assert
    ASSERT_FALSE(result.HasErrors);

    for (uint32_t i = 0; i < result.Meshlets.Length; i++)
    {
        ElemMeshlet meshlet = result.Meshlets.Items[i];

        if (i < result.Meshlets.Length - 1) 
        {
            ElemMeshlet nextMeshlet = result.Meshlets.Items[i + 1];
            ASSERT_NE_MSG(meshlet.TriangleOffset + meshlet.TriangleCount - 1, nextMeshlet.TriangleOffset, "Error Not Last");
        }
        else 
        {
            ASSERT_NE_MSG(meshlet.TriangleOffset + meshlet.TriangleCount - 1, result.MeshletTriangleIndexBuffer.Length, "Error");
        }
    }
}
