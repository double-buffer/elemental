using Elemental.Tools.CodeGenerator;

var codeGenerators = new Dictionary<string, ICodeGenerator> 
{
    { "c-loader", new CLoaderCodeGenerator() },
    { "dotnet", new DotnetCodeGenerator() }
}.ToFrozenDictionary();

if (args.Length != 3)
{
    Console.WriteLine("Error: Wrong number of arguments!");    
    return 1;
}

// TODO: Add a caching mechanism to only re generate when needed

var type = args[0];
var inputFile = args[1];
var output = args[2];

if (!codeGenerators.ContainsKey(type))
{
    Console.WriteLine("Error: Wrong type of code generator!");    
    return 1;
}

Console.WriteLine($"Generating {type} from '{inputFile}' to '{output}'...");

var options = GetCppParserOptions();
var compilation = CppParser.ParseFile(inputFile, options);

if (compilation.HasErrors)
{
    foreach (var message in compilation.Diagnostics.Messages)
    {
        Console.WriteLine($"{message.Type}: {message.Text}");
    }
}

var codeGenerator = codeGenerators[type];
codeGenerator.GenerateCode(compilation, File.ReadAllText(inputFile), inputFile, output);

return 0;

CppParserOptions GetCppParserOptions()
{
    var options = new CppParserOptions();
    options.ParseSystemIncludes = false;

    if (OperatingSystem.IsMacOS())
    {
        options.TargetSystem = "darwin";
        options.TargetVendor = "apple";
        options.SystemIncludeFolders.Add("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include");
        options.SystemIncludeFolders.Add("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1");
        options.SystemIncludeFolders.Add("/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include");
        options.SystemIncludeFolders.Add("/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1");
        options.SystemIncludeFolders.Add("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/11.0.0/include");
        options.AdditionalArguments.Add("-stdlib=libc++");
    }

    return options;
}
