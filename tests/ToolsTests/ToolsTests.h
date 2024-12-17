#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "ElementalTools.h"
#include "utest.h"

void ConfigureTestFileIO();
void AddTestFile(const char* path, ElemToolsDataSpan data);
