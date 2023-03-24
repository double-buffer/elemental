#pragma once

#if _WINDOWS
#define DllExport extern "C" __declspec(dllexport)
#include <windows.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;
#else
#define DllExport extern "C" __attribute__((visibility("default"))) 
#include <iconv.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>

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