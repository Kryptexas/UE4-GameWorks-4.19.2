using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	/// <summary>
	/// Public Linux functions exposed to UAT
	/// </summary>
	public class LinuxExports
	{
		/// <summary>
		/// 
		/// </summary>
		/// <param name="SourceFileName"></param>
		/// <param name="TargetFileName"></param>
		public static void StripSymbols(string SourceFileName, string TargetFileName)
		{
			LinuxToolChain ToolChain = new LinuxToolChain(LinuxPlatform.DefaultArchitecture);
			ToolChain.StripSymbols(SourceFileName, TargetFileName);
		}
	}
}
