#if defined(_WIN32)
#include <windows.h>
#endif

#include "Elemental.h"

int ElementalTranslationUnit1(void)
{
    return (int)sizeof(ElemSystemInfo);
}
