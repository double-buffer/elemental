namespace Elemental.Tests;

class Program
{
    static object consoleLock = new object();
    static ManualResetEvent finished = new ManualResetEvent(false);
    static int result = 0;

    static int Main(string[] args)
    {
        Console.WriteLine("Test");

        var path = System.Reflection.Assembly.GetEntryAssembly()!.Location;
        Console.WriteLine($"Path: {path}");

        using var runner = AssemblyRunner.WithoutAppDomain(path);

        runner.OnDiscoveryComplete = OnDiscoveryComplete;
        runner.OnExecutionComplete = OnExecutionComplete;
        runner.OnTestPassed = OnTestPassed;
        runner.OnTestFailed = OnTestFailed;
        runner.OnTestSkipped = OnTestSkipped;

        Console.WriteLine("Scanning tests...");
        runner.Start();
        Console.WriteLine("Done");

        finished.WaitOne();
        finished.Dispose();

        return result;
    }

    static void OnDiscoveryComplete(DiscoveryCompleteInfo info)
    {
        lock (consoleLock)
            Console.WriteLine($"Running {info.TestCasesToRun} of {info.TestCasesDiscovered} tests...");
    }

    static void OnExecutionComplete(ExecutionCompleteInfo info)
    {
        lock (consoleLock)
            Console.WriteLine($"Finished: {info.TotalTests} tests in {Math.Round(info.ExecutionTime, 3)}s ({info.TestsFailed} failed, {info.TestsSkipped} skipped)");

        finished.Set();
    }

    static void OnTestPassed(TestPassedInfo info)
    {
        lock (consoleLock)
        {
            Console.ForegroundColor = ConsoleColor.Green;

            Console.WriteLine("[PASS] {0}", info.TestDisplayName);
            Console.ResetColor();
        }

        result = 1;
    }

    static void OnTestFailed(TestFailedInfo info)
    {
        lock (consoleLock)
        {
            Console.ForegroundColor = ConsoleColor.Red;

            Console.WriteLine("[FAIL] {0}: {1}", info.TestDisplayName, info.ExceptionMessage);
            if (info.ExceptionStackTrace != null)
                Console.WriteLine(info.ExceptionStackTrace);

            Console.ResetColor();
        }

        result = 1;
    }

    static void OnTestSkipped(TestSkippedInfo info)
    {
        lock (consoleLock)
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("[SKIP] {0}: {1}", info.TestDisplayName, info.SkipReason);
            Console.ResetColor();
        }
    }
}
