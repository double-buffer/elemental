namespace Elemental.Tests.Common;

public static class Assert
{
    public static void True(bool value)
    {
        if (!value)
        {
            Console.WriteLine("NOK");
        }

        Console.WriteLine(value);
    }
}
