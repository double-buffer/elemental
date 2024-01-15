namespace Elemental.Tests.Common;

public delegate void TestDelegate();

public record Test(string Name, TestDelegate TestDelegate);

public class TestRunner
{
    internal static ThreadLocal<bool> TestContext = new ThreadLocal<bool>(false);
    private readonly List<Test> Tests;

    public TestRunner()
    {
        Tests = new List<Test>();
    }

    public void AddTest(string name, TestDelegate testDelegate)
    {
        Tests.Add(new Test(name, testDelegate));
    }

    public void RunTests()
    {
        var stopwatch = new Stopwatch();

        Console.ForegroundColor = ConsoleColor.Green; 
        Console.Write("[==========]");
        Console.ResetColor();
        Console.WriteLine($" Running {Tests.Count} test{(Tests.Count > 1 ? "s": "")}");

        var passedCount = 0;
        var failedTests = new List<string>();

        foreach (var test in Tests)
        {
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write("[ RUN      ]");
            Console.ResetColor();
            Console.WriteLine($" {test.Name}");

            TestContext.Value = true;

            stopwatch.Restart();
            test.TestDelegate();
            stopwatch.Stop();

            if (TestContext.Value)
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.Write("[       OK ]");
                passedCount++;
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Write("[  FAILED  ]");
                failedTests.Add(test.Name);
            }
                
            Console.ResetColor();
            Console.WriteLine($" {test.Name}({stopwatch.Elapsed.Nanoseconds}ns)");
        }

        Console.ForegroundColor = ConsoleColor.Green; 
        Console.WriteLine("[==========]");
        Console.ResetColor();
        
        if (passedCount > 0)
        {
            Console.ForegroundColor = ConsoleColor.Green; 
            Console.Write("[  PASSED  ]");
            Console.ResetColor();
        
            Console.WriteLine($" {passedCount} test{(passedCount > 1 ? "s": "")}");
        }

        if (failedTests.Count > 0)
        {
            Console.ForegroundColor = ConsoleColor.Red; 
            Console.Write("[  FAILED  ]");
            Console.ResetColor();
        
            Console.WriteLine($" {failedTests.Count} test{(failedTests.Count > 1 ? "s": "")}, listed below:");

            foreach (var test in failedTests)
            {
                Console.ForegroundColor = ConsoleColor.Red; 
                Console.Write("[  FAILED  ]");
                Console.ResetColor();
                Console.WriteLine($" {test}");
            } 
        }
    }
}
