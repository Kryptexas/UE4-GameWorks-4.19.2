using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	public enum OutputLevel
	{
		Error,
		Warning,
		Info,
		Profile,
		Diagnostic,
		Verbose,
	}

	public abstract class OutputDevice
	{
		public abstract void WriteLine(OutputLevel Level, string Message);

		public void WriteLine(OutputLevel Level, string Format, params object[] Args)
		{
			WriteLine(Level, String.Format(Format, Args));
		}

		public void TraceError(string Message)
		{
			WriteLine(OutputLevel.Error, Message);
		}

		public void TraceError(string Format, params object[] Args)
		{
			WriteLine(OutputLevel.Error, Format, Args);
		}

		public void TraceWarning(string Message)
		{
			WriteLine(OutputLevel.Warning, Message);
		}

		public void TraceWarning(string Format, params object[] Args)
		{
			WriteLine(OutputLevel.Warning, Format, Args);
		}

		public void TraceInfo(string Message)
		{
			WriteLine(OutputLevel.Info, Message);
		}

		public void TraceInfo(string Format, params object[] Args)
		{
			WriteLine(OutputLevel.Info, Format, Args);
		}

		public void TraceVerbose(string Message)
		{
			WriteLine(OutputLevel.Verbose, Message);
		}

		public void TraceVerbose(string Format, params object[] Args)
		{
			WriteLine(OutputLevel.Verbose, Format, Args);
		}
	}

	sealed class OutputDeviceFile : OutputDevice, IDisposable
	{
		StreamWriter Writer;

		public OutputDeviceFile(string FullName)
		{
			Writer = new StreamWriter(File.Open(FullName, FileMode.Create, FileAccess.Write, FileShare.Read));
			Writer.AutoFlush = true;
		}

		public void Dispose()
		{
			Writer.Dispose();
		}

		public override void WriteLine(OutputLevel Level, string Message)
		{
			Writer.WriteLine(Message);
		}
	}

	class OutputDeviceBuffer : OutputDevice
	{
		List<Tuple<OutputLevel, string>> Messages = new List<Tuple<OutputLevel, string>>();

		public override void WriteLine(OutputLevel Level, string Message)
		{
			Messages.Add(Tuple.Create(Level, Message));
		}

		public void FlushTo(OutputDevice Other)
		{
			foreach (Tuple<OutputLevel, string> Message in Messages)
			{
				Other.WriteLine(Message.Item1, Message.Item2);
			}
			Messages.Clear();
		}
	}

	class OutputDeviceConsole : OutputDevice
	{
		OutputLevel MinLevel;

		public OutputDeviceConsole(OutputLevel MinLevel)
		{
			this.MinLevel = MinLevel;
		}

		public override void WriteLine(OutputLevel Level, string Message)
		{
			if (Level >= MinLevel)
			{
				Console.WriteLine(Message);
			}
		}
	}

	class OutputDeviceSynchronized : OutputDevice
	{
		OutputDevice Inner;

		public OutputDeviceSynchronized(OutputDevice Inner)
		{
			this.Inner = Inner;
		}

		public override void WriteLine(OutputLevel Level, string Message)
		{
			lock (this)
			{
				Inner.WriteLine(Level, Message);
			}
		}
	}

	class OutputDeviceRedirector : OutputDevice
	{
		List<OutputDevice> Outputs = new List<OutputDevice>();

		public void Add(OutputDevice Output)
		{
			Outputs.Add(Output);
		}

		public void Remove(OutputDevice Output)
		{
			Outputs.Remove(Output);
		}

		public override void WriteLine(OutputLevel Level, string Message)
		{
			foreach (OutputDevice Output in Outputs)
			{
				Output.WriteLine(Level, Message);
			}
		}
	}
}
