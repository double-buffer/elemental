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
    auto output = ExecuteProcess("xcrun -f metal");
    return !output.empty();
}
    
Span<uint8_t> MetalShaderCompilerProvider::CompileShader(std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
#ifdef _WINDOWS
    logList.push_back({ShaderCompilerLogEntryType_Error, ConvertWStringToUtf8(L"Metal shader compiler is not supported on Windows.")});
    return Span<uint8_t>::Empty();
#else
    auto inputFilePath = GenerateTempFilename() + ".metal";
    auto airFilePath = GenerateTempFilename();
    auto outputFilePath = GenerateTempFilename();

    WriteBytesToFile(inputFilePath, shaderCode.Pointer, shaderCode.Length);

    // TODO: Refactor all of that!

    auto commandLine = "xcrun metal -c " + inputFilePath + " -o " + airFilePath;
    auto output = ExecuteProcess(commandLine.c_str());

    auto outputWString = ConvertUtf8ToWString((uint8_t*)output.c_str());
    auto lines = SplitString(outputWString, L"\n");
    
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
            ShaderCompilerLogEntry logEntry = {};
            logEntry.Type = currentLogType;
            logEntry.Message = ConvertWStringToUtf8(line);
            logList.push_back(logEntry);
        }
    }
    
    if (hasErrors)
    {
        // TODO: Delete files
        return Span<uint8_t>::Empty();
    }
    
    commandLine = "xcrun metallib " + airFilePath + " -o " + outputFilePath;
    output = ExecuteProcess(commandLine.c_str());

    outputWString = ConvertUtf8ToWString((uint8_t*)output.c_str());
    lines = SplitString(outputWString, L"\n");
    
    currentLogType = ShaderCompilerLogEntryType_Error;

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
            ShaderCompilerLogEntry logEntry = {};
            logEntry.Type = currentLogType;
            logEntry.Message = ConvertWStringToUtf8(line);
            logList.push_back(logEntry);
        }
    }
    
    if (hasErrors)
    {
        // TODO: Delete files
        return Span<uint8_t>::Empty();
    }
    
    // TODO: Read file
    uint8_t* outputShaderData;
    int32_t outputShaderDataSize;

    ReadBytesFromFile(outputFilePath, &outputShaderData, &outputShaderDataSize);
 
    // TODO: Delete files
    //File.Delete(inputFilePath);
    //File.Delete(airFilePath);
    //File.Delete(outputFilePath);
        
    return Span<uint8_t>(outputShaderData, outputShaderDataSize);
    #endif
}