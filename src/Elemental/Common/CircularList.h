#pragma once


// TODO: Refactor this structure later when we have integration tests on the command lists
// TODO: Can we have the same read/write struct so that it can be used by the input system? 
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
    uint64_t _currentIndex;
    uint32_t _maxSize;
};
