#pragma once
#include "BaseGraphicsService.h"

class BaseGraphicsObject
{
public:
    BaseGraphicsObject(BaseGraphicsService* graphicsService)
    {
        GraphicsService = graphicsService;
    }

public:
    BaseGraphicsService* GraphicsService;
};