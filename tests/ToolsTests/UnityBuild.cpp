#include "ToolsTests.cpp"
#include "ShaderCompilerTests.cpp"
#include "MeshLoaderTests.cpp"
#include "MeshBuilderTests.cpp"

UTEST_STATE();

int main(int argc, const char* argv[]) 
{
    ConfigureTestFileIO();
    return utest_main(argc, argv);
}
