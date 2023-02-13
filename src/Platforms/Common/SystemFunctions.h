
uint32_t GetThreadId()
{
    #ifdef _WINDOWS
    return GetCurrentThreadId();
    #endif
}