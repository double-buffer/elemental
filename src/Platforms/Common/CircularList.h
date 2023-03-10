#pragma once

template<typename T>
class CircularList
{
public:
    CircularList(uint32_t maxSize, T defaultValue = T())
    {
        _maxSize = maxSize;
        _currentIndex = 0;
        _data = new T[maxSize];

        for (uint32_t i = 0; i < maxSize; i++)
        {
            _data[i] = defaultValue;
        }
    }

    ~CircularList()
    {
        delete[] _data;
    }

    void GetCurrentItemPointerAndMove(T** value)
    {
        auto localIndex = InterlockedIncrement(&_currentIndex) % _maxSize;
        *value = &_data[localIndex];
    }

private:
    T* _data;
    uint32_t _maxSize;
    uint64_t _currentIndex;
};