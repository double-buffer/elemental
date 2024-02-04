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
                            if (!library) {
                                return NULL;
                            }

                            #if defined(_WIN32)
                                return GetProcAddress(library, functionName);
                            #else
                                return dlsym(library, functionName);
                            #endif
                        }
                        """;

const string functionTemplate = """
    if (!LoadElementalLibrary()) 
    {
        assert(library);
        return (##RETURN_TYPE##){0};
    }

    typedef ##RETURN_TYPE## (*FunctionType)(##FUNCTION_PARAMETERS##);
    FunctionType functionPointer;

    functionPointer = (FunctionType)GetFunctionPointer("##FUNCTION_NAME##");

    if (!functionPointer) 
    {
        assert(functionPointer);
        return (##RETURN_TYPE##){0};
    }

    return functionPointer(##FUNCTION_PARAMETER_VALUES##);
""";

const string functionTemplateVoid = """
    if (!LoadElementalLibrary()) 
    {
        assert(library);
        return;
    }

    typedef ##RETURN_TYPE## (*FunctionType)(##FUNCTION_PARAMETERS##);
    FunctionType functionPointer;

    functionPointer = (FunctionType)GetFunctionPointer("##FUNCTION_NAME##");

    if (!functionPointer) 
    {
        assert(functionPointer);
        return;
    }

    functionPointer(##FUNCTION_PARAMETER_VALUES##);
""";

if (args.Length != 2)
{
    Console.WriteLine("Error: Wrong number of arguments!");    
    return 1;
}

// TODO: Build a table of function pointers to speed up calls

var inputFile = args[0];
var outputFile = args[1];

var builder = new StringBuilder();
builder.AppendLine(template);

Console.WriteLine($"Processing file '{args[0]}'...");

var compilation = CppParser.ParseFile(inputFile);

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
    var parametersBuilder = new StringBuilder();
    var parameterValuesBuilder = new StringBuilder();

    foreach (var parameter in function.Parameters)
    {
        if (isFirstParameter)
        {
            isFirstParameter = false;
        }
        else
        {
            builder.Append(", ");
            parametersBuilder.Append(", ");
            parameterValuesBuilder.Append(", ");
        }

        builder.Append($"{parameter.Type.GetDisplayName()} {parameter.Name}");
        parametersBuilder.Append($"{parameter.Type.GetDisplayName()}");
        parameterValuesBuilder.Append($"{parameter.Name}");
    }

    builder.AppendLine(")");
    builder.AppendLine("{");

    builder.AppendLine(function.ReturnType.GetDisplayName() == "void" ? functionTemplateVoid : functionTemplate)
           .Replace("##RETURN_TYPE##", function.ReturnType.GetDisplayName())
           .Replace("##FUNCTION_NAME##", function.Name)
           .Replace("##FUNCTION_PARAMETERS##", parametersBuilder.ToString())
           .Replace("##FUNCTION_PARAMETER_VALUES##", parameterValuesBuilder.ToString());

    builder.AppendLine("}");
}

File.WriteAllText(outputFile, builder.ToString());

return 0;
