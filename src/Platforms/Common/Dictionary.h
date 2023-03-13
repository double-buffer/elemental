
// https://github.com/EpicGames/UnrealEngine/blob/5ca9da84c694c6eee288c30a547fcaa1a40aed9b/Engine/Source/Runtime/Core/Public/Containers/Map.h

// TODO: Basic implementation with std:map first

#pragma once

// HACK: Temporary
#include <map>

template<typename TKey, typename TValue>
class Dictionary
{
public:
    Dictionary()
    {
    }

    ~Dictionary()
    {
    }

    bool ContainsKey(TKey key)
    {
        return _map.count(key) != 0;
    }

    void Add(TKey key, TValue value)
    {
        _map[key] = value;
    }

    TValue& operator[](TKey key)
    {
        return _map[key];
    }

private:
    std::map<TKey, TValue> _map;
};