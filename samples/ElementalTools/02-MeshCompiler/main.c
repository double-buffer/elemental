#include "ElementalTools.h"
#include "SampleUtils.h"
#include "SampleMath.h"

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

// TODO: Fix warning
typedef struct
{
    union
    {
        struct 
        {
            uint32_t VertexIndex;
            uint32_t NormalIndex;
            uint32_t TextCoordIndex;
        };

        uint32_t Indices[3];
    } Elements[3];
} ObjMeshFace;

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

SampleVector3 ReadObjVector3(ElemToolsDataSpan* lineParts, uint32_t linePartCount)
{
    if (linePartCount < 4)
    {
        printf("Error: Invalid number of elements in the line for Vector3.\n");
        return (SampleVector3) { 0 };
    }

    float x = atof((const char*)lineParts[1].Items);
    float y = atof((const char*)lineParts[2].Items);
    float z = atof((const char*)lineParts[3].Items);

    return (SampleVector3)
    {
        .X = x,
        .Y = y,
        .Z = z
    };
}

ObjMeshFace ReadObjFace(ElemToolsDataSpan* lineParts, uint32_t linePartCount)
{
    if (linePartCount < 4)
    {
        printf("Error: Invalid number of elements in the line for Face.\n");
        return (ObjMeshFace) { 0 };
    }

    ObjMeshFace result = { 0 };

    for (uint32_t i = 0; i < 3; i++)
    {
        ElemToolsDataSpan faceParts[3];
        uint32_t facePartCount = 0;

        SampleSplitString(faceParts, &facePartCount, lineParts[i + 1], '/');

        for (uint32_t j = 0; j < 3; j++)
        {
            if (faceParts[j].Length > 0)
            {
                result.Elements[i].Indices[j] = atoi((const char*)faceParts[j].Items);
            }
        }
    }

    return result;
}

void ReadObjMesh(ElemToolsDataSpan data)
{
    ObjMeshElementCount meshElementCount = CountObjMeshElements(data);
    ElemToolsDataSpan line = SampleReadLine(&data);

    SampleVector3* vertexList = malloc(meshElementCount.VertexCount * sizeof(SampleVector3));
    uint32_t vertexCount = 1;

    SampleVector3* normalList = malloc(meshElementCount.NormalCount * sizeof(SampleVector3));
    uint32_t normalCount = 1;

    SampleVector2* textCoordList = malloc(meshElementCount.TextCoordCount * sizeof(SampleVector2));
    uint32_t textCoordCount = 1;

    ObjMeshFace* faceList = malloc(meshElementCount.FaceCount * sizeof(ObjMeshFace));
    uint32_t faceCount = 0;

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
            }
            else if (SampleCompareString(lineParts[0], "v"))
            {
                vertexList[vertexCount++] = ReadObjVector3(lineParts, linePartCount);
            }
            else if (SampleCompareString(lineParts[0], "vn"))
            {
                normalList[normalCount++] = ReadObjVector3(lineParts, linePartCount);
            }
            else if (SampleCompareString(lineParts[0], "vt"))
            {
                textCoordList[textCoordCount++] = ReadObjVector2(lineParts, linePartCount);
            }
            else if (SampleCompareString(lineParts[0], "f"))
            {
                faceList[faceCount++] = ReadObjFace(lineParts, linePartCount);
            }
        }

        line = SampleReadLine(&data);
    }

    for (uint32_t i = 0; i < 10; i++)
    {
        printf("Vertex %d: %f, %f, %f\n", i, vertexList[i].X, vertexList[i].Y, vertexList[i].Z);
    }

    for (uint32_t i = 0; i < 10; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            printf("Face %d-%d: %d, %d, %d\n", i, j, faceList[i].Elements[j].VertexIndex, faceList[i].Elements[j].NormalIndex, faceList[i].Elements[j].TextCoordIndex);
        }
    }
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

    ReadObjMesh(inputData); 
}
