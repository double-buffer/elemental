using Elemental.Tests.NativeApplicationTests;

var runner = new TestRunner();

var test = new NativeApplicationServiceTests();
runner.AddTest($"{nameof(NativeApplicationServiceTests)}.{nameof(test.Constructor_WithLogger_WriteInitOKMessage)}", test.Constructor_WithLogger_WriteInitOKMessage);
runner.AddTest($"{nameof(NativeApplicationServiceTests)}.{nameof(test.CreateApplication_WithName_ReturnsValidObject)}", test.CreateApplication_WithName_ReturnsValidObject);

runner.RunTests();
