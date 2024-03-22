namespace Elemental.Tools.CodeGenerator;

public class CLoaderCodeGenerator : ICodeGenerator
{
    const string template = """
        #define ##LIBRARY_NAME##Loader

        #include <assert.h>
        #include <stdio.h>
        #include "##SOURCE_HEADER##"

        #if defined(_WIN32)
           #define UNICODE
           #include <windows.h>
        #else
           #include <dlfcn.h>
           #include <unistd.h>
           #include <string.h>
        #endif

        #if defined(_WIN32)
            static HMODULE library##LIBRARY_NAME## = NULL;
        #else
            static void* library##LIBRARY_NAME## = NULL;
        #endif

        static int functionPointersLoaded##LIBRARY_NAME## = 0;

        typedef struct ##LIBRARY_NAME##Functions 
        {
            ##ELEM_FUNCTIONS_DECLARATIONS##
        } ##LIBRARY_NAME##Functions;

        static ##LIBRARY_NAME##Functions list##LIBRARY_NAME##Functions;

        static bool Load##LIBRARY_NAME##Library(void) 
        {
            if (!library##LIBRARY_NAME##) 
            {
                #if defined(_WIN32)
                    library##LIBRARY_NAME## = LoadLibrary(L"##DLL_NAME##.dll");
                #elif __APPLE__
                    library##LIBRARY_NAME## = dlopen("lib##DLL_NAME##.dylib", RTLD_LAZY);
                #else
                    library##LIBRARY_NAME## = dlopen("lib##DLL_NAME##.so", RTLD_LAZY);
                #endif

                if (!library##LIBRARY_NAME##) 
                {
                    return false;
                }
            }

            return true;
        }

        void* Get##LIBRARY_NAME##FunctionPointer(const char* functionName) 
        {
            if (!library##LIBRARY_NAME##) 
            {
                return NULL;
            }

            #if defined(_WIN32)
                return (void*)GetProcAddress(library##LIBRARY_NAME##, functionName);
            #else
                return dlsym(library##LIBRARY_NAME##, functionName);
            #endif
        }

        static bool Load##LIBRARY_NAME##FunctionPointers(void) 
        {
            if (!Load##LIBRARY_NAME##Library() || functionPointersLoaded##LIBRARY_NAME##)
            {
                return functionPointersLoaded##LIBRARY_NAME##;
            }

            ##ELEM_FUNCTIONS##

            functionPointersLoaded##LIBRARY_NAME## = 1;
            return true;
        }
        """;

    const string defaultLogsTemplate = """
        static inline void ElemConsoleLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
        {
            printf("[");
            printf("\033[36m");

            if (category == ElemLogMessageCategory_Assert)
            {
                printf("Assert");
            }
            else if (category == ElemLogMessageCategory_Memory)
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
                printf("\033[31m Error:");
            }
            else if (messageType == ElemLogMessageType_Warning)
            {
                printf("\033[33m Warning:");
            }
            else if (messageType == ElemLogMessageType_Debug)
            {
                printf("\033[0m Debug:");
            }
            else
            {
                printf("\033[0m");
            }

            printf(" %s\033[0m\n", message);
            fflush(stdout);
        }

        static inline void ElemConsoleErrorLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
        {
            if (messageType != ElemLogMessageType_Error)
            {
                return;
            }

            printf("[");
            printf("\033[36m");

            if (category == ElemLogMessageCategory_Assert)
            {
                printf("Assert");
            }
            else if (category == ElemLogMessageCategory_Memory)
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
            printf("\033[32m %s\033[31m Error: %s\033[0m\n", function, message);
            fflush(stdout);
        }

        """;

    const string functionTemplate = """
        if (!Load##LIBRARY_NAME##FunctionPointers()) 
        {
            assert(library##LIBRARY_NAME##);

            #ifdef __cplusplus
            ##RETURN_TYPE## result = {};
            #else
            ##RETURN_TYPE## result = (##RETURN_TYPE##){0};
            #endif

            return result;
        }

        if (!list##LIBRARY_NAME##Functions.##FUNCTION_NAME##) 
        {
            assert(list##LIBRARY_NAME##Functions.##FUNCTION_NAME##);

            #ifdef __cplusplus
            ##RETURN_TYPE## result = {};
            #else
            ##RETURN_TYPE## result = (##RETURN_TYPE##){0};
            #endif

            return result;
        }

        return list##LIBRARY_NAME##Functions.##FUNCTION_NAME##(##FUNCTION_PARAMETER_VALUES##);
    """;

    const string functionTemplateVoid = """
        if (!Load##LIBRARY_NAME##FunctionPointers()) 
        {
            assert(library##LIBRARY_NAME##);
            return;
        }

        if (!list##LIBRARY_NAME##Functions.##FUNCTION_NAME##) 
        {
            assert(list##LIBRARY_NAME##Functions.##FUNCTION_NAME##);
            return;
        }

        list##LIBRARY_NAME##Functions.##FUNCTION_NAME##(##FUNCTION_PARAMETER_VALUES##);
    """;


    public void GenerateCode(CppCompilation compilation, string source, string input, string output)
    {
        var builder = new StringBuilder();
        builder.AppendLine(template);

        if (Path.GetFileName(input) == "Elemental.h")
        {
            builder.AppendLine(defaultLogsTemplate);
        }

        var functionsDeclarationStringBuilder = new StringBuilder();
        var functionsStringBuilder = new StringBuilder();

        foreach (var function in compilation.Functions)
        {
            if (!(Path.GetFileName(function.SourceFile) == "Elemental.h" || Path.GetFileName(function.SourceFile) == "ElementalTools.h"))
            {
                continue;
            }

            builder.AppendLine();
            builder.Append($"static inline {function.ReturnType.GetDisplayName()} {function.Name}(");
            var isFirstParameter = true;
            var parameterValuesBuilder = new StringBuilder();

            functionsStringBuilder.Append($"list##LIBRARY_NAME##Functions.{function.Name} = ({function.ReturnType.GetDisplayName()} (*)(");
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

            functionsStringBuilder.AppendLine($"))Get##LIBRARY_NAME##FunctionPointer(\"{function.Name}\");");
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

        var libraryName = Path.GetFileNameWithoutExtension(input);

        builder.Replace("##SOURCE_HEADER##", Path.GetFileName(input));
        builder.Replace("##LIBRARY_NAME##", libraryName);
        builder.Replace("##DLL_NAME##", libraryName == "Elemental" ? "Elemental" : "Elemental.Tools");

        File.WriteAllText(output, builder.ToString());
    }
}
