#pragma once

#if _WINDOWS
#define DllExport extern "C" __declspec(dllexport)
#include <windows.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;
#else
#define DllExport extern "C" __attribute__((visibility("default"))) 
#define ComPtr CComPtr
#include <iconv.h>
#endif

// TODO: Cleanup includes
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <locale>
#include <codecvt>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

#define AssertIfFailed(result) assert(!FAILED(result))

#if _WINDOWS
uint8_t* ConvertWStringToUtf8(const std::wstring &source)
{
    char *destination = new char[source.length() + 1];
    destination[source.length()] = '\0';
    WideCharToMultiByte(65001, 0, source.c_str(), -1, destination, (int)source.length(), NULL, NULL);

    return (uint8_t*)destination;
}

std::wstring ConvertUtf8ToWString(uint8_t* source)
{
    auto stringLength = std::string((char*)source).length();
    std::wstring destination;
    destination.resize(stringLength + 1);
    MultiByteToWideChar(CP_UTF8, 0, (char*)source, -1, (wchar_t*)destination.c_str(), (int)(stringLength + 1));

	return destination;
}
std::string GenerateTempFilename() 
{
    char temp[MAX_PATH];
    DWORD dwRetVal = GetTempFileName(TEXT("."), TEXT("mytempfile"), 0, temp);
    if (dwRetVal != 0) {
        return std::string(temp);
    }
    return "";
}
#else
uint8_t* ConvertWStringToUtf8(const std::wstring &source)
{
    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");

    char* output_buffer = new char[source.length() * 4];
    char* output_ptr = output_buffer;
    size_t output_len = source.length() * 4;

    char* input_ptr = reinterpret_cast<char*>((char*)source.c_str());
    size_t input_len = source.length() * sizeof(wchar_t);

    int result = iconv(cd, &input_ptr, &input_len, &output_ptr, &output_len);

    return (uint8_t*)output_buffer;
}

// TODO: This seems to be the thing to do, convert the other functions
std::wstring ConvertUtf8ToWString(const uint8_t* source)
{
    iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
    
    auto inputString = std::string((char*)source);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    auto outputString = converter.from_bytes(inputString);
    
    return outputString;
}

std::string GenerateTempFilename() 
{
    char temp[] = "/tmp/tempfileXXXXXX";
    int fd = mkstemp(temp);
    if (fd != -1) {
        close(fd);
        return std::string(temp);
    }
    return "";
}
#endif

std::vector<std::wstring> splitString(std::wstring w, std::wstring tokenizerStr) 
{
    std::vector<std::wstring> result;
    long tokeninzerLength = tokenizerStr.length();
    long position = 0;
    long findIndex = w.find(tokenizerStr, position);

    while (findIndex != -1)
    {
        std::wstring str = w.substr(position, findIndex - position);
        result.push_back(str);
        position = findIndex + tokeninzerLength;
        findIndex = w.find(tokenizerStr, position);
    }

    result.push_back(w.substr(position, w.length() - position));

    return result;
}

void WriteBytesToFile(const std::string& filename, const uint8_t* data, int32_t dataCount) 
{
    FILE* outFile = fopen(filename.c_str(), "wb");
    fwrite(data, sizeof(uint8_t), dataCount, outFile);
    fclose(outFile);
}

void ReadBytesFromFile(const std::string& filename, uint8_t** outputData, int32_t* dataSize)
{
    FILE* inFile = fopen(filename.c_str(), "rb");

    if (!inFile) 
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::fseek(inFile, 0, SEEK_END);
    std::size_t fileSize = std::ftell(inFile);
    std::rewind(inFile);

    *outputData = new uint8_t[fileSize];
    std::size_t bytesRead = std::fread(*outputData, sizeof(uint8_t), fileSize, inFile);

    assert(bytesRead == fileSize);
    std::fclose(inFile);

    *dataSize = fileSize;
}

std::string ExecuteProcess(const std::string cmd) 
{
    char buffer[128];
    std::string result = "";

    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    assert(pipe);

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) 
    {
        result += buffer;
    }

    pclose(pipe);
    return result;
}
