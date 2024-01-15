namespace Elemental.Tests.Common;

public static class Assert
{
    public static void True(bool value, [CallerFilePath]string sourceFilePath = "", [CallerLineNumber]int sourceLineNumber = 0)
    {
        if (!value)
        {
            TestRunner.TestContext.Value = false;

            Console.WriteLine($"{sourceFilePath}:{sourceLineNumber}: Failure");
            Console.WriteLine("Expected: True");
            Console.WriteLine($"Actual: {value}");
        }
    }

    public static void NotEqual<T>(T expected, T actual, [CallerFilePath]string sourceFilePath = "", [CallerLineNumber]int sourceLineNumber = 0) where T : IEquatable<T>
    {
        if (expected.Equals(actual))
        {
            TestRunner.TestContext.Value = false;

            Console.WriteLine($"{sourceFilePath}:{sourceLineNumber}: Failure");
            Console.WriteLine($"Expected: ({expected}) != (result)");
            Console.WriteLine($"Actual: {expected} vs {actual}");
        }
    }
}
