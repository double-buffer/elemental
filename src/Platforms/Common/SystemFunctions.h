#pragma once

void* SystemLoadLibrary(const std::string libraryName)
{
#ifdef _WINDOWS
    const std::string libraryExtension = ".dll";
#elif __APPLE__
    const std::string libraryExtension = ".dylib";
#else
    const std::string libraryExtension = ".so";
#endif

#ifdef _WINDOWS
    return LoadLibraryA((libraryName + libraryExtension).c_str());
#else
    return dlopen(("lib" + libraryName + libraryExtension).c_str(), RTLD_LAZY);
#endif
}

void SystemFreeLibrary(void* library)
{
#ifdef _WINDOWS
    FreeLibrary((HMODULE)library);
#else
    dlclose(library);
#endif
}

void* SystemGetFunctionExport(void* library, std::string functionName)
{
#ifdef _WINDOWS
    return GetProcAddress((HMODULE)library, functionName.c_str());
#else
    return dlsym(library, functionName.c_str());
#endif
}

std::wstring ConvertUtf8ToWString(unsigned char* source)
{
    auto stringLength = std::string((char*)source).length();
    std::wstring destination;
    destination.resize(stringLength + 1);
    MultiByteToWideChar(CP_UTF8, 0, (char*)source, -1, (wchar_t*)destination.c_str(), (int)(stringLength + 1));

	return destination;
}

unsigned char* ConvertWStringToUtf8(const std::wstring &source)
{
    char *destination = new char[source.length() + 1];
    destination[source.length()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, source.c_str(), -1, destination, (int)source.length(), NULL, NULL);

    return (unsigned char*)destination;
}

std::wstring SystemGetExecutableFolderPath() 
{
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);

    // Find the position of the last backslash character
    wchar_t* last_slash = wcsrchr(path, L'\\');

    if (last_slash != NULL) 
    {
        // Create a string with the folder path
        return std::wstring(path, last_slash - path);
    }
    else 
    {
        // No backslash character found, so return an empty string
        return L"";
    }
}