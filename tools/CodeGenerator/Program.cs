using System.Text;
using CppAst;


const string template = """
                        #define ElementalLoader

                        #include <assert.h>
                        #include "Elemental.h"

                        #if defined(_WIN32)
                           #include <windows.h>
                        #else
                           #include <dlfcn.h>
                        #endif

                        static void* library = NULL;
                        static int functionPointersLoaded = 0;

                        typedef struct ElementalFunctions 
                        {
                            ##ELEM_FUNCTIONS_DECLARATIONS##
                        } ElementalFunctions;
    
                        static ElementalFunctions elementalFunctions;

                        static bool LoadElementalLibrary() 
                        {
                            if (!library) 
                            {
                                #if defined(_WIN32)
                                    library = LoadLibrary(L"Elemental.dll");
                                #elif __APPLE__
                                    library = dlopen("libElemental.dylib", RTLD_LAZY);
                                #else
                                    library = dlopen("libElemental.so", RTLD_LAZY);
                                #endif

                                if (!library) 
                                {
                                    return false;
                                }
                            }

                            return true;
                        }

                        void* GetFunctionPointer(const char* functionName) 
                        {
                            if (!library) 
                            {
                                return NULL;
                            }

                            #if defined(_WIN32)
                                return GetProcAddress(library, functionName);
                            #else
                                return dlsym(library, functionName);
                            #endif
                        }

                        static bool LoadFunctionPointers() 
                        {
                            if (!LoadElementalLibrary() || functionPointersLoaded)
                            {
                                return functionPointersLoaded;
                            }

                            ##ELEM_FUNCTIONS##

                            functionPointersLoaded = 1;
                            return true;
                        }
                        """;

const string functionTemplate = """
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return (##RETURN_TYPE##){0};
    }

    return elementalFunctions.##FUNCTION_NAME##(##FUNCTION_PARAMETER_VALUES##);
""";

const string functionTemplateVoid = """
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    elementalFunctions.##FUNCTION_NAME##(##FUNCTION_PARAMETER_VALUES##);
""";

if (args.Length != 2)
{
    Console.WriteLine("Error: Wrong number of arguments!");    
    return 1;
}

// TODO: Add a caching mechanism to only re generate when needed

var inputFile = args[0];
var outputFile = args[1];

var builder = new StringBuilder();
builder.AppendLine(template);

Console.WriteLine($"Processing file '{args[0]}'...");

var compilation = CppParser.ParseFile(inputFile);

var functionsDeclarationStringBuilder = new StringBuilder();
var functionsStringBuilder = new StringBuilder();

foreach (var function in compilation.Functions)
{
    if (Path.GetFileName(function.SourceFile) != "Elemental.h" || function.Name == "ElemConsoleLogHandler")
    {
        continue;
    }

    Console.WriteLine($"Test: {function.Comment} {function.Name}");
    Console.WriteLine($"Function: {function.Name}");

    builder.AppendLine();
    builder.Append($"static {function.ReturnType.GetDisplayName()} {function.Name}(");
    var isFirstParameter = true;
    var parameterValuesBuilder = new StringBuilder();

    functionsStringBuilder.Append($"elementalFunctions.{function.Name} = ({function.ReturnType.GetDisplayName()} (*)(");
    functionsDeclarationStringBuilder.Append($"{function.ReturnType.GetDisplayName()} (*{function.Name})(");

    foreach (var parameter in function.Parameters)
    {
        if (isFirstParameter)
        {
            isFirstParameter = false;
        }
        else
        {
            builder.Append(", ");
            functionsStringBuilder.Append(", ");
            functionsDeclarationStringBuilder.Append(", ");
            parameterValuesBuilder.Append(", ");
        }

        builder.Append($"{parameter.Type.GetDisplayName()} {parameter.Name}");
        functionsStringBuilder.Append($"{parameter.Type.GetDisplayName()}");
        functionsDeclarationStringBuilder.Append($"{parameter.Type.GetDisplayName()}");
        parameterValuesBuilder.Append($"{parameter.Name}");
    }

    functionsStringBuilder.AppendLine($"))GetFunctionPointer(\"{function.Name}\");");
    functionsStringBuilder.Append("    ");

    functionsDeclarationStringBuilder.AppendLine($");");
    functionsDeclarationStringBuilder.Append("    ");

    builder.AppendLine(")");
    builder.AppendLine("{");

    builder.AppendLine(function.ReturnType.GetDisplayName() == "void" ? functionTemplateVoid : functionTemplate)
           .Replace("##RETURN_TYPE##", function.ReturnType.GetDisplayName())
           .Replace("##FUNCTION_NAME##", function.Name)
           .Replace("##FUNCTION_PARAMETER_VALUES##", parameterValuesBuilder.ToString());

    builder.AppendLine("}");
}

builder.Replace("##ELEM_FUNCTIONS##", functionsStringBuilder.ToString());
builder.Replace("##ELEM_FUNCTIONS_DECLARATIONS##", functionsDeclarationStringBuilder.ToString());

File.WriteAllText(outputFile, builder.ToString());

return 0;
