namespace Elemental;

/// <summary>
/// Defines a function pointer type for handling application events.
/// </summary>
public delegate void ApplicationHandler<T>(ref T payload) where T : unmanaged;
