#include "MetalShaderCompilerProvider.h"
#include "SystemFunctions.h"

ShaderLanguage MetalShaderCompilerProvider::GetShaderLanguage()
{
    return ShaderLanguage_Msl;
}

void MetalShaderCompilerProvider::GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount)
{
    int32_t index = 0;

    targetLanguages[index++] = ShaderLanguage_MetalIR;
    *targetLanguagesCount = index;
}

bool MetalShaderCompilerProvider::IsCompilerInstalled()
{
    // TODO: Doesn't work on windows
    // TODO: Use a scratch arena
    auto tempMemoryArena = SystemAllocateMemoryArena(1024);
    //auto output =  SystemExecuteProcess(tempMemoryArena, "xcrun -f metal");

    SystemFreeMemoryArena(tempMemoryArena);
    return true;
}
    
Span<uint8_t> MetalShaderCompilerProvider::CompileShader(MemoryArena memoryArena, std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

#ifdef _WINDOWS
    logList.push_back({ShaderCompilerLogEntryType_Error, (uint8_t*)"Metal shader compiler is not supported on Windows."});
    return Span<uint8_t>();
#else

    // TODO: Use Format 😄
    auto inputFilePath = SystemConcatBuffers<char>(stackMemoryArena, SystemGenerateTempFilename(stackMemoryArena, "ShaderInput"), ".metal");
    auto airFilePath = SystemGenerateTempFilename(stackMemoryArena, "ShaderAir");
    auto outputFilePath = SystemGenerateTempFilename(stackMemoryArena, "ShaderOutput");

    SystemFileWriteBytes(inputFilePath, shaderCode);

    auto commandLine = SystemFormatString(stackMemoryArena, "xcrun metal -c %s -o %s", inputFilePath.Pointer, airFilePath.Pointer);

    auto output = SystemExecuteProcess(stackMemoryArena, commandLine);

    auto hasErrors = ProcessLogOutput(memoryArena, logList, output);
    
    if (hasErrors)
    {
        SystemFileDelete(inputFilePath);
        return Span<uint8_t>();
    }
    
    commandLine = SystemFormatString(stackMemoryArena, "xcrun metallib %s -o %s", airFilePath.Pointer, outputFilePath.Pointer);
    output = SystemExecuteProcess(stackMemoryArena, commandLine);
    
    hasErrors = ProcessLogOutput(memoryArena, logList, output);
    
    if (hasErrors)
    {
        SystemFileDelete(inputFilePath);
        SystemFileDelete(airFilePath);
        
        return Span<uint8_t>();
    }
    
    auto outputShaderData = SystemFileReadBytes(memoryArena, outputFilePath);

    SystemFileDelete(inputFilePath);
    SystemFileDelete(airFilePath);
    SystemFileDelete(outputFilePath);
        
    return outputShaderData;
    #endif
}
    
bool MetalShaderCompilerProvider::ProcessLogOutput(MemoryArena memoryArena, std::vector<ShaderCompilerLogEntry>& logList, ReadOnlySpan<char> output)
{
    // BUG: 
    return false;

    auto lines = SystemSplitString(memoryArena, output, '\n');
    
    auto hasErrors = false;
    auto currentLogType = ShaderCompilerLogEntryType_Error;
    std::string line;

    // TODO: Merge error messages and warnings message until next find
    for (size_t i = 0; i < lines.Length; i++)
    {
        line = lines[i].Pointer;

        if (line.find("warning:", 0) != -1)
        {
            currentLogType = ShaderCompilerLogEntryType_Warning;
        }
        else if (line.find("error:", 0) != -1)
        {
            currentLogType = ShaderCompilerLogEntryType_Error;
            hasErrors = true;
        }

        if (line.length() > 0)
        { 
            logList.push_back({ currentLogType, (uint8_t*)line.c_str() });
        }
    }

    return hasErrors;
}
