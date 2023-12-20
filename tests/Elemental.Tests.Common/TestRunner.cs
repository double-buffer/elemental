namespace Elemental.Tests.Common;

public delegate void TestDelegate();

public record Test(string Name, TestDelegate TestDelegate);

public class TestRunner
{
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
        Console.ForegroundColor = ConsoleColor.Green; 
        Console.Write("[==========]");
        Console.ResetColor();
        Console.WriteLine($" Running {Tests.Count} test{(Tests.Count > 1 ? "s": "")}");

        foreach (var test in Tests)
        {
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write("[ RUN      ]");
            Console.ResetColor();
            Console.WriteLine($" {test.Name}");

            test.TestDelegate();
        }

        Console.ForegroundColor = ConsoleColor.Green; 
        Console.WriteLine("[==========]");
        Console.ResetColor();
    }
}
