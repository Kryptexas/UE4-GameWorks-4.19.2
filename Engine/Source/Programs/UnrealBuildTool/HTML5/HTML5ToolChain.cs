// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace UnrealBuildTool
{
	class HTML5ToolChain : VCToolChain
	{
		// Debug options -- TODO: add these to SDK/Project setup?
		static bool bEnableTracing = false;

		// cache the location of SDK tools
		public HTML5ToolChain()
			: base(CppPlatform.HTML5, WindowsCompiler.VisualStudio2015)
		{
			if (!HTML5SDKInfo.IsSDKInstalled())
			{
				throw new BuildException("HTML5 SDK is not installed; cannot use toolchain.");
			}
		}

		public static void PreBuildSync()
		{
			Log.TraceInformation("Setting Emscripten SDK ");
			HTML5SDKInfo.SetupEmscriptenTemp();
			HTML5SDKInfo.SetUpEmscriptenConfigFile();

			if (Environment.GetEnvironmentVariable("EMSDK") == null) // If env. var EMSDK is present, Emscripten is already configured by the developer
			{
				// If not using preset emsdk, configure our generated .emscripten config, instead of autogenerating one in the user's home directory.
				Environment.SetEnvironmentVariable("EM_CONFIG", HTML5SDKInfo.DOT_EMSCRIPTEN);
				Environment.SetEnvironmentVariable("EM_CACHE", HTML5SDKInfo.EMSCRIPTEN_CACHE);
			}
		}

		static string GetSharedArguments_Global(CppConfiguration TargetConfiguration, string Architecture, bool bEnableShadowVariableWarnings, bool bShadowVariableWarningsAsErrors, bool bEnableUndefinedIdentifierWarnings, bool bUndefinedIdentifierWarningsAsErrors)
		{
			string Result = " ";

			if (Architecture == "-win32") // simulator
			{
				return Result;
			}

//			Result += " -funsigned-char";
//			Result += " -fno-strict-aliasing";
			Result += " -fno-exceptions";
//			Result += " -fno-short-enums";

			Result += " -Wno-unused-value"; // appErrorf triggers this
			Result += " -Wno-switch"; // many unhandled cases
			Result += " -Wno-tautological-constant-out-of-range-compare"; // disables some warnings about comparisons from TCHAR being a char
			// this hides the "warning : comparison of unsigned expression < 0 is always false" type warnings due to constant comparisons, which are possible with template arguments
			Result += " -Wno-tautological-compare";
			Result += " -Wno-inconsistent-missing-override"; // as of 1.35.0, overriding a member function but not marked as 'override' triggers warnings
			Result += " -Wno-expansion-to-defined"; // 1.36.11
			Result += " -Wno-undefined-var-template"; // 1.36.11
			Result += " -Wno-nonportable-include-path"; // 1.36.11

			// okay, in UE4, we'd fix the code for these, but in UE3, not worth it
			Result += " -Wno-logical-op-parentheses"; // appErrorf triggers this
			Result += " -Wno-array-bounds"; // some VectorLoads go past the end of the array, but it's okay in that case
			// needed to suppress warnings about using offsetof on non-POD types.
			Result += " -Wno-invalid-offsetof";
			// we use this feature to allow static FNames.
			Result += " -Wno-gnu-string-literal-operator-template";

			if (bEnableShadowVariableWarnings)
			{
				Result += " -Wshadow" + (bShadowVariableWarningsAsErrors ? "" : " -Wno-error=shadow");
			}

			if (bEnableUndefinedIdentifierWarnings)
			{
				Result += " -Wundef" + (bUndefinedIdentifierWarningsAsErrors ? "" : " -Wno-error=undef");
			}

			// JavsScript option overrides (see src/settings.js)

			// we have to specify the full amount of memory with Asm.JS (1.5 G)
//			ConfigHierarchy cc = ConfigCache.ReadHierarchy(ConfigHierarchyType.Engine, DirectoryReference.FromFile(ProjectFile), UnrealTargetPlatform.HTML5);
//			int TotalMemory = HTML5SDKInfo.HeapSize(cc, TargetConfiguration.ToString()) * 1024 * 1024;
//			Result += " -s TOTAL_MEMORY=" + TotalMemory.ToString();
			// and, have to specify size smaller than used and must use "grow memory" !!!
			Result += " -s TOTAL_MEMORY=" + 128 * 1024 * 1024;
			Result += " -s ALLOW_MEMORY_GROWTH=1";

			// no need for exceptions
			Result += " -s DISABLE_EXCEPTION_CATCHING=1";
			// enable checking for missing functions at link time as opposed to runtime
			Result += " -s WARN_ON_UNDEFINED_SYMBOLS=1";

			// enable hardware accelerated and advanced instructions/functions
//			Result += " -s USE_WEBGL2=1 -s FULL_ES3=1";
			Result += " -s FULL_ES2=1";
//			Result += " -msse";
//			Result += " -msse2";
//			Result += " -s SIMD=1";
//			Result += " -s USE_PTHREADS=1"; // 2:polyfill -- SharedInt\d+Array available by ES7

			// THESE ARE TEST/DEBUGGING
//			Environment.SetEnvironmentVariable("EMCC_DEBUG", "1");
//			Environment.SetEnvironmentVariable("EMCC_CORES", "8");
//			Environment.SetEnvironmentVariable("EMCC_OPTIMIZE_NORMALLY", "1");

			// export console command handler. Export main func too because default exports ( e.g Main ) are overridden if we use custom exported functions.
			Result += " -s EXPORTED_FUNCTIONS=\"['_main', '_on_fatal']\"";

			// NOTE: This may slow down the compiler's startup time!
			{
				Result += " -s NO_EXIT_RUNTIME=1 --memory-init-file 1";
			}

			if (bEnableTracing)
			{
				Result += " --tracing";
			}
			return Result;
		}

		static string GetCLArguments_Global(CppCompileEnvironment CompileEnvironment)
		{
			string Result = GetSharedArguments_Global(CompileEnvironment.Configuration, CompileEnvironment.Architecture, CompileEnvironment.bEnableShadowVariableWarnings, CompileEnvironment.bShadowVariableWarningsAsErrors, CompileEnvironment.bEnableUndefinedIdentifierWarnings, CompileEnvironment.bUndefinedIdentifierWarningsAsErrors);

			if (CompileEnvironment.Architecture != "-win32")  // ! simulator
			{
				// do we want debug info?
//				if (CompileEnvironment.bCreateDebugInfo)
//				{
//					Result += " -g";
//
//					// dump headers: http://stackoverflow.com/questions/42308/tool-to-track-include-dependencies
//					Result += " -H";
//				}

//				Result += " -Wno-warn-absolute-paths "; // as of emscripten 1.35.0 complains that this is unknown
				Result += " -Wno-reorder"; // we disable constructor order warnings.

				if (CompileEnvironment.Configuration == CppConfiguration.Debug || CompileEnvironment.Configuration == CppConfiguration.Development)
				{
					Result += " -s GL_ASSERTIONS=1";
				}

				if (!CompileEnvironment.bOptimizeCode)
				{
					Result += " -O0";
				}
				else // development & shipiing
				{
					Result += " -s ASM_JS=1";

					if (CompileEnvironment.bOptimizeForSize)
					{
						Result += " -Oz -s OUTLINING_LIMIT=40000";
					}
					else
					{
						Result += " -s OUTLINING_LIMIT=110000";

						if (CompileEnvironment.Configuration == CppConfiguration.Development)
						{
							Result += " -O2";
                        }
						if (CompileEnvironment.Configuration == CppConfiguration.Shipping)
						{
							Result += " -O3";
						}
					}
				}
			}

			return Result;
		}

		static string GetCLArguments_CPP(CppCompileEnvironment CompileEnvironment)
		{
			string Result = "";

			if (CompileEnvironment.Architecture != "-win32") // ! simulator
			{
				Result = " -std=c++14";
			}

			return Result;
		}

		static string GetCLArguments_C(string Architecture)
		{
			string Result = "";
			return Result;
		}

		string GetLinkArguments(LinkEnvironment LinkEnvironment)
		{
			string Result = GetSharedArguments_Global(LinkEnvironment.Configuration, LinkEnvironment.Architecture, false, false, false, false);

			if (LinkEnvironment.Architecture != "-win32") // ! simulator
			{
				// suppress link time warnings
				Result += " -Wno-ignored-attributes"; // function alias that always gets resolved
				Result += " -Wno-parentheses"; // precedence order
				Result += " -Wno-shift-count-overflow"; // 64bit is more than enough for shift 32

				// enable verbose mode
				Result += " -v";

				if (LinkEnvironment.Configuration == CppConfiguration.Debug || LinkEnvironment.Configuration == CppConfiguration.Development)
				{
					// check for alignment/etc checking
//					Result += " -s SAFE_HEAP=1";
					//Result += " -s CHECK_HEAP_ALIGN=1";
					//Result += " -s SAFE_DYNCALLS=1";

					// enable assertions in non-Shipping/Release builds
					Result += " -s ASSERTIONS=1";
				}

				if (LinkEnvironment.Configuration == CppConfiguration.Debug)
				{
					Result += " -O0";
				}
				else // development & shipping
				{
					Result += " -s ASM_JS=1";

					if (LinkEnvironment.bOptimizeForSize)
					{
						Result += " -Oz -s OUTLINING_LIMIT=40000";
					}
					else
					{
						Result += " -s OUTLINING_LIMIT=110000";

						if (LinkEnvironment.Configuration == CppConfiguration.Development)
						{
							Result += " -O2";
						}
						if (LinkEnvironment.Configuration == CppConfiguration.Shipping)
						{
							Result += " -O3";
						}
					}
				}

				Result += " -s CASE_INSENSITIVE_FS=1";
//				Result += " -s EVAL_CTORS=1";
			}

			return Result;
		}

		static string GetLibArguments(LinkEnvironment LinkEnvironment)
		{
			string Result = "";

			if (LinkEnvironment.Architecture == "-win32") // simulator
			{
				// Prevents the linker from displaying its logo for each invocation.
				Result += " /NOLOGO";

				// Prompt the user before reporting internal errors to Microsoft.
				Result += " /errorReport:prompt";

				// Win32 build
				Result += " /MACHINE:x86";

				// Always CONSOLE because of main()
				Result += " /SUBSYSTEM:CONSOLE";

				//
				//	Shipping & LTCG
				//
				if (LinkEnvironment.Configuration == CppConfiguration.Shipping)
				{
					// Use link-time code generation.
					Result += " /ltcg";
				}

				return Result;
			}

			return Result;
		}

		public static void CompileOutputReceivedDataEventHandler(Object Sender, DataReceivedEventArgs Line)
		{
			var Output = Line.Data;
			if (Output == null)
			{
				return;
			}

			Output = Output.Replace("\\", "/");
			// Need to match following for clickable links
			string RegexFilePath = @"^([\/A-Za-z0-9_\-\.]*)+\.(cpp|c|mm|m|hpp|h)";
			string RegexFilePath2 = @"^([A-Z]:[\/A-Za-z0-9_\-\.]*)+\.(cpp|c|mm|m|hpp|h)";
			string RegexLineNumber = @"\:\d+\:\d+\:";
			string RegexDescription = @"(\serror:\s|\swarning:\s).*";

			// Get Matches
			string MatchFilePath = Regex.Match(Output, RegexFilePath).Value;
			if (MatchFilePath.Length == 0)
			{
				MatchFilePath = Regex.Match(Output, RegexFilePath2).Value;
			}
			string MatchLineNumber = Regex.Match(Output, RegexLineNumber).Value;
			string MatchDescription = Regex.Match(Output, RegexDescription).Value;

			// If any of the above matches failed, do nothing
			if (MatchFilePath.Length == 0 ||
				MatchLineNumber.Length == 0 ||
				MatchDescription.Length == 0)
			{
				Log.TraceWarning(Output);
				return;
			}

			// Convert Path
			string RegexStrippedPath = @"(Engine\/|[A-Za-z0-9_\-\.]*\/).*";
			string ConvertedFilePath = Regex.Match(MatchFilePath, RegexStrippedPath).Value;
			ConvertedFilePath = Path.GetFullPath(/*"..\\..\\" +*/ ConvertedFilePath);

			// Extract Line + Column Number
			string ConvertedLineNumber = Regex.Match(MatchLineNumber, @"\d+").Value;
			string ConvertedColumnNumber = Regex.Match(MatchLineNumber, @"(?<=:\d+:)\d+").Value;

			// Write output
			string ConvertedExpression = "  " + ConvertedFilePath + "(" + ConvertedLineNumber + "," + ConvertedColumnNumber + "):" + MatchDescription;
			Log.TraceInformation(ConvertedExpression); // To create clickable vs link
			Log.TraceInformation(Output);				// To preserve readable output log
		}

		public override CPPOutput CompileCPPFiles(CppCompileEnvironment CompileEnvironment, List<FileItem> SourceFiles, string ModuleName, ActionGraph ActionGraph)
		{
			if (CompileEnvironment.Architecture == "-win32") // simulator
			{
				return base.CompileCPPFiles(CompileEnvironment, SourceFiles, ModuleName, ActionGraph);
			}

			string Arguments = GetCLArguments_Global(CompileEnvironment);

			CPPOutput Result = new CPPOutput();

			// Add include paths to the argument list.
			foreach (string IncludePath in CompileEnvironment.IncludePaths.UserIncludePaths)
			{
				Arguments += string.Format(" -I\"{0}\"", IncludePath);
			}
			foreach (string IncludePath in CompileEnvironment.IncludePaths.SystemIncludePaths)
			{
				Arguments += string.Format(" -I\"{0}\"", IncludePath);
			}


			// Add preprocessor definitions to the argument list.
			foreach (string Definition in CompileEnvironment.Definitions)
			{
				Arguments += string.Format(" -D{0}", Definition);
			}

			if (bEnableTracing)
			{
				Arguments += string.Format(" -D__EMSCRIPTEN_TRACING__");
			}

			foreach (FileItem SourceFile in SourceFiles)
			{
				Action CompileAction = ActionGraph.Add(ActionType.Compile);
				bool bIsPlainCFile = Path.GetExtension(SourceFile.AbsolutePath).ToUpperInvariant() == ".C";

				// Add the C++ source file and its included files to the prerequisite item list.
				AddPrerequisiteSourceFile(CompileEnvironment, SourceFile, CompileAction.PrerequisiteItems);

				// Add the source file path to the command-line.
				string FileArguments = string.Format(" \"{0}\"", SourceFile.AbsolutePath);
				var ObjectFileExtension = UEBuildPlatform.GetBuildPlatform(UnrealTargetPlatform.HTML5).GetBinaryExtension(UEBuildBinaryType.Object);
				// Add the object file to the produced item list.
				FileItem ObjectFile = FileItem.GetItemByFileReference(
					FileReference.Combine(
						CompileEnvironment.OutputDirectory,
						Path.GetFileName(SourceFile.AbsolutePath) + ObjectFileExtension
						)
					);
				CompileAction.ProducedItems.Add(ObjectFile);
				FileArguments += string.Format(" -o \"{0}\"", ObjectFile.AbsolutePath);

				// Add C or C++ specific compiler arguments.
				if (bIsPlainCFile)
				{
					FileArguments += GetCLArguments_C(CompileEnvironment.Architecture);
				}
				else
				{
					FileArguments += GetCLArguments_CPP(CompileEnvironment);
				}

				CompileAction.WorkingDirectory = UnrealBuildTool.EngineSourceDirectory.FullName;
				CompileAction.CommandPath = HTML5SDKInfo.Python();

				CompileAction.CommandArguments = HTML5SDKInfo.EmscriptenCompiler() + " " + Arguments + FileArguments + CompileEnvironment.AdditionalArguments;

				//System.Console.WriteLine(CompileAction.CommandArguments);
				CompileAction.StatusDescription = Path.GetFileName(SourceFile.AbsolutePath);
				CompileAction.OutputEventHandler = new DataReceivedEventHandler(CompileOutputReceivedDataEventHandler);

				// Don't farm out creation of precomputed headers as it is the critical path task.
				CompileAction.bCanExecuteRemotely = CompileEnvironment.PrecompiledHeaderAction != PrecompiledHeaderAction.Create;

				// this is the final output of the compile step (a .abc file)
				Result.ObjectFiles.Add(ObjectFile);

				// VC++ always outputs the source file name being compiled, so we don't need to emit this ourselves
				CompileAction.bShouldOutputStatusDescription = true;

				// Don't farm out creation of precompiled headers as it is the critical path task.
				CompileAction.bCanExecuteRemotely =
					CompileEnvironment.PrecompiledHeaderAction != PrecompiledHeaderAction.Create ||
					CompileEnvironment.bAllowRemotelyCompiledPCHs;
			}

			return Result;
		}

		public override CPPOutput CompileRCFiles(CppCompileEnvironment CompileEnvironment, List<FileItem> RCFiles, ActionGraph ActionGraph)
		{
			CPPOutput Result = new CPPOutput();

			if (CompileEnvironment.Architecture == "-win32") // simulator
			{
				return base.CompileRCFiles(CompileEnvironment, RCFiles, ActionGraph);
			}

			return Result;
		}

		/// <summary>
		/// Translates clang output warning/error messages into vs-clickable messages
		/// </summary>
		/// <param name="sender"> Sending object</param>
		/// <param name="e"> Event arguments (In this case, the line of string output)</param>
		protected void RemoteOutputReceivedEventHandler(object sender, DataReceivedEventArgs e)
		{
			var Output = e.Data;
			if (Output == null)
			{
				return;
			}

			if (Utils.IsRunningOnMono)
			{
				Log.TraceInformation(Output);
			}
			else
			{
				// Need to match following for clickable links
				string RegexFilePath = @"^(\/[A-Za-z0-9_\-\.]*)+\.(cpp|c|mm|m|hpp|h)";
				string RegexLineNumber = @"\:\d+\:\d+\:";
				string RegexDescription = @"(\serror:\s|\swarning:\s).*";

				// Get Matches
				string MatchFilePath = Regex.Match(Output, RegexFilePath).Value.Replace("Engine/Source/../../", "");
				string MatchLineNumber = Regex.Match(Output, RegexLineNumber).Value;
				string MatchDescription = Regex.Match(Output, RegexDescription).Value;

				// If any of the above matches failed, do nothing
				if (MatchFilePath.Length == 0 ||
					MatchLineNumber.Length == 0 ||
					MatchDescription.Length == 0)
				{
					Log.TraceInformation(Output);
					return;
				}

				// Convert Path
				string RegexStrippedPath = @"\/Engine\/.*"; //@"(Engine\/|[A-Za-z0-9_\-\.]*\/).*";
				string ConvertedFilePath = Regex.Match(MatchFilePath, RegexStrippedPath).Value;
				ConvertedFilePath = Path.GetFullPath("..\\.." + ConvertedFilePath);

				// Extract Line + Column Number
				string ConvertedLineNumber = Regex.Match(MatchLineNumber, @"\d+").Value;
				string ConvertedColumnNumber = Regex.Match(MatchLineNumber, @"(?<=:\d+:)\d+").Value;

				// Write output
				string ConvertedExpression = "  " + ConvertedFilePath + "(" + ConvertedLineNumber + "," + ConvertedColumnNumber + "):" + MatchDescription;
				Log.TraceInformation(ConvertedExpression); // To create clickable vs link
				//			Log.TraceInformation(Output);				// To preserve readable output log
			}
		}

		public override FileItem LinkFiles(LinkEnvironment LinkEnvironment, bool bBuildImportLibraryOnly, ActionGraph ActionGraph)
		{
			if (LinkEnvironment.Architecture == "-win32") // simulator
			{
				return base.LinkFiles(LinkEnvironment, bBuildImportLibraryOnly, ActionGraph);
			}

			FileItem OutputFile;

			// Make the final javascript file
			Action LinkAction = ActionGraph.Add(ActionType.Link);

			// ResponseFile lines.
			List<string> ReponseLines = new List<string>();

			LinkAction.bCanExecuteRemotely = false;
			LinkAction.WorkingDirectory = UnrealBuildTool.EngineSourceDirectory.FullName;
			LinkAction.CommandPath = HTML5SDKInfo.Python();
			LinkAction.CommandArguments = HTML5SDKInfo.EmscriptenCompiler();
			ReponseLines.Add(GetLinkArguments(LinkEnvironment));

			// Add the input files to a response file, and pass the response file on the command-line.
			foreach (FileItem InputFile in LinkEnvironment.InputFiles)
			{
				//System.Console.WriteLine("File  {0} ", InputFile.AbsolutePath);
				ReponseLines.Add(string.Format(" \"{0}\"", InputFile.AbsolutePath));
				LinkAction.PrerequisiteItems.Add(InputFile);
			}

			if (!LinkEnvironment.bIsBuildingLibrary)
			{
				// Make sure ThirdParty libs are at the end.
				List<string> ThirdParty = (from Lib in LinkEnvironment.AdditionalLibraries
											where Lib.Contains("ThirdParty")
											select Lib).ToList();

				LinkEnvironment.AdditionalLibraries.RemoveAll(Element => Element.Contains("ThirdParty"));
				LinkEnvironment.AdditionalLibraries.AddRange(ThirdParty);

				foreach (string InputFile in LinkEnvironment.AdditionalLibraries)
				{
					FileItem Item = FileItem.GetItemByPath(InputFile);

					if (Item.AbsolutePath.Contains(".lib"))
						continue;

					if (Item.ToString().Contains(".js"))
						ReponseLines.Add(string.Format(" --js-library \"{0}\"", Item.AbsolutePath));
					else
						ReponseLines.Add(string.Format(" \"{0}\"", Item.AbsolutePath));
					LinkAction.PrerequisiteItems.Add(Item);
				}
			}
			// make the file we will create


			OutputFile = FileItem.GetItemByFileReference(LinkEnvironment.OutputFilePath);
			LinkAction.ProducedItems.Add(OutputFile);
			ReponseLines.Add(string.Format(" -o \"{0}\"", OutputFile.AbsolutePath));

			FileItem OutputBC = FileItem.GetItemByPath(LinkEnvironment.OutputFilePath.FullName.Replace(".js", ".bc").Replace(".html", ".bc"));
			LinkAction.ProducedItems.Add(OutputBC);
			ReponseLines.Add(" --emit-symbol-map " + string.Format(" --save-bc \"{0}\"", OutputBC.AbsolutePath));

			LinkAction.StatusDescription = Path.GetFileName(OutputFile.AbsolutePath);

			FileReference ResponseFileName = GetResponseFileName(LinkEnvironment, OutputFile);


			LinkAction.CommandArguments += string.Format(" @\"{0}\"", ResponseFile.Create(ResponseFileName, ReponseLines));

			LinkAction.OutputEventHandler = new DataReceivedEventHandler(RemoteOutputReceivedEventHandler);

			return OutputFile;
		}

		public override void CompileCSharpProject(CSharpEnvironment CompileEnvironment, FileReference ProjectFileName, FileReference DestinationFile, ActionGraph ActionGraph)
		{
			throw new BuildException("HTML5 cannot compile C# files");
		}

		public override void ModifyBuildProducts(ReadOnlyTargetRules Target, UEBuildBinary Binary, List<string> Libraries, List<UEBuildBundleResource> BundleResources, Dictionary<FileReference, BuildProductType> BuildProducts)
		{
			// we need to include the generated .mem and .symbols file.
			if (Binary.Config.Type != UEBuildBinaryType.StaticLibrary)
			{
				BuildProducts.Add(Binary.Config.OutputFilePath + ".mem", BuildProductType.RequiredResource);
				BuildProducts.Add(Binary.Config.OutputFilePath + ".symbols", BuildProductType.RequiredResource);
			}
		}
	};
}
