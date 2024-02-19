namespace Elemental;

/// <summary>
/// Defines a function pointer type for application run handling.
/// </summary>
/// <param name="status">The current application status.</param>
/// <returns>Returns true to continue running, false to exit.</returns>
[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate bool RunHandler(ApplicationStatus status);
