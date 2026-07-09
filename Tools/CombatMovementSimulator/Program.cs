namespace CombatMovementSimulator;

internal static class Program
{
	[STAThread]
	private static int Main(string[] args)
	{
		if (args.Any(argument => string.Equals(argument, "--headless-eval", StringComparison.OrdinalIgnoreCase)))
		{
			return HeadlessEvaluation.Run(args);
		}

		ApplicationConfiguration.Initialize();
		Application.Run(new SimulatorForm());
		return 0;
	}
}
