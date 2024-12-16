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

size_t SystemAlignToPowerOf2(size_t offset, size_t alignment) 
{
    return (offset + alignment - 1) & ~(alignment - 1);
}

double SystemRound(double value)
{
    return (value < 0.0) ? (int32_t)(value - 0.5) : (int32_t)(value + 0.5);
}

int32_t SystemRoundUp(double value)
{
    auto intPart = (int32_t)value;
    auto fractionalPart = value - intPart;
    
    if (fractionalPart > 0.0) 
    {
        return intPart + 1;
    }

    return intPart;
}

template<typename T>
T SystemAbs(T value)
{
    return (value < 0) ? -value : value;
}

template<typename T>
T SystemMax(T value1, T value2)
{
    return (value1 > value2) ? value1 : value2;
}

template<typename T>
T SystemMin(T value1, T value2)
{
    return (value1 < value2) ? value1 : value2;
}

//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

template<typename T>
ReadOnlySpan<char> SystemConvertNumberToString(MemoryArena memoryArena, T value)
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

ReadOnlySpan<char> SystemConvertFloatToString(MemoryArena memoryArena, double value)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto integerPart = SystemConvertNumberToString(stackMemoryArena, (int32_t)value);
    auto fractionalPart = SystemConvertNumberToString(stackMemoryArena, -SystemAbs((int32_t)SystemRound((value - (int32_t)value) * 100.0)));
    ((char*)fractionalPart.Pointer)[0] = '.';

    return SystemConcatBuffers<char>(memoryArena, integerPart, fractionalPart);
}

ReadOnlySpan<char> SystemFormatString(MemoryArena memoryArena, ReadOnlySpan<char> format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    auto result = SystemFormatString(memoryArena, format, arguments);    
    va_end(arguments);

    return result;
}

ReadOnlySpan<char> SystemFormatString(MemoryArena memoryArena, ReadOnlySpan<char> format, va_list arguments)
{
    bool insideFormat = true;
    size_t elementCount = 0;

    for (uint32_t i = 0; i < (uint32_t)format.Length; i++)
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
    uint32_t startPartIndex = 0;
    uint32_t currentPartIndex = 0;
    uint32_t finalCount = 0;

    for (uint32_t i = 0; i < (uint32_t)format.Length; i++)
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
                    formatedArgument = SystemConvertNumberToString(stackMemoryArena, __builtin_va_arg(arguments, int32_t));
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
    
    for (uint32_t i = 0; i < (uint32_t)resultParts.Length; i++)
    {
        SystemCopyBuffer(resultSpan, resultParts[i]);
        resultSpan = resultSpan.Slice(resultParts[i].Length);
    }

    return result;
}

ReadOnlySpan<ReadOnlySpan<char>> SystemSplitString(MemoryArena memoryArena, ReadOnlySpan<char> source, char separator) 
{
    auto count = 1;

    for (uint32_t i = 0; i < (uint32_t)source.Length; i++)
    {
        if (source[i] == separator)
        {
            count++;
        }
    }

    auto result = SystemPushArray<ReadOnlySpan<char>>(memoryArena, count);
    auto currentIndex = 0;

    for (uint32_t i = 0; i < (uint32_t)count; i++)
    {
        auto separatorIndex = (uint32_t)source.Length;

        for (uint32_t j = currentIndex; j < (uint32_t)source.Length; j++)
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
    for (int32_t i = (uint32_t)source.Length - 1; i >= 0; i--)
    {
        if (source[i] == separator)
        {
            return i;
        }
    }

    return -1;
}

int64_t SystemFindSubString(ReadOnlySpan<char> source, ReadOnlySpan<char> subString)
{
    int64_t result = -1;
    uint32_t foundCurrentIndex = 0;

    for (uint32_t i = 0; i < (uint32_t)source.Length; i++)
    {
        if (foundCurrentIndex >= subString.Length)
        {
            return result;
        }

        if (source[i] == subString[foundCurrentIndex])
        {
            foundCurrentIndex++;

            if (result == -1)
            {
                result = i;
            }
        }
        else 
        {
            result = -1;
            foundCurrentIndex = 0;
        }
    }

    if (foundCurrentIndex >= subString.Length)
    {
        return result;
    }

    return -1;
}

ReadOnlySpan<wchar_t> SystemConvertUtf8ToWideChar(MemoryArena memoryArena, ReadOnlySpan<char> source)
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

ReadOnlySpan<char> SystemConvertWideCharToUtf8(MemoryArena memoryArena, ReadOnlySpan<wchar_t> source)
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

ReadOnlySpan<char> SystemGenerateTempFilename(MemoryArena memoryArena, ReadOnlySpan<char> prefix) 
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto systemDateTime = SystemPlatformGetCurrentDateTime(stackMemoryArena);
    auto postfix = SystemFormatString(stackMemoryArena, "%d%d%d%d%d%d", systemDateTime->Year, systemDateTime->Month, systemDateTime->Day, systemDateTime->Hour, systemDateTime->Minute, systemDateTime->Second);

    return SystemConcatBuffers<char>(memoryArena, prefix, postfix);
}

ReadOnlySpan<char> SystemGetExecutableFolderPath(MemoryArena memoryArena) 
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

Span<uint8_t> SystemFileReadBytes(MemoryArena memoryArena, ReadOnlySpan<char> path)
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

ReadOnlySpan<char> SystemExecuteProcess(MemoryArena memoryArena, ReadOnlySpan<char> command)
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

void SystemWaitThread(SystemThread thread)
{
    SystemPlatformWaitThread(thread.Handle);
}

void SystemYieldThread()
{
    SystemPlatformYieldThread();
}

void SystemFreeThread(SystemThread thread)
{
    SystemPlatformFreeThread(thread.Handle);
}
