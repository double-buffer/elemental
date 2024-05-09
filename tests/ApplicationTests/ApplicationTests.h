#pragma once

#include <functional>
#include "Elemental.h"

void RunApplicationTest(std::function<void()> testFunction)
{
    testFunction();
}

