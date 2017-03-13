using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	/// <summary>
	/// A DLL built by MSBuild from a C# project.
	/// </summary>
	class UEBuildBinaryCSDLL : UEBuildBinary
	{
		/// <summary>
		/// Create an instance initialized to the given configuration
		/// </summary>
		/// <param name="InConfig">The build binary configuration to initialize the instance to</param>
		public UEBuildBinaryCSDLL(UEBuildBinaryConfiguration InConfig)
			: base(InConfig)
		{
		}

		/// <summary>
		/// Builds the binary.
		/// </summary>
		/// <param name="Target">The target rules</param>
		/// <param name="ToolChain">The toolchain to use for building</param>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <param name="SharedPCHs">List of available shared PCHs</param>
		/// <param name="ActionGraph">The graph to add build actions to</param>
		/// <returns>Set of produced build artifacts</returns>
		public override IEnumerable<FileItem> Build(ReadOnlyTargetRules Target, UEToolChain ToolChain, CppCompileEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment, List<PrecompiledHeaderTemplate> SharedPCHs, ActionGraph ActionGraph)
		{
			CSharpEnvironment ProjectCSharpEnviroment = new CSharpEnvironment();
			if (LinkEnvironment.Configuration == CppConfiguration.Debug)
			{
				ProjectCSharpEnviroment.TargetConfiguration = CSharpTargetConfiguration.Debug;
			}
			else
			{
				ProjectCSharpEnviroment.TargetConfiguration = CSharpTargetConfiguration.Development;
			}
			ProjectCSharpEnviroment.EnvironmentTargetPlatform = LinkEnvironment.Platform;

			ToolChain.CompileCSharpProject(ProjectCSharpEnviroment, Config.ProjectFilePath, Config.OutputFilePath, ActionGraph);

			return new FileItem[] { FileItem.GetItemByFileReference(Config.OutputFilePath) };
		}
	}
}
