#pragma once
#include <stdint.h>

template<typename T>
class FreeList
{
public:
    FreeList() : FreeList(64)
    {
    }

    FreeList(uint32_t maxSize)
    {
        _maxSize = maxSize;
        _currentIndex = 0;
        _data = new T[maxSize];

        for (uint32_t i = 0; i < maxSize; i++)
        {
            _data[i] = T();
        }
    }

    ~FreeList()
    {
        delete[] _data;
    }
    
    void Add(T value)
    {
        printf("Add\n");

        assert(_currentIndex < _maxSize);
        _data[_currentIndex++] = value;
    }

    bool GetItem(T* value)
    { 
        printf("Get\n");

        if (_currentIndex > 0)
        {
            *value = _data[_currentIndex--];
            return true;
        }

        return false;
    }

private:
    T* _data;
    uint32_t _maxSize;
    uint64_t _currentIndex;
};