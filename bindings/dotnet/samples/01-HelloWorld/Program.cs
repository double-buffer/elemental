﻿using Elemental;

static void LogMessageHandler(LogMessageType messageType, LogMessageCategory category, string function, string message) 
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
    Console.Write($" {function}");

    Console.ForegroundColor = mainForegroundColor;
    Console.WriteLine($" {message}");
    Console.ForegroundColor = ConsoleColor.Gray;
}

var counter = 0;

using var applicationService = new NativeApplicationService(new() { LogMessageHandler = LogMessageHandler });
using var application = applicationService.CreateApplication("Hello World");

applicationService.RunApplication(application, (status) =>
{
    if (counter > 10 || !status.IsActive)
    {
        return false;
    }

    Console.WriteLine($"Hello World {counter}!");
    counter++;
    return true;
});