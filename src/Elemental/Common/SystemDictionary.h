#pragma once

#include "SystemMemory.h"

/**
 * Template structure for dictionary storage, specialized by value type.
 */
template<typename TValue>
struct SystemDictionaryStorage;

/**
 * A dictionary data structure template, mapping keys of type TKey to values of type TValue.
 * 
 * @tparam TKey The type of the keys.
 * @tparam TValue The type of the values.
 */
template<typename TKey, typename TValue>
struct SystemDictionary
{
    SystemDictionaryStorage<TValue>* Storage; ///< Pointer to the storage structure of the dictionary.
    
    /**
     * Overloads the [] operator to access values associated with a given key.
     * @param key The key of the value to access.
     * @return Reference to the value associated with the key.
     */
    TValue& operator[](TKey key);
};

/**
 * Creates a dictionary with a specified maximum number of items.
 * 
 * @tparam TKey The type of the keys.
 * @tparam TValue The type of the values.
 * @param memoryArena The memory arena where the dictionary is to be allocated.
 * @param maxItemsCount The maximum number of items the dictionary can hold.
 * @return A SystemDictionary instance.
 */
template<typename TKey, typename TValue>
SystemDictionary<TKey, TValue> SystemCreateDictionary(MemoryArena memoryArena, size_t maxItemsCount);

/**
 * Adds a new entry to the specified dictionary.
 * @tparam TKey The type of the keys.
 * @tparam TValue The type of the values.
 * @param dictionary The dictionary to which the entry is to be added.
 * @param key The key for the new entry.
 * @param value The value for the new entry.
 */
template<typename TKey, typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key, TValue value);

/**
 * Adds a new entry to the specified dictionary.
 * @tparam TValue The type of the values.
 * @param dictionary The dictionary to which the entry is to be added.
 * @param key The key for the new entry.
 * @param value The value for the new entry.
 */
template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key, TValue value);

/**
 * Adds a new entry to the specified dictionary.
 * @tparam TValue The type of the values.
 * @tparam T The type of the key element.
 * @param dictionary The dictionary to which the entry is to be added.
 * @param key The key for the new entry.
 * @param value The value for the new entry.
 */
template<typename TValue, typename T>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key, TValue value);

/**
 * Removes an entry from the specified dictionary.
 * @tparam TKey The type of the keys.
 * @tparam TValue The type of the values.
 * @param dictionary The dictionary from which the entry is to be removed.
 * @param key The key of the entry to remove.
 */
template<typename TKey, typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key);

/**
 * Removes an entry from the specified dictionary.
 * @tparam TValue The type of the values.
 * @param dictionary The dictionary from which the entry is to be removed.
 * @param key The key of the entry to remove.
 */
template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

/**
 * Removes an entry from the specified dictionary.
 * @tparam TValue The type of the values.
 * @tparam T The type of the key element.
 * @param dictionary The dictionary from which the entry is to be removed.
 * @param key The key of the entry to remove.
 */
template<typename TValue, typename T>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

/**
 * Retrieves the value for a specified key from the dictionary, if it exists.
 * @tparam TKey The type of the keys.
 * @tparam TValue The type of the values.
 * @param dictionary The dictionary from which to retrieve the value.
 * @param key The key of the value to retrieve.
 * @return Pointer to the value, or nullptr if the key does not exist.
 */
template<typename TKey, typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<TKey, TValue> dictionary, TKey key);

/**
 * Retrieves the value for a specified key from the dictionary, if it exists.
 * @tparam TValue The type of the values.
 * @param dictionary The dictionary from which to retrieve the value.
 * @param key The key of the value to retrieve.
 * @return Pointer to the value, or nullptr if the key does not exist.
 */
template<typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

/**
 * Retrieves the value for a specified key from the dictionary, if it exists.
 * @tparam TValue The type of the values.
 * @tparam T The type of the key element.
 * @param dictionary The dictionary from which to retrieve the value.
 * @param key The key of the value to retrieve.
 * @return Pointer to the value, or nullptr if the key does not exist.
 */
template<typename TValue, typename T>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

/**
 * Checks if the dictionary contains a given key.
 * @tparam TKey The type of the keys.
 * @tparam TValue The type of the values.
 * @param dictionary The dictionary to check.
 * @param key The key to look for.
 * @return True if the key exists in the dictionary, false otherwise.
 */
template<typename TKey, typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<TKey, TValue> dictionary, TKey key);

/**
 * Checks if the dictionary contains a given key.
 * @tparam TValue The type of the values.
 * @param dictionary The dictionary to check.
 * @param key The key to look for.
 * @return True if the key exists in the dictionary, false otherwise.
 */
template<typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

/**
 * Checks if the dictionary contains a given key.
 * @tparam TValue The type of the values.
 * @tparam T The type of the key element.
 * @param dictionary The dictionary to check.
 * @param key The key to look for.
 * @return True if the key exists in the dictionary, false otherwise.
 */
template<typename TValue, typename T>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

/**
 * Prints debug information for a given dictionary. This function is useful for 
 * development and debugging purposes to inspect the contents and state of the dictionary.
 * It outputs key-value pairs, the structure of the storage, and other relevant information 
 * that aids in understanding the dictionary's current state.
 * 
 * @tparam TKey The type of the keys in the dictionary.
 * @tparam TValue The type of the values in the dictionary.
 * @param dictionary The dictionary to debug.
 */
template<typename TKey, typename TValue>
void SystemDebugDictionary(SystemDictionary<TKey, TValue> dictionary);
