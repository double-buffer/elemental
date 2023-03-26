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
    auto output = ExecuteProcess("xcrun -f metal");
    return !output.empty();
}
    
ShaderCompilerResult MetalShaderCompilerProvider::CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ToolsShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
{
    auto inputFilePath = GenerateTempFilename() + ".metal";
    auto airFilePath = GenerateTempFilename();
    auto outputFilePath = GenerateTempFilename();

    WriteBytesToFile(inputFilePath, shaderCode, shaderCodeSize);

    // TODO: Refactor all of that!

    auto commandLine = "xcrun metal -c " + inputFilePath + " -o " + airFilePath;
    auto output = ExecuteProcess(commandLine.c_str());

    auto outputWString = ConvertUtf8ToWString((uint8_t*)output.c_str());
    auto lines = splitString(outputWString, L"\n");
    
    auto logList = std::vector<ShaderCompilerLogEntry>();
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
        
        ShaderCompilerLogEntry logEntry = {};
        logEntry.Type = currentLogType;
        logEntry.Message = ConvertWStringToUtf8(line);
        logList.push_back(logEntry);
    }
    
    if (hasErrors)
    {
        auto logEntriesData = new ShaderCompilerLogEntry[logList.size()];
        memcpy(logEntriesData, logList.data(), logList.size() * sizeof(ShaderCompilerLogEntry));
        
        // TODO: Delete files
        ShaderCompilerResult result = {};

        result.IsSuccess = !hasErrors;
        result.Stage = shaderStage;
        result.EntryPoint = entryPoint;
        result.LogEntries = logEntriesData;
        result.LogEntryCount = logList.size();

        return result;
    }
    
    commandLine = "xcrun metallib " + airFilePath + " -o " + outputFilePath;
    output = ExecuteProcess(commandLine.c_str());

    outputWString = ConvertUtf8ToWString((uint8_t*)output.c_str());
    lines = splitString(outputWString, L"\n");
    
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
        
        ShaderCompilerLogEntry logEntry = {};
        logEntry.Type = currentLogType;
        logEntry.Message = ConvertWStringToUtf8(line);
        logList.push_back(logEntry);
    }
    
    if (hasErrors)
    {
        auto logEntriesData = new ShaderCompilerLogEntry[logList.size()];
        memcpy(logEntriesData, logList.data(), logList.size() * sizeof(ShaderCompilerLogEntry));
        
        // TODO: Delete files
        ShaderCompilerResult result = {};

        result.IsSuccess = !hasErrors;
        result.Stage = shaderStage;
        result.EntryPoint = entryPoint;
        result.LogEntries = logEntriesData;
        result.LogEntryCount = logList.size();

        return result;
    }
    
    // TODO: Read file
    uint8_t* outputShaderData;
    int32_t outputShaderDataSize;

    ReadBytesFromFile(outputFilePath, &outputShaderData, &outputShaderDataSize);
    
    auto logEntriesData = new ShaderCompilerLogEntry[logList.size()];
    memcpy(logEntriesData, logList.data(), logList.size() * sizeof(ShaderCompilerLogEntry));

    // TODO: Delete files
    //File.Delete(inputFilePath);
    //File.Delete(airFilePath);
    //File.Delete(outputFilePath);
    
    ShaderCompilerResult result = {};

    result.IsSuccess = !hasErrors;
    result.Stage = shaderStage;
    result.EntryPoint = entryPoint;
    result.ShaderData = outputShaderData;
    result.ShaderDataCount = outputShaderDataSize;
    result.LogEntries = logEntriesData;
    result.LogEntryCount = logList.size();

    return result;
}