#pragma once

#if _WINDOWS
#define DllExport extern "C" __declspec(dllexport)
#include <windows.h>
#else
#define DllExport extern "C" __attribute__((visibility("default"))) 
#include <iconv.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <string>


// TODO: Move that to SystemFunctions.h
#if _WINDOWS
uint8_t* ConvertWStringToUtf8(const std::wstring &source)
{
    char *destination = new char[source.length() + 1];
    destination[source.length()] = '\0';
    WideCharToMultiByte(65001, 0, source.c_str(), -1, destination, (int)source.length(), NULL, NULL);

    return (uint8_t*)destination;
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