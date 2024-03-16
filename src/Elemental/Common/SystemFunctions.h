#pragma once

#include "SystemMemory.h"
#include "SystemLogging.h"

#undef assert

#ifdef _DEBUG
    #define SystemAssert(expression) if (!(expression)) { SystemLogErrorMessage(ElemLogMessageCategory_Assert, #expression); int* ptr = 0; *ptr = 0; }
    #define SystemAssertReturnNullHandle(expression) if (!(expression)) { SystemLogErrorMessage(ElemLogMessageCategory_Assert, #expression); int* ptr = 0; *ptr = 0; }
#else
    #define SystemAssert(expression) if (!(expression)) { SystemLogErrorMessage(ElemLogMessageCategory_Assert, #expression); exit(1); }
    #define SystemAssertReturnNullHandle(expression) if (!(expression)) { SystemLogErrorMessage(ElemLogMessageCategory_Assert, #expression); return ELEM_HANDLE_NULL; }
#endif

//---------------------------------------------------------------------------------------------------------------
// Math functions
//---------------------------------------------------------------------------------------------------------------

/**
 * Rounds up the given value to the nearest power of 2.
 *
 * This function calculates the smallest power of 2 that is greater than or equal to the input value.
 *
 * @param value The input value.
 * @return The rounded-up value to the nearest power of 2. If the input is already a power of 2, 
 *         it returns the same value.
 */
size_t SystemRoundUpToPowerOf2(size_t value);

/**
 * Aligns the given offset to the specified power of two alignment.
 *
 * This function adjusts the input offset to the nearest equal or higher multiple of the specified alignment.
 * The alignment value must be a power of 2. If the alignment is not a power of 2, the behavior of the function 
 * is undefined or it may return the input offset as is, depending on implementation.
 *
 * @param offset The input offset value to align.
 * @param alignment The alignment value, which must be a power of 2.
 * @return The aligned offset. If the input alignment is not a power of 2, the behavior is undefined or it 
 *         may return the input offset, depending on implementation.
 */
size_t SystemAlignToPowerOf2(size_t offset, size_t alignment);

/**
 * Rounds the given double value to the nearest integer.
 *
 * This function rounds a double value to the nearest integer using the standard half-away-from-zero rule.
 *
 * @param value The input double value.
 * @return The rounded value as an integer. 
 */
double SystemRound(double value);

/**
 * Rounds a double value up to the nearest integer.
 * If the value is already an integer, it will remain unchanged.
 *
 * @param value The double value to be rounded up.
 * @return The rounded integer as int32_t.
 */
int32_t SystemRoundUp(double value);

/**
 * Calculates the absolute value of the given input.
 *
 * This template function returns the absolute (non-negative) value of any numeric type.
 *
 * @tparam T The type of the input value. Must support unary '-' operator.
 * @param value The input value.
 * @return The absolute value of the input. The return type is the same as the input type.
 */
template<typename T>
T SystemAbs(T value);

/**
 * Returns the maximum of two given values.
 *
 * This template function compares two values of the same type and returns the greater one.
 *
 * @tparam T The type of the input values. Must support the '<' operator.
 * @param value1 The first input value.
 * @param value2 The second input value.
 * @return The greater of the two input values. The return type is the same as the input types.
 */
template<typename T>
T SystemMax(T value1, T value2);

/**
 * Returns the minimum of two given values.
 *
 * This template function compares two values of the same type and returns the lesser one.
 *
 * @tparam T The type of the input values. Must support the '<' operator.
 * @param value1 The first input value.
 * @param value2 The second input value.
 * @return The lesser of the two input values. The return type is the same as the input types.
 */
template<typename T>
T SystemMin(T value1, T value2);

//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

/**
 * Converts a number to a string using the provided memory arena.
 *
 * @tparam T The type of the input value.
 * @param memoryArena The memory arena for string conversion.
 * @param value The input value.
 * @return A read-only span containing the converted string.
 */
template<typename T>
ReadOnlySpan<char> SystemConvertNumberToString(MemoryArena memoryArena, T value);

/**
 * Converts a double value to a string using the provided memory arena.
 *
 * @param memoryArena The memory arena for string conversion.
 * @param value The input value.
 * @return A read-only span containing the converted string.
 */
ReadOnlySpan<char> SystemConvertFloatToString(MemoryArena memoryArena, double value);

/**
 * Formats a string using the provided memory arena and format specifier.
 *
 * @param memoryArena The memory arena for string formatting.
 * @param format The format specifier.
 * @param ... Additional arguments for formatting.
 * @return A read-only span containing the formatted string.
 */
ReadOnlySpan<char> SystemFormatString(MemoryArena memoryArena, ReadOnlySpan<char> format, ...);

/**
 * Formats a string using the provided memory arena, format specifier, and variable arguments.
 *
 * @param memoryArena The memory arena for string formatting.
 * @param format The format specifier.
 * @param arguments The variable arguments for formatting.
 * @return A read-only span containing the formatted string.
 */
ReadOnlySpan<char> SystemFormatString(MemoryArena memoryArena, ReadOnlySpan<char> format, __builtin_va_list arguments);

/**
 * Splits a string into substrings based on the specified separator.
 *
 * @param memoryArena The memory arena for string splitting.
 * @param source The source string to split.
 * @param separator The character used as a separator.
 * @return A read-only span of read-only spans containing the split substrings.
 */
ReadOnlySpan<ReadOnlySpan<char>> SystemSplitString(MemoryArena memoryArena, ReadOnlySpan<char> source, char separator);

/**
 * Finds the last index of the specified character in the given string.
 *
 * @param source The source string to search.
 * @param separator The character to search for.
 * @return The last index of the specified character, or -1 if not found.
 */
int64_t SystemLastIndexOf(ReadOnlySpan<char> source, char separator);

/**
 * Converts a UTF-8 encoded string to a wide character (UTF-16) string.
 *
 * @param memoryArena The memory arena for string conversion.
 * @param source The UTF-8 encoded source string.
 * @return A read-only span containing the wide character string.
 */
ReadOnlySpan<wchar_t> SystemConvertUtf8ToWideChar(MemoryArena memoryArena, ReadOnlySpan<char> source);

/**
 * Converts a wide character (UTF-16) string to UTF-8 encoding.
 *
 * @param memoryArena The memory arena for string conversion.
 * @param source The wide character source string.
 * @return A read-only span containing the UTF-8 encoded string.
 */
ReadOnlySpan<char> SystemConvertWideCharToUtf8(MemoryArena memoryArena, ReadOnlySpan<wchar_t> source);


//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

/**
 * Generates a temporary filename with the specified prefix.
 *
 * @param memoryArena Pointer to the memory arena for dynamic memory allocation.
 * @param prefix The prefix for the temporary filename.
 * @return A read-only span containing the generated temporary filename.
 */
ReadOnlySpan<char> SystemGenerateTempFilename(MemoryArena memoryArena, ReadOnlySpan<char> prefix);

/**
 * Retrieves the executable folder path.
 *
 * @param memoryArena Pointer to the memory arena for dynamic memory allocation.
 * @return A read-only span containing the executable folder path.
 */
ReadOnlySpan<char> SystemGetExecutableFolderPath(MemoryArena memoryArena);

/**
 * Checks if a file exists at the specified path.
 *
 * @param path The path to the file.
 * @return True if the file exists; otherwise, false.
 */
bool SystemFileExists(ReadOnlySpan<char> path);

/**
 * Writes an array of bytes to a file at the specified path.
 *
 * @param path The path to the file.
 * @param data A read-only span containing the data to be written.
 */
void SystemFileWriteBytes(ReadOnlySpan<char> path, ReadOnlySpan<uint8_t> data);

/**
 * Reads the contents of a file into a span of bytes.
 *
 * @param memoryArena Pointer to the memory arena for dynamic memory allocation.
 * @param path The path to the file.
 * @return A span containing the read bytes.
 */
Span<uint8_t> SystemFileReadBytes(MemoryArena memoryArena, ReadOnlySpan<char> path);

/**
 * Deletes the file at the specified path.
 *
 * @param path The path to the file to be deleted.
 */
void SystemFileDelete(ReadOnlySpan<char> path);


//---------------------------------------------------------------------------------------------------------------
// Library / Process Functions
//---------------------------------------------------------------------------------------------------------------

/**
 * Represents a dynamic-link library handle.
 */
struct SystemLibrary
{
    void* Handle;
};

/**
 * Executes a system process with the specified command.
 *
 * @param memoryArena The memory arena for allocating temporary memory.
 * @param command The command to be executed.
 * @return A read-only span containing the output of the executed process.
 */
ReadOnlySpan<char> SystemExecuteProcess(MemoryArena memoryArena, ReadOnlySpan<char> command);

/**
 * Loads a dynamic-link library with the given library name.
 *
 * @param libraryName The name of the library to be loaded.
 * @return A SystemLibrary structure representing the loaded library.
 */
SystemLibrary SystemLoadLibrary(ReadOnlySpan<char> libraryName);

/**
 * Frees the specified dynamic-link library.
 *
 * @param library A SystemLibrary structure representing the loaded library.
 */
void SystemFreeLibrary(SystemLibrary library);

/**
 * Retrieves the address of an exported function from the specified dynamic-link library.
 *
 * @param library A SystemLibrary structure representing the loaded library.
 * @param functionName The name of the function to be retrieved.
 * @return A pointer to the exported function.
 */
void* SystemGetFunctionExport(SystemLibrary library, ReadOnlySpan<char> functionName);


//---------------------------------------------------------------------------------------------------------------
// Threading functions
//---------------------------------------------------------------------------------------------------------------

/**
 * Represents a system thread handle.
 */
struct SystemThread
{
    void* Handle;
};

/**
 * Atomically replaces the value of a variable if it matches the expected value.
 *
 * @param destination         The variable to update.
 * @param expectedExpression  The value to compare against the destination.
 * @param replaceExpression   The new value to set if the destination matches expectedExpression.
 */
#define SystemAtomicReplace(destination, expectedExpression, replaceExpression) \
        {\
        auto expected = (expectedExpression);\
        auto desired = (replaceExpression); \
        while (!__atomic_compare_exchange_n(&(destination), &(expected), desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) \
        {\
            SystemYieldThread();\
            expected = (expectedExpression);\
            desired = (replaceExpression);\
        }\
        }\

/**
 * Atomically loads the value of a variable.
 *
 * @param source        The variable to load from.
 * @param destination   The variable to store the loaded value.
 */
#define SystemAtomicLoad(source, destination) __atomic_load(&(source), &(destination), __ATOMIC_ACQUIRE)

/**
 * Atomically stores a value in a variable.
 *
 * @param destination  The variable to store the value in.
 * @param value        The value to store.
 */
#define SystemAtomicStore(destination, value) __atomic_store_n(&(destination), (value), __ATOMIC_RELEASE)

/**
 * Atomically adds a value to a variable.
 *
 * @param destination  The variable to add to.
 * @param value        The value to add.
 */
#define SystemAtomicAdd(destination, value) __atomic_fetch_add(&(destination), (value), __ATOMIC_SEQ_CST)

/**
 * Atomically subtracts a value from a variable.
 *
 * @param destination  The variable to subtract from.
 * @param value        The value to subtract.
 */
#define SystemAtomicSubstract(destination, value) __atomic_fetch_sub(&(destination), (value), __ATOMIC_SEQ_CST)

/**
 * Atomically compares the variable to an expected value and, if they are equal, replaces it with a new value.
 *
 * @param destination     The variable to compare and potentially update.
 * @param expectedValue   The value to compare against the destination.
 * @param value           The new value to set if the destination matches expectedValue.
 */
#define SystemAtomicCompareExchange(destination, expectedValue, value) __atomic_compare_exchange_n(&(destination), &(expectedValue), (value), true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)

/**
 * Function signature for a system thread.
 */
typedef void (*SystemThreadFunction)(void* parameters);

/**
 * Creates a system thread with the specified thread function and parameters.
 *
 * This function initializes a new thread, setting up its execution context with the provided function and parameters.
 *
 * @param threadFunction The function to be executed by the created thread. This is a pointer to a function of type SystemThreadFunction.
 * @param parameters A void pointer representing the parameters to be passed to the thread function.
 * @return A SystemThread structure representing the created thread. This structure contains a handle to the new thread.
 */
SystemThread SystemCreateThread(SystemThreadFunction threadFunction, void* parameters);

/**
 * Waits for the specified system thread to complete its execution.
 *
 * This function blocks the calling thread until the specified thread has finished executing. It should be used to synchronize the end of a thread's lifecycle.
 *
 * @param thread A SystemThread structure representing the thread to wait for. It should have been created using SystemCreateThread.
 */
void SystemWaitThread(SystemThread thread);

void SystemYieldThread();

/**
 * Frees the resources associated with the specified system thread.
 *
 * This function should be called after the thread has completed its execution or when it's no longer needed. It ensures that any resources allocated to the thread are properly released.
 *
 * @param thread A SystemThread structure representing the thread to be freed. This thread should have been created using SystemCreateThread.
 */
void SystemFreeThread(SystemThread thread);

