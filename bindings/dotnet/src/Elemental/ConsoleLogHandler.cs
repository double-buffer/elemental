namespace Elemental;

/// <summary>
/// Class that contains a default log handler implementation.
/// </summary>
public static class DefaultLogHandlers
{
    /// <summary>
    /// Default console log handler.
    /// </summary>
    /// <param name="messageType">Message type</param>
    /// <param name="category">Category</param>
    /// <param name="function">Function</param>
    /// <param name="message">Message</param>
    public static void ConsoleLogHandler(LogMessageType messageType, LogMessageCategory category, ReadOnlySpan<byte> function, ReadOnlySpan<byte> message) 
    {
        var mainForegroundColor = messageType switch
        {
            LogMessageType.Warning => ConsoleColor.Yellow,
                LogMessageType.Error => ConsoleColor.Red,
                _ => ConsoleColor.Gray
        };

        Console.Write("[");
        Console.ForegroundColor = ConsoleColor.Cyan;
        Console.Write($"{category}");
        Console.ForegroundColor = ConsoleColor.Gray;
        Console.Write("]");

        Console.ForegroundColor = ConsoleColor.Green;
        Console.Write($" {System.Text.UTF8Encoding.UTF8.GetString(function)}");

        Console.ForegroundColor = mainForegroundColor;
        Console.WriteLine($" {messageType}: {System.Text.UTF8Encoding.UTF8.GetString(message)}");
        Console.ForegroundColor = ConsoleColor.Gray;
    }
}
