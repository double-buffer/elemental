namespace Elemental;

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate bool RunHandler(ApplicationStatus status);
