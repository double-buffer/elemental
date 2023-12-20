using Elemental.Tests.NativeApplicationTests;

var runner = new TestRunner();

var test = new NativeApplicationServiceTests();
runner.AddTest(nameof(test.Constructor_WithLogger_WriteInitOKMessage), test.Constructor_WithLogger_WriteInitOKMessage);

runner.RunTests();
