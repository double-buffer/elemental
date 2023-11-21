#include "PreCompiledHeader.h"
#include "MetalShaderCompilerProvider.h"

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
    auto tempMemoryArena = SystemMemoryArenaAllocate(1024);
    //auto output =  SystemExecuteProcess(tempMemoryArena, "xcrun -f metal");

    SystemMemoryArenaFree(tempMemoryArena);
    return true;
}
    
Span<uint8_t> MetalShaderCompilerProvider::CompileShader(MemoryArena* memoryArena, std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
#ifdef _WINDOWS
    logList.push_back({ShaderCompilerLogEntryType_Error, (uint8_t*)SystemConvertWideCharToUtf8(memoryArena, L"Metal shader compiler is not supported on Windows.").Pointer});
    return Span<uint8_t>::Empty();
#else
    auto inputFilePath = std::string(SystemGenerateTempFilename()) + ".metal";
    auto airFilePath = std::string(SystemGenerateTempFilename());
    auto outputFilePath = std::string(SystemGenerateTempFilename());

    SystemWriteBytesToFile(inputFilePath.c_str(), shaderCode.Pointer, shaderCode.Length);

    auto commandLine = "xcrun metal -c " + inputFilePath + " -o " + airFilePath;

    // TODO: Change that
    char* output = new char[1024];
    SystemExecuteProcess(commandLine.c_str(), output);

    auto hasErrors = ProcessLogOutput(logList, output);
    
    if (hasErrors)
    {
        delete[] output;
        SystemDeleteFile(inputFilePath.c_str());
        return Span<uint8_t>::Empty();
    }
    
    commandLine = "xcrun metallib " + airFilePath + " -o " + outputFilePath;
    // TODO: Change that
    delete[] output;
    output = new char[1024];
    SystemExecuteProcess(commandLine.c_str(), output);
    
    hasErrors = ProcessLogOutput(logList, output);
    
    if (hasErrors)
    {
        delete[] output;
        SystemDeleteFile(inputFilePath.c_str());
        SystemDeleteFile(airFilePath.c_str());
        
        return Span<uint8_t>::Empty();
    }
    
    uint8_t* outputShaderData;
    size_t outputShaderDataSize;

    SystemReadBytesFromFile(outputFilePath.c_str(), &outputShaderData, &outputShaderDataSize);

    delete[] output;

    SystemDeleteFile(inputFilePath.c_str());
    SystemDeleteFile(airFilePath.c_str());
    SystemDeleteFile(outputFilePath.c_str());
        
    return Span<uint8_t>(outputShaderData, outputShaderDataSize);
    #endif
}
    
bool MetalShaderCompilerProvider::ProcessLogOutput(MemoryArena* memoryArena, std::vector<ShaderCompilerLogEntry>& logList, char* output)
{
    // BUG: 
    return false;
    auto outputWString = SystemConvertUtf8ToWideChar(memoryArena, output);

    if (outputWString.IsEmpty())
    {
        return false;
    }
    
    auto lines = SystemSplitString(memoryArena, outputWString, L'\n');
    
    auto hasErrors = false;
    auto currentLogType = ShaderCompilerLogEntryType_Error;
    std::wstring line;

    // TODO: Merge error messages and warnings message until next find
    for (size_t i = 0; i < lines.Length; i++)
    {
        line = lines[i].Pointer;

        if (line.find(L"warning:", 0) != -1)
        {
            currentLogType = ShaderCompilerLogEntryType_Warning;
        }
        else if (line.find(L"error:", 0) != -1)
        {
            currentLogType = ShaderCompilerLogEntryType_Error;
            hasErrors = true;
        }

        if (line.length() > 0)
        { 
            logList.push_back({ currentLogType, (uint8_t*)SystemConvertWideCharToUtf8(memoryArena, line.c_str()).Pointer });
        }
    }

    return hasErrors;
}
