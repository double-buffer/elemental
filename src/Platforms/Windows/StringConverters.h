#pragma once
#include "WindowsCommon.h"

std::wstring ConvertUtf8ToWString(unsigned char* source)
{
    auto stringLength = std::string((char*)source).length();
    std::wstring destination;
    destination.resize(stringLength + 1);
    MultiByteToWideChar(CP_UTF8, 0, (char*)source, -1, (wchar_t*)destination.c_str(), (int)(stringLength + 1));

	return destination;
}

// TODO: Find a way to delete the produced string
unsigned char* ConvertWStringToUtf8(const std::wstring &source)
{
    char *destination = new char[source.length() + 1];
    destination[source.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, source.c_str(), -1, destination, (int)source.length(), NULL, NULL);

    return (unsigned char*)destination;
}