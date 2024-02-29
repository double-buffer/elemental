namespace Elemental.Tools.CodeGenerator;

public class CLoaderCodeGenerator : ICodeGenerator
{
    const string template = """
        #define ElementalLoader

        #include <assert.h>
        #include <stdio.h>
        #include "Elemental.h"

        #if defined(_WIN32)
           #define UNICODE
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

        static bool LoadElementalLibrary(void) 
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

        static bool LoadFunctionPointers(void) 
        {
            if (!LoadElementalLibrary() || functionPointersLoaded)
            {
                return functionPointersLoaded;
            }

            ##ELEM_FUNCTIONS##

            functionPointersLoaded = 1;
            return true;
        }

        static inline void ElemConsoleLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
        {
            printf("[");
            printf("\033[36m");

            // TODO: Provide a mapping function
            if (category == ElemLogMessageCategory_Memory)
            {
                printf("Memory");
            }
            else if (category == ElemLogMessageCategory_NativeApplication)
            {
                printf("NativeApplication");
            }
            else if (category == ElemLogMessageCategory_Graphics)
            {
                printf("Graphics");
            }
            else if (category == ElemLogMessageCategory_Inputs)
            {
                printf("Inputs");
            }

            printf("\033[0m]");

            printf("\033[32m %s", function);

            if (messageType == ElemLogMessageType_Error)
            {
                printf("\033[31m");
            }
            else if (messageType == ElemLogMessageType_Warning)
            {
                printf("\033[33m");
            }
            else
            {
                printf("\033[0m");
            }

            printf(" %s\n\033[0m", message);
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


    public void GenerateCode(CppCompilation compilation, string source, string input, string output)
    {
        var builder = new StringBuilder();
        builder.AppendLine(template);

        var functionsDeclarationStringBuilder = new StringBuilder();
        var functionsStringBuilder = new StringBuilder();

        foreach (var function in compilation.Functions)
        {
            if (Path.GetFileName(function.SourceFile) != "Elemental.h" || function.Name == "ElemConsoleLogHandler")
            {
                continue;
            }

            builder.AppendLine();
            builder.Append($"static inline {function.ReturnType.GetDisplayName()} {function.Name}(");
            var isFirstParameter = true;
            var parameterValuesBuilder = new StringBuilder();

            functionsStringBuilder.Append($"elementalFunctions.{function.Name} = ({function.ReturnType.GetDisplayName()} (*)(");
            functionsDeclarationStringBuilder.Append($"{function.ReturnType.GetDisplayName()} (*{function.Name})(");

            if (function.Parameters.Count == 0)
            {
                builder.Append("void");
                functionsStringBuilder.Append("void");
                functionsDeclarationStringBuilder.Append("void");
            }
            else
            {
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

        File.WriteAllText(output, builder.ToString());
    }
}
