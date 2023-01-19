#pragma once
#include <stdbool.h>

enum NativeApplicationStatusFlags
{
    None = 0,
    Active = 1
};

struct NativeApplicationStatus
{
    unsigned int Status;
};

typedef bool (*RunHandlerPtr)(struct NativeApplicationStatus status);