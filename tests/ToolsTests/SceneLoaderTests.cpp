#include "ToolsTests.h"
#include "utest.h"

// TODO: Add real test files on disk?

auto cubeObjSceneSource = R"(o Cube
    v 1.000000 -1.000000 -1.000000
    v 1.000000 -1.000000 1.000000
    v -1.000000 -1.000000 1.000000
    v -1.000000 -1.000000 -1.000000
    v 1.000000 1.000000 -0.999999
    v 0.999999 1.000000 1.000001
    v -1.000000 1.000000 1.000000
    v -1.000000 1.000000 -1.000000
    vt 1.000000 0.333333
    vt 1.000000 0.666667
    vt 0.666667 0.666667
    vt 0.666667 0.333333
    vt 0.666667 0.000000
    vt 0.000000 0.333333
    vt 0.000000 0.000000
    vt 0.333333 0.000000
    vt 0.333333 1.000000
    vt 0.000000 1.000000
    vt 0.000000 0.666667
    vt 0.333333 0.333333
    vt 0.333333 0.666667
    vt 1.000000 0.000000
    vn 0.000000 -1.000000 0.000000
    vn 0.000000 1.000000 0.000000
    vn 1.000000 0.000000 0.000000
    vn -0.000000 0.000000 1.000000
    vn -1.000000 -0.000000 -0.000000
    vn 0.000000 0.000000 -1.000000
    s off
    f 2/1/1 3/2/1 4/3/1
    f 8/1/2 7/4/2 6/5/2
    f 5/6/3 6/7/3 2/8/3
    f 6/8/4 7/5/4 3/4/4
    f 3/9/5 7/10/5 8/11/5
    f 1/12/6 4/13/6 8/11/6
    f 1/4/1 2/1/1 4/3/1
    f 5/14/2 8/1/2 6/5/2
    f 1/12/3 5/6/3 2/8/3
    f 2/12/4 6/8/4 3/4/4
    f 4/13/5 3/9/5 8/11/5
    f 5/6/6 1/12/6 8/11/6)";

struct SceneLoader_LoadScene
{
    const char* Path;
    ElemSceneFormat SceneFormat;
    uint32_t ExpectedNodeCount;
    uint32_t ExpectedMeshCount;
    uint32_t ExpectedVertexCount;
};

UTEST_F_SETUP(SceneLoader_LoadScene)
{
    AddTestFile("Cube.obj", { .Items = (uint8_t*)cubeObjSceneSource, .Length = (uint32_t)strlen(cubeObjSceneSource) });
}

UTEST_F_TEARDOWN(SceneLoader_LoadScene)
{
    // Act
    auto result = ElemLoadScene(utest_fixture->Path, NULL);

    // Assert
    ASSERT_FALSE(result.HasErrors);

    ASSERT_EQ_MSG(result.SceneFormat, utest_fixture->SceneFormat, "Scene format is invalid.");

    // TODO: 
    ASSERT_EQ_MSG(result.Nodes.Length, utest_fixture->ExpectedNodeCount, "Node count is not correct."); 
    ASSERT_EQ_MSG(result.Meshes.Length, utest_fixture->ExpectedMeshCount, "Mesh count is not correct."); 

    ElemSceneMesh mesh = result.Meshes.Items[0];
    ElemSceneMeshPrimitive meshPart = mesh.MeshPrimitives.Items[0];

    ASSERT_EQ_MSG(meshPart.VertexBuffer.VertexCount, utest_fixture->ExpectedVertexCount, "MeshPrimitive vertex count is not correct."); 
    ASSERT_GT_MSG(meshPart.VertexBuffer.VertexSize, 0u, "Vertex size must be greater than 0.");
    ASSERT_GT_MSG(meshPart.VertexBuffer.Data.Length, 0u, "VertexBuffer size must be greater than 0.");

    bool isEmpty = true;

    for (uint32_t i = 0; i < meshPart.VertexBuffer.Data.Length; i++)
    {
        if (meshPart.VertexBuffer.Data.Items[i] != 0)
        {
            isEmpty = false;
            break;
        }
    }

    ASSERT_FALSE_MSG(isEmpty, "VertexBuffer must contains data.");

    // TODO: Add more data checks based on the format
}

UTEST_F(SceneLoader_LoadScene, Obj) 
{
    utest_fixture->Path = "Cube.obj";
    utest_fixture->SceneFormat = ElemSceneFormat_Obj;
    utest_fixture->ExpectedMeshCount = 1;
    utest_fixture->ExpectedNodeCount = 1;

    // TODO: To change
    utest_fixture->ExpectedVertexCount = 36;
}
