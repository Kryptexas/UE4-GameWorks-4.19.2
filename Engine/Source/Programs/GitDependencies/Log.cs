using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GitDependencies
{
	static class Log
	{
		static string CurrentStatus = "";

		public static void WriteLine()
		{
			FlushStatus();
			Console.WriteLine();
		}

		public static void WriteLine(string Line)
		{
			FlushStatus();
			Console.WriteLine(Line);
		}

		public static void WriteLine(string Format, params object[] Args)
		{
			FlushStatus();
			Console.WriteLine(Format, Args);
		}

		public static void WriteError(string Format, params object[] Args)
		{
			FlushStatus();
			Console.ForegroundColor = ConsoleColor.Red;
			Console.WriteLine(Format, Args);
			Console.ResetColor();
		}

		public static void WriteStatus(string Format, params object[] Args)
		{
			string NewStatus = String.Format(Format, Args);

			// Figure out how many characters are in common
			int NumCommon = 0;
			while(NumCommon < CurrentStatus.Length && NumCommon < NewStatus.Length && CurrentStatus[NumCommon] == NewStatus[NumCommon])
			{
				NumCommon++;
			}

			// Go back to the point that they diverge and write the new string
			Console.Write(new String('\b', CurrentStatus.Length - NumCommon));
			Console.Write(NewStatus.Substring(NumCommon));

			// If the new message is shorter, clear the rest of the line
			if(NewStatus.Length < CurrentStatus.Length)
			{
				Console.Write(new String(' ', CurrentStatus.Length - NewStatus.Length));
				Console.Write(new String('\b', CurrentStatus.Length - NewStatus.Length));
			}

			// Update the status message
			CurrentStatus = NewStatus;
		}

		public static void FlushStatus()
		{
			if(CurrentStatus.Length > 0)
			{
				Console.WriteLine();
				CurrentStatus = "";
			}
		}
	}
}
