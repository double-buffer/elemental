#pragma once

#include "ElementalTools.h"
#include "SystemMemory.h"

MemoryArena GetSceneLoaderMemoryArena();
ElemToolsMatrix4x4 CreateSceneLoaderGlobalTransformMatrix(const ElemLoadSceneOptions* options);
