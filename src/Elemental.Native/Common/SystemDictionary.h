#pragma once

#include "SystemMemory.h"

template<typename TValue>
struct SystemDictionaryStorage;

template<typename TKey, typename TValue>
struct SystemDictionary
{
    SystemDictionaryStorage<TValue>* Storage;
    
    TValue& operator[](TKey key);
};

template<typename TKey, typename TValue>
struct SystemDictionaryEnumerator
{
    SystemDictionary<TKey, TValue> Dictionary;
    size_t PartitionIndex;
    size_t Index;
};

template<typename TKey, typename TValue>
SystemDictionary<TKey, TValue> SystemCreateDictionary(MemoryArena memoryArena, size_t maxItemsCount);

template<typename TKey, typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key, TValue value);

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key, TValue value);

template<typename TValue, typename T>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key, TValue value);

template<typename TKey, typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

template<typename TValue, typename T>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

template<typename TKey, typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

template<typename TValue, typename T>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

template<typename TKey, typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

template<typename TValue, typename T>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

template<typename TKey, typename TValue>
SystemDictionaryEnumerator<TKey, TValue> SystemGetDictionaryEnumerator(SystemDictionary<TKey, TValue> dictionary);

template<typename TKey, typename TValue>
TValue* SystemGetDictionaryEnumeratorNextValue(SystemDictionaryEnumerator<TKey, TValue>* enumerator);

template<typename TKey, typename TValue>
void SystemDebugDictionary(SystemDictionary<TKey, TValue> dictionary);
