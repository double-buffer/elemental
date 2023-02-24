#pragma once
#include <stdint.h>

template<typename T>
struct FreeListNode
{
    FreeListNode(T data)
    {
        Data = data;
        Next = nullptr;
    }

    T Data;
    FreeListNode *Next;
};

template<typename T>
class FreeList
{
public:
    FreeList()
    {
        _rootNode = nullptr;
    }

    ~FreeList()
    {
        // TODO: Delete recurse
    }

    void Add(T value)
    {
        auto newRootNode = new FreeListNode(value);
        newRootNode->Next = _rootNode;

        if (InterlockedCompareExchangePointer((PVOID*)&_rootNode, newRootNode, newRootNode->Next) == newRootNode->Next)
        {
            return;
        }

        printf("Failed to add freelist node\n");
    }

    bool GetItem(T* value)
    {
        auto localRootNode = _rootNode;

        if (localRootNode == nullptr)
        {
            return false;
        }

        if (InterlockedCompareExchangePointer((PVOID*)&_rootNode, _rootNode->Next, localRootNode) == localRootNode)
        {
            *value = localRootNode->Data;

            delete localRootNode;
            return true;
        }
        
        printf("Failed to get Item freelist node\n");
        return false;
    }

private:
    FreeListNode<T>* _rootNode;
    uint32_t _count;
};