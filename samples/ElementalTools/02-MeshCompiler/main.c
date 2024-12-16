#include "ElementalTools.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleMesh.h"

/**
 * WARNING: This obj parsing code is just for demonstration purpose only. The parsing is not feature complete
 * and the implementation focus on readability and not performance. Don't use it in your production code!
 */

typedef struct
{
    uint32_t SubObjectCount;
    uint32_t VertexCount;
    uint32_t NormalCount;
    uint32_t TextCoordCount;
    uint32_t FaceCount;
} ObjMeshElementCount;

typedef struct 
{
    uint32_t VertexIndex;
    uint32_t TextCoordIndex;
    uint32_t NormalIndex;
} ObjMeshVertex;

typedef struct
{
    union
    {
        ObjMeshVertex Vertex;        
        uint32_t Indices[3];
    } Elements[3];
} ObjMeshFace;

typedef struct
{
    SampleVector3 Position;
    SampleVector3 Normal;
    SampleVector2 TextureCoordinates;
} InputMeshVertex;

typedef struct
{
    InputMeshVertex* VertexList;
    uint32_t VertexCount;
} InputMeshData;

ObjMeshElementCount CountObjMeshElements(ElemToolsDataSpan data)
{
    ElemToolsDataSpan line = SampleReadLine(&data);

    ObjMeshElementCount result = { .VertexCount = 1, .NormalCount = 1, .TextCoordCount = 1 };

    while (line.Length > 0)
    {
        if (line.Length > 2)
        {
            if (line.Items[0] == 'g')
            {
                result.SubObjectCount++;
            }
            else if (line.Items[0] == 'v' && line.Items[1] == 'n')
            {
                result.NormalCount++;
            }
            else if (line.Items[0] == 'v' && line.Items[1] == 't')
            {
                result.TextCoordCount++;
            }
            else if (line.Items[0] == 'v')
            {
                result.VertexCount++;
            }
            else if (line.Items[0] == 'f')
            {
                result.FaceCount++;
            }
        }

        line = SampleReadLine(&data);
    }

    printf("SubObject: %d\n", result.SubObjectCount);
    printf("Vextex: %d\n", result.VertexCount);
    printf("Normal: %d\n", result.NormalCount);
    printf("TestCoord: %d\n", result.TextCoordCount);
    printf("Face: %d\n", result.FaceCount);

    return result;
}

SampleVector2 ReadObjVector2(ElemToolsDataSpan* lineParts, uint32_t linePartCount)
{
    if (linePartCount < 3)
    {
        printf("Error: Invalid number of elements in the line for Vector2.\n");
        return (SampleVector2) { 0 };
    }

    float x = atof((const char*)lineParts[1].Items);
    float y = atof((const char*)lineParts[2].Items);

    return (SampleVector2)
    {
        .X = x,
        .Y = y
    };
}

SampleVector3 ReadObjVector3(ElemToolsDataSpan* lineParts, uint32_t linePartCount, bool reverseHandedness)
{
    if (linePartCount < 4)
    {
        printf("Error: Invalid number of elements in the line for Vector3.\n");
        return (SampleVector3) { 0 };
    }

    float x = atof((const char*)lineParts[1].Items);
    float y = atof((const char*)lineParts[2].Items);
    float z = atof((const char*)lineParts[3].Items);

    if (reverseHandedness)
    {
        z = -z;
    }

    return (SampleVector3)
    {
        .X = x,
        .Y = y,
        .Z = z
    };
}

ObjMeshFace ReadObjFace(ElemToolsDataSpan* lineParts, uint32_t linePartCount, bool reverseHandedness)
{
    if (linePartCount < 4)
    {
        printf("Error: Invalid number of elements in the line for Face.\n");
        return (ObjMeshFace) {};
    }

    ObjMeshFace result = {};

    for (uint32_t i = 0; i < 3; i++)
    {
        ElemToolsDataSpan faceParts[3];
        uint32_t facePartCount = 0;

        SampleSplitString(faceParts, &facePartCount, lineParts[i + 1], '/');

        for (uint32_t j = 0; j < 3; j++)
        {
            if (faceParts[j].Length > 0)
            {
                // Note: We are not handling negative indices
                result.Elements[i].Indices[j] = atoi((const char*)faceParts[j].Items);
            }
        }
    }

    if (reverseHandedness)
    {
        ObjMeshVertex temp = result.Elements[1].Vertex;
        result.Elements[1] = result.Elements[2];
        result.Elements[2].Vertex = temp;
    }

    return result;
}

InputMeshData ReadObjMesh(ElemToolsDataSpan data, bool reverseHandedness)
{
    ObjMeshElementCount meshElementCount = CountObjMeshElements(data);
    ElemToolsDataSpan line = SampleReadLine(&data);

    SampleVector3* vertexList = malloc(meshElementCount.VertexCount * sizeof(SampleVector3));
    memset(vertexList, 0, meshElementCount.VertexCount * sizeof(SampleVector3));
    uint32_t vertexCount = 1;

    SampleVector3* normalList = malloc(meshElementCount.NormalCount * sizeof(SampleVector3));
    memset(normalList, 0, meshElementCount.NormalCount * sizeof(SampleVector3));
    uint32_t normalCount = 1;

    SampleVector2* textCoordList = malloc(meshElementCount.TextCoordCount * sizeof(SampleVector2));
    memset(textCoordList, 0, meshElementCount.TextCoordCount * sizeof(SampleVector2));
    uint32_t textCoordCount = 1;

    InputMeshVertex* inputMeshVertexList = malloc(meshElementCount.FaceCount * 3 * sizeof(InputMeshVertex));
    uint32_t inputMeshVertexCount = 0;

    while (line.Length > 0)
    {
        ElemToolsDataSpan lineParts[64]; 
        uint32_t linePartCount = 0;
        SampleSplitString(lineParts, &linePartCount, line, ' ');

        if (linePartCount > 1)
        {
            if (SampleCompareString(lineParts[0], "g"))
            {
                printf("ERROR: Group not implemented yet!\n");
                return (InputMeshData) { 0 };
            }
            else if (SampleCompareString(lineParts[0], "v"))
            {
                vertexList[vertexCount++] = ReadObjVector3(lineParts, linePartCount, reverseHandedness);
            }
            else if (SampleCompareString(lineParts[0], "vn"))
            {
                normalList[normalCount++] = ReadObjVector3(lineParts, linePartCount, reverseHandedness);
            }
            else if (SampleCompareString(lineParts[0], "vt"))
            {
                textCoordList[textCoordCount++] = ReadObjVector2(lineParts, linePartCount);
            }
            else if (SampleCompareString(lineParts[0], "f"))
            {
                ObjMeshFace face = ReadObjFace(lineParts, linePartCount, reverseHandedness);
                
                for (uint32_t i = 0; i < 3; i++)
                {
                    InputMeshVertex vertex = 
                    {
                        .Position = vertexList[face.Elements[i].Vertex.VertexIndex],
                        .Normal = normalList[face.Elements[i].Vertex.NormalIndex],
                        .TextureCoordinates = textCoordList[face.Elements[i].Vertex.TextCoordIndex]
                    };
                
                    inputMeshVertexList[inputMeshVertexCount++] = vertex;
                }
            }
        }

        line = SampleReadLine(&data);
    }

    return (InputMeshData)
    {
        .VertexList = inputMeshVertexList,
        .VertexCount = inputMeshVertexCount
    };
}

int main(int argc, const char* argv[]) 
{
    // TODO: Add an option to handle handness change
    if (argc < 3)
    {
        printf("USAGE: MeshCompiler [options] inputfile outputfile\n");
        printf("\n");
        printf("OPTIONS:\n");
        printf("   --meshlet-triangle-count\tTBD: TBD. Default: TBD.\n");
        printf("\n");
        return 0;
    }

    int32_t inputPathIndex = argc - 2;
    const char* inputPath = argv[inputPathIndex];

    int32_t outputPathIndex = argc - 1;
    const char* outputPath = argv[outputPathIndex];

    // TODO: Add more checks
    for (uint32_t i = 1; i < (uint32_t)(argc - 2); i++)
    {
        printf("Options: %s\n", argv[i]);

        /*if (strcmp(argv[i], "--target-platform") == 0)
        {
            const char* targetPlatformString = argv[i + 1];

            if (strcmp(targetPlatformString, "iOS") == 0)
            {
                targetPlatform = ElemToolsPlatform_iOS;
                printf("iOS platform\n");
            }
        }
        else if (strcmp(argv[i], "--target-api") == 0)
        {
            const char* targetPlatformString = argv[i + 1];

            if (strcmp(targetPlatformString, "vulkan") == 0)
            {
                targetApi = ElemToolsGraphicsApi_Vulkan;
                printf("Vulkan api\n");
            }
        }
        else if (strcmp(argv[i], "--debug") == 0)
        {
            debugMode = true;
        }*/
    }

    printf("Compiling mesh: %s\n", inputPath);

    ElemToolsDataSpan inputData = SampleReadFile(inputPath); 
    
    if (inputData.Length == 0)
    {
        printf("File doesn't exist.\n");
        return 1;
    }

    ElemLoadMeshResult inputMesh = ElemLoadMesh(inputData, ElemMeshFormat_Obj, NULL);
    printf("Input mesh vertex Count: %d\n", inputMesh.VertexCount);

    InputMeshData inputMeshData = ReadObjMesh(inputData, true); 
    printf("Input mesh vertex Count: %d\n", inputMeshData.VertexCount);

    // TODO: Temporary
    if (inputMeshData.VertexCount == 0)
    {
        uint8_t test = { 1 };
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = &test, .Length = 1 }, false);
        return 0;
    }

    ElemVertexBuffer vertexBuffer =
    {
        .Data = { .Items = (uint8_t*)inputMeshData.VertexList, .Length = inputMeshData.VertexCount * sizeof(InputMeshVertex) },
        .VertexSize = sizeof(InputMeshVertex)
    };

    ElemBuildMeshletResult result = ElemBuildMeshlets(vertexBuffer, NULL);
    
    // TODO: Refactor this into an util function to display messages with proper colors
    for (uint32_t i = 0; i < result.Messages.Length; i++)
    {
        printf("Compil msg (%d): %s\n", result.Messages.Items[i].Type, result.Messages.Items[i].Message);
    }

    if (result.HasErrors)
    {
        printf("Error while compiling shader!\n");
        return 1;
    }

    // TODO: Good Unit test too!
    for (uint32_t i = 0; i < result.Meshlets.Length; i++)
    {
        ElemMeshlet meshlet = result.Meshlets.Items[i];

        if (i < result.Meshlets.Length - 1) 
        {
            ElemMeshlet nextMeshlet = result.Meshlets.Items[i + 1];

            if (meshlet.TriangleOffset + meshlet.TriangleCount - 1 == nextMeshlet.TriangleOffset)
            {
                printf("Error not last\n");
            }
        }
        else if (meshlet.TriangleOffset + meshlet.TriangleCount - 1 == result.MeshletTriangleIndexBuffer.Length)
        {
            printf("ERROR\n");
        }
    }

    for (uint32_t i = result.MeshletTriangleIndexBuffer.Length - 10; i < result.MeshletTriangleIndexBuffer.Length + 3; i++)
    {
        printf("Trig index: %u\n", result.MeshletTriangleIndexBuffer.Items[i]);
    }
    
    for (uint32_t i = 0; i < result.MeshletTriangleIndexBuffer.Length; i++)
    {
        if (result.MeshletTriangleIndexBuffer.Items[i] == 0)
        {
            printf("Zero Triangle index at: %d\n", i);
        }
    }

    printf("Writing mesh data to: %s\n", outputPath);

    uint32_t meshletBufferSize = result.Meshlets.Length * sizeof(ElemMeshlet);
    uint32_t meshletVertexIndexBufferSize = result.MeshletVertexIndexBuffer.Length * sizeof(uint32_t);

    SampleMeshHeader meshHeader = 
    {
        .FileId = { 'M', 'E', 'S', 'H' },
        .MeshletCount = result.Meshlets.Length,
        .MeshletMaxVertexCount = result.MeshletMaxVertexCount,
        .MeshletMaxTriangleCount = result.MeshletMaxTriangleCount,
        .VertexBufferOffset = sizeof(SampleMeshHeader),
        .VertexBufferSizeInBytes = result.VertexBuffer.Data.Length,
        .MeshletBufferOffset = sizeof(SampleMeshHeader) + result.VertexBuffer.Data.Length,
        .MeshletBufferSizeInBytes = meshletBufferSize,
        .MeshletVertexIndexBufferOffset = sizeof(SampleMeshHeader) + result.VertexBuffer.Data.Length + meshletBufferSize,
        .MeshletVertexIndexBufferSizeInBytes = meshletVertexIndexBufferSize,
        .MeshletTriangleIndexBufferOffset = sizeof(SampleMeshHeader) + result.VertexBuffer.Data.Length + meshletBufferSize + meshletVertexIndexBufferSize,
        .MeshletTriangleIndexBufferSizeInBytes = result.MeshletTriangleIndexBuffer.Length * sizeof(uint32_t)
    };
    
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = (uint8_t*)&meshHeader, .Length = sizeof(SampleMeshHeader) }, false);
    SampleWriteDataToFile(outputPath, result.VertexBuffer.Data, true);
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = (uint8_t*)result.Meshlets.Items, .Length = meshletBufferSize}, true);
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = (uint8_t*)result.MeshletVertexIndexBuffer.Items, .Length = result.MeshletVertexIndexBuffer.Length * sizeof(uint32_t) }, true);
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = (uint8_t*)result.MeshletTriangleIndexBuffer.Items, .Length = result.MeshletTriangleIndexBuffer.Length * sizeof(uint32_t) }, true);
}
