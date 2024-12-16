#include "ToolsTests.h"
#include "utest.h"

auto objMeshSourceFile = R"(
    o Cube
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

struct MeshLoader_LoadMesh
{
    ElemMeshFormat MeshFormat;
    ElemToolsDataSpan MeshSource;
    uint32_t ExpectedVertexCount;
};

UTEST_F_SETUP(MeshLoader_LoadMesh)
{
}

UTEST_F_TEARDOWN(MeshLoader_LoadMesh)
{
    // Act
    auto result = ElemLoadMesh(utest_fixture->MeshSource, utest_fixture->MeshFormat, NULL);

    // Assert
    ASSERT_FALSE(result.HasErrors);
    ASSERT_EQ_MSG(result.VertexCount, utest_fixture->ExpectedVertexCount, "LoadMesh vertex count is not correct."); 
}

UTEST_F(MeshLoader_LoadMesh, Obj) 
{
    utest_fixture->MeshFormat = ElemMeshFormat_Obj;
    utest_fixture->MeshSource = { .Items = (uint8_t*)objMeshSourceFile, .Length = (uint32_t)strlen(objMeshSourceFile) };
    utest_fixture->ExpectedVertexCount = 8;
}
