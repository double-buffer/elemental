#pragma once

struct NativeAppStatus
{
    unsigned int Status;
};

typedef int (*RunHandlerPtr)(struct NativeAppStatus status);