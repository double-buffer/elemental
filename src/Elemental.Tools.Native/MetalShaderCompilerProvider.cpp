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
    auto output = SystemExecuteProcess("xcrun -f metal");
    return !output.empty();
}
    
Span<uint8_t> MetalShaderCompilerProvider::CompileShader(std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
#ifdef _WINDOWS
    logList.push_back({ShaderCompilerLogEntryType_Error, SystemConvertWStringToUtf8(L"Metal shader compiler is not supported on Windows.")});
    return Span<uint8_t>::Empty();
#else
    auto inputFilePath = SystemGenerateTempFilename() + ".metal";
    auto airFilePath = SystemGenerateTempFilename();
    auto outputFilePath = SystemGenerateTempFilename();

    SystemWriteBytesToFile(inputFilePath, shaderCode);

    auto commandLine = "xcrun metal -c " + inputFilePath + " -o " + airFilePath;
    auto output = SystemExecuteProcess(commandLine);

    auto hasErrors = ProcessLogOutput(logList, output);
    
    if (hasErrors)
    {
        SystemDeleteFile(inputFilePath);
        return Span<uint8_t>::Empty();
    }
    
    commandLine = "xcrun metallib " + airFilePath + " -o " + outputFilePath;
    output = SystemExecuteProcess(commandLine);
    
    hasErrors = ProcessLogOutput(logList, output);
    
    if (hasErrors)
    {
        SystemDeleteFile(inputFilePath);
        SystemDeleteFile(airFilePath);
        
        return Span<uint8_t>::Empty();
    }
    
    auto outputShaderData = SystemReadBytesFromFile(outputFilePath);
 
    SystemDeleteFile(inputFilePath);
    SystemDeleteFile(airFilePath);
    SystemDeleteFile(outputFilePath);
        
    return outputShaderData;
    #endif
}
    
bool MetalShaderCompilerProvider::ProcessLogOutput(std::vector<ShaderCompilerLogEntry>& logList, std::string output)
{
    auto outputWString = SystemConvertUtf8ToWString((uint8_t*)output.c_str());
    auto lines = SystemSplitString(outputWString, L"\n");
    
    auto hasErrors = false;
    auto currentLogType = ShaderCompilerLogEntryType_Error;
    std::wstring line;

    // TODO: Merge error messages and warnings message until next find
    for (int32_t i = 0; i < lines.size(); i++)
    {
        line = lines[i];

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
            logList.push_back({ currentLogType, SystemConvertWStringToUtf8(line) });
        }
    }

    return hasErrors;
}