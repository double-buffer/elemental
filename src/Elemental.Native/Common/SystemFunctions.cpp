#include "SystemFunctions.h"
#include "SystemPlatformFunctions.h"

//---------------------------------------------------------------------------------------------------------------
// Math functions
//---------------------------------------------------------------------------------------------------------------

size_t SystemRoundUpToPowerOf2(size_t value) 
{
    size_t result = 1;

    while (result < value) 
    {
        result <<= 1;
    }

    return result;
}

double SystemRound(double value)
{
    return (value < 0.0) ? (int32_t)(value - 0.5) : (int32_t)(value + 0.5);
}

//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

template<typename T>
ReadOnlySpan<char> SystemConvertNumberToString(MemoryArena* memoryArena, T value)
{
    auto isNegative = value < 0;
    auto length = isNegative ? 1 : 0;
    auto temp = value;

    do 
    {
        temp /= 10;
        length++;
    } while (temp != 0);

    auto numString = SystemPushArrayZero<char>(memoryArena, length);
    auto startIndex = 0;

    if (isNegative)
    {
        value = -value;
        numString[0] = '-';
        startIndex = 1;
    }

    for (int32_t i = length - 1; i >= startIndex; i--)
    {
        numString[i] = '0' + (value % 10);
        value /= 10;
    }

    return numString;
}

ReadOnlySpan<char> SystemConvertFloatToString(MemoryArena* memoryArena, double value)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto integerPart = SystemConvertNumberToString(stackMemoryArena, (int32_t)value);
    auto fractionalPart = SystemConvertNumberToString(stackMemoryArena, -SystemAbs((int32_t)SystemRound((value - (int32_t)value) * 100.0)));
    ((char*)fractionalPart.Pointer)[0] = '.';

    return SystemConcatBuffers<char>(memoryArena, integerPart, fractionalPart);
}

ReadOnlySpan<char> SystemFormatString(MemoryArena* memoryArena, ReadOnlySpan<char> format, ...)
{
    __builtin_va_list arguments;

    __builtin_va_start(arguments, format);
    auto result = SystemFormatString(memoryArena, format, arguments);    
    __builtin_va_end(arguments);

    return result;
}

ReadOnlySpan<char> SystemFormatString(MemoryArena* memoryArena, ReadOnlySpan<char> format, __builtin_va_list arguments)
{
    bool insideFormat = true;
    size_t elementCount = 0;

    for (size_t i = 0; i < format.Length; i++)
    {
        if (insideFormat)
        {
            elementCount++;
            insideFormat = false;
        }

        if (format[i] == '%')
        {
            if (i == format.Length - 1)
            {
                break;
            }

            switch (format[++i])
            {
            case 'd':
            case 's':
            case 'f':
            case 'u':
                elementCount++;
                insideFormat = true;
                break;
            }
        }
    }

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto resultParts = SystemPushArray<ReadOnlySpan<char>>(stackMemoryArena, elementCount);
    size_t startPartIndex = 0;
    size_t currentPartIndex = 0;
    size_t finalCount = 0;

    for (size_t i = 0; i < format.Length; i++)
    {
        if (format[i] == '%')
        {
            if (i == format.Length - 1)
            {
                break;
            }

            auto formatedArgument = ReadOnlySpan<char>("");

            switch (format[++i])
            {
                case 'd':
                    formatedArgument = SystemConvertNumberToString(stackMemoryArena, __builtin_va_arg(arguments, long));
                    break;
                case 'f':
                    formatedArgument = SystemConvertFloatToString(stackMemoryArena, __builtin_va_arg(arguments, double));
                    break;
                case 'u':
                    formatedArgument = SystemConvertNumberToString(stackMemoryArena, __builtin_va_arg(arguments, unsigned long long));
                    break;
                case 's':
                    formatedArgument = (ReadOnlySpan<char>)__builtin_va_arg(arguments, const char*);
                    break;
            }

            auto partLength = (i - 1) - startPartIndex;
            finalCount += partLength;
            resultParts[currentPartIndex++] = format.Slice(startPartIndex, partLength);

            startPartIndex = i + 1;

            resultParts[currentPartIndex++] = formatedArgument;
            finalCount += formatedArgument.Length;
        }
    }

    auto partLength = format.Length - startPartIndex;

    if (partLength > 0)
    {
        finalCount += partLength;
        resultParts[currentPartIndex++] = format.Slice(startPartIndex, partLength);
    }

    auto result = SystemPushArrayZero<char>(memoryArena, finalCount);
    auto resultSpan = result;
    
    for (size_t i = 0; i < resultParts.Length; i++)
    {
        SystemCopyBuffer(resultSpan, resultParts[i]);
        resultSpan = resultSpan.Slice(resultParts[i].Length);
    }

    return result;
}

ReadOnlySpan<ReadOnlySpan<char>> SystemSplitString(MemoryArena* memoryArena, ReadOnlySpan<char> source, char separator) 
{
    auto count = 1;

    for (size_t i = 0; i < source.Length; i++)
    {
        if (source[i] == separator)
        {
            count++;
        }
    }

    auto result = SystemPushArray<ReadOnlySpan<char>>(memoryArena, count);
    auto currentIndex = 0;

    for (int i = 0; i < count; i++)
    {
        auto separatorIndex = source.Length;

        for (size_t j = currentIndex; j < source.Length; j++)
        {
            if (source[j] == separator)
            {
                separatorIndex = j;
                break;
            }
        }

        auto resultString = SystemPushArrayZero<char>(memoryArena, separatorIndex - currentIndex);
        SystemCopyBuffer(resultString, source.Slice(currentIndex, separatorIndex - currentIndex));

        result[i] = resultString;

        currentIndex = separatorIndex + 1;
    }

    return result;
}

int64_t SystemLastIndexOf(ReadOnlySpan<char> source, char separator)
{
    for (int64_t i = source.Length - 1; i >= 0; i--)
    {
        if (source[i] == separator)
        {
            return i;
        }
    }

    return -1;
}

ReadOnlySpan<wchar_t> SystemConvertUtf8ToWideChar(MemoryArena* memoryArena, ReadOnlySpan<char> source)
{
    size_t size = 0;
    auto sourceSpan = source;

    while (sourceSpan[0] != '\0')
    {
        if ((sourceSpan[0] & 0b10000000) == 0b00000000) 
        {
            sourceSpan = sourceSpan.Slice(1);
        } 
        else if ((sourceSpan[0] & 0b11100000) == 0b11000000) 
        {
            sourceSpan = sourceSpan.Slice(2);
        } 
        else if ((sourceSpan[0] & 0b11110000) == 0b11100000) 
        {
            sourceSpan = sourceSpan.Slice(3);
        } 
        else if ((sourceSpan[0] & 0b11111000) == 0b11110000) 
        {
            sourceSpan = sourceSpan.Slice(4);
        }

        size++;
    }

    auto destination = SystemPushArrayZero<wchar_t>(memoryArena, size);
    auto destinationSpan = destination;

    sourceSpan = source;

    while (sourceSpan[0] != '\0')
    {
        if ((sourceSpan[0] & 0b10000000) == 0b00000000) 
        {
            destinationSpan[0] = sourceSpan[0];
            sourceSpan = sourceSpan.Slice(1);
        } 
        else if ((sourceSpan[0] & 0b11100000) == 0b11000000) 
        {
            destinationSpan[0] = ((sourceSpan[0] & 0b00011111) << 6) | (sourceSpan[1] & 0b00111111);
            sourceSpan = sourceSpan.Slice(2);
        } 
        else if ((sourceSpan[0] & 0b11110000) == 0b11100000) 
        {
            destinationSpan[0] = ((sourceSpan[0] & 0b00001111) << 12) | ((sourceSpan[1] & 0b00111111) << 6) | (sourceSpan[2] & 0b00111111);
            sourceSpan = sourceSpan.Slice(3);
        } 
        else if ((sourceSpan[0] & 0b11111000) == 0b11110000) 
        {
            destinationSpan[0] = ((sourceSpan[0] & 0b00000111) << 18) | ((sourceSpan[1] & 0b00111111) << 12) | ((sourceSpan[2] & 0b00111111) << 6) | (sourceSpan[3] & 0b00111111);
            sourceSpan = sourceSpan.Slice(4);
        }

        destinationSpan = destinationSpan.Slice(1);
    }

    return destination;
}

ReadOnlySpan<char> SystemConvertWideCharToUtf8(MemoryArena* memoryArena, ReadOnlySpan<wchar_t> source)
{
    size_t size = 0;
    auto sourceSpan = source;

    while (sourceSpan[0] != '\0')
    {
        if (sourceSpan[0] <= 0X7F) 
        {
            size++;
        } 
        else if (sourceSpan[0] <= 0x7FF)
        {
            size += 2;
        } 
        else if (sourceSpan[0] <= 0xFFFF)
        {
            size += 3;
        } 
        else
        {
            size += 4;
        }

        sourceSpan = sourceSpan.Slice(1);
    }

    auto destination = SystemPushArrayZero<char>(memoryArena, size);
    auto destinationSpan = destination;

    sourceSpan = source;

    while (sourceSpan[0] != '\0')
    {
        if (sourceSpan[0] <= 0X7F) 
        {
            destinationSpan[0] = sourceSpan[0];
            destinationSpan = destinationSpan.Slice(1);
        } 
        else if (sourceSpan[0] <= 0x7FF)
        {
            destinationSpan[0] = (char)(0xC0 | ((sourceSpan[0] >> 6) & 0x1F));
            destinationSpan[1] = (char)(0x80 | (sourceSpan[0] & 0x3F));
            
            destinationSpan = destinationSpan.Slice(2);
        } 
        else if (sourceSpan[0] <= 0xFFFF)
        {
            destinationSpan[0] = (char)(0xE0 | ((sourceSpan[0] >> 12) & 0x0F));
            destinationSpan[1] = (char)(0x80 | ((sourceSpan[0] >> 6) & 0x3F));
            destinationSpan[2] = (char)(0x80 | (sourceSpan[0] & 0x3F));
            
            destinationSpan = destinationSpan.Slice(3);
        } 
        else
        {
            destinationSpan[0] = (char)(0xF0 | ((sourceSpan[0] >> 18) & 0x07));
            destinationSpan[1] = (char)(0x80 | ((sourceSpan[0] >> 12) & 0x3F));
            destinationSpan[2] = (char)(0x80 | ((sourceSpan[0] >> 6) & 0x3F));
            destinationSpan[3] = (char)(0x80 | (sourceSpan[0] & 0x3F));;
            
            destinationSpan = destinationSpan.Slice(4);
        }

        sourceSpan = sourceSpan.Slice(1);
    }

    return destination;
}


//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

ReadOnlySpan<char> SystemGenerateTempFilename(MemoryArena* memoryArena, ReadOnlySpan<char> prefix) 
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto systemDateTime = SystemPlatformGetCurrentDateTime(stackMemoryArena);
    auto postfix = SystemFormatString(stackMemoryArena, "%d%d%d%d%d%d", systemDateTime->Year, systemDateTime->Month, systemDateTime->Day, systemDateTime->Hour, systemDateTime->Minute, systemDateTime->Second);

    return SystemConcatBuffers<char>(memoryArena, prefix, postfix);
}

ReadOnlySpan<char> SystemGetExecutableFolderPath(MemoryArena* memoryArena) 
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto pathSeparator = SystemPlatformGetEnvironment(stackMemoryArena)->PathSeparator;
    auto executablePath = SystemPlatformGetExecutablePath(stackMemoryArena);

    auto lastPathSeparator = SystemLastIndexOf(executablePath, pathSeparator);
   
    if (lastPathSeparator == -1)
    {
        return SystemPushArrayZero<char>(memoryArena, 0);
    }

    auto result = SystemPushArrayZero<char>(memoryArena, lastPathSeparator + 1);
    SystemCopyBuffer<char>(result, executablePath.Slice(0, lastPathSeparator + 1));

    return result;
}

bool SystemFileExists(ReadOnlySpan<char> path)
{
    return SystemPlatformFileExists(path);
}

void SystemFileWriteBytes(ReadOnlySpan<char> path, ReadOnlySpan<uint8_t> data)
{
    SystemPlatformFileWriteBytes(path, data);
}

Span<uint8_t> SystemFileReadBytes(MemoryArena* memoryArena, ReadOnlySpan<char> path)
{
    auto fileSizeInBytes = SystemPlatformFileGetSizeInBytes(path);
    auto fileData = SystemPushArray<uint8_t>(memoryArena, fileSizeInBytes);

    SystemPlatformFileReadBytes(path, fileData);

    return fileData;
}

void SystemFileDelete(ReadOnlySpan<char> path)
{
    SystemPlatformFileDelete(path);
}

//---------------------------------------------------------------------------------------------------------------
// Library / process functions
//---------------------------------------------------------------------------------------------------------------

ReadOnlySpan<char> SystemExecuteProcess(MemoryArena* memoryArena, ReadOnlySpan<char> command)
{
    return SystemPlatformExecuteProcess(memoryArena, command);
}

SystemLibrary SystemLoadLibrary(ReadOnlySpan<char> libraryName)
{
    return { SystemPlatformLoadLibrary(libraryName) };
}

void SystemFreeLibrary(SystemLibrary library)
{
    SystemPlatformFreeLibrary(library.Handle);
}

void* SystemGetFunctionExport(SystemLibrary library, ReadOnlySpan<char> functionName)
{
    return SystemPlatformGetFunctionExport(library.Handle, functionName);
}

//---------------------------------------------------------------------------------------------------------------
// Threading functions
//---------------------------------------------------------------------------------------------------------------

SystemThread SystemCreateThread(SystemThreadFunction threadFunction, void* parameters)
{
    return { SystemPlatformCreateThread((void*)threadFunction, parameters) };
}

void SystemFreeThread(SystemThread thread)
{
    SystemPlatformFreeThread(thread.Handle);
}
