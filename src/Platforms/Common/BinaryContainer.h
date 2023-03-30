#pragma once

class BinaryContainer
{
public:
    BinaryContainer()
    {
        _data = nullptr;
        _size = 0;
    }

    void SetData(void* sourceData, uint32_t size)
    {
        _size = size;
        _data = new uint8_t[size];
        memcpy(_data, sourceData, size);
    }

    ~BinaryContainer()
    {
        delete[] _data;
    }

    void* GetBufferPointer()
    {
        return _data;
    }

    uint32_t GetBufferSize()
    {
        return _size;
    }

    bool IsEmpty()
    {
        return _size == 0;
    }

private:
    void* _data;
    uint32_t _size;
};