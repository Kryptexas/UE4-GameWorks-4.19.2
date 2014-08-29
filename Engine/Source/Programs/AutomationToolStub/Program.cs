using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Reflection;
using AutomationTool;

namespace AutomationToolStub
{
	class Program
	{
		// This program can be used to build and debug AutomationTool and any automation scripts from the IDE.
		//
		// AutomationTool's default behavior is to find and compile assemblies at runtime, which is inconvenient for diagnosing compile errors and causes sharing violations if 
		// the debugger has already loaded the assemblies.
		//
		// UnrealBuildTool creates the AutomationHost project from Engine/Source/Programs/AutomationHostTemplate.csproj while generating project files, and adds explicit references
		// to the other automation modules in the solution so that the IDE can compile them normally. The -nocompile is set on UAT to prevent it compiling them a second time.

		static void Main(string[] args)
		{
			AutomationTool.GlobalCommandLine.NoCompile.Set();
			AutomationTool.Program.Main();
		}
	}
}
