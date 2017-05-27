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
		// ini configurations
		static bool targetingWasm = false;
		static bool targetWebGL2 = true; // Currently if this is set to true, UE4 can still fall back to WebGL 1 at runtime if browser does not support WebGL 2.
		static bool enableSIMD = false;
		static bool enableMultithreading = false;
		static bool bEnableTracing = false; // Debug option

		public HTML5ToolChain(FileReference InProjectFile)
			: base(CppPlatform.HTML5, WindowsCompiler.VisualStudio2015, false)
		{
			if (!HTML5SDKInfo.IsSDKInstalled())
			{
				throw new BuildException("HTML5 SDK is not installed; cannot use toolchain.");
			}

			// ini configs
			// - normal ConfigCache w/ UnrealBuildTool.ProjectFile takes all game config ini files
			//   (including project config ini files)
			// - but, during packaging, if -remoteini is used -- need to use UnrealBuildTool.GetRemoteIniPath()
			//   (note: ConfigCache can take null ProjectFile)
			string EngineIniPath = UnrealBuildTool.GetRemoteIniPath();
			DirectoryReference ProjectDir = !String.IsNullOrEmpty(EngineIniPath) ? new DirectoryReference(EngineIniPath)
												: DirectoryReference.FromFile(InProjectFile);
			ConfigHierarchy Ini = ConfigCache.ReadHierarchy(ConfigHierarchyType.Engine, ProjectDir, UnrealTargetPlatform.HTML5);

			// these will be going away...
			bool targetingAsmjs = true; // inverted check
			bool targetWebGL1 = false; // inverted check
			if ( Ini.GetBool("/Script/HTML5PlatformEditor.HTML5TargetSettings", "TargetAsmjs", out targetingAsmjs) )
			{
				targetingWasm = !targetingAsmjs;
			}
			if ( Ini.GetBool("/Script/HTML5PlatformEditor.HTML5TargetSettings", "TargetWebGL1", out targetWebGL1) )
			{
				targetWebGL2  = !targetWebGL1;
			}
			Ini.GetBool("/Script/HTML5PlatformEditor.HTML5TargetSettings", "EnableSIMD", out enableSIMD);
			Ini.GetBool("/Script/HTML5PlatformEditor.HTML5TargetSettings", "EnableMultithreading", out enableMultithreading);
			Ini.GetBool("/Script/HTML5PlatformEditor.HTML5TargetSettings", "EnableTracing", out bEnableTracing);
			Log.TraceInformation("HTML5ToolChain: TargetWasm = "         + targetingWasm        );
			Log.TraceInformation("HTML5ToolChain: TargetWebGL2 = "       + targetWebGL2         );
			Log.TraceInformation("HTML5ToolChain: EnableSIMD = "         + enableSIMD           );
			Log.TraceInformation("HTML5ToolChain: EnableMultithreading " + enableMultithreading );
			Log.TraceInformation("HTML5ToolChain: EnableTracing = "      + bEnableTracing       );

			// TODO: remove this "fix" when emscripten supports (SIMD & pthreads) + WASM
			if ( targetingWasm )
			{
				enableSIMD = false;

				// TODO: double check Engine/Source/Runtime/Core/Private/HTML5/HTML5PlatformProcess.cpp::SupportsMultithreading()
				enableMultithreading = false;
			}
		}

		public static void PreBuildSync()
		{
			Log.TraceInformation("Setting Emscripten SDK: located in " + HTML5SDKInfo.EMSCRIPTEN_ROOT);
			HTML5SDKInfo.SetupEmscriptenTemp();
			HTML5SDKInfo.SetUpEmscriptenConfigFile();

			if (Environment.GetEnvironmentVariable("EMSDK") == null) // If env. var EMSDK is present, Emscripten is already configured by the developer
			{
				// If not using preset emsdk, configure our generated .emscripten config, instead of autogenerating one in the user's home directory.
				Environment.SetEnvironmentVariable("EM_CONFIG", HTML5SDKInfo.DOT_EMSCRIPTEN);
				Environment.SetEnvironmentVariable("EM_CACHE", HTML5SDKInfo.EMSCRIPTEN_CACHE);
			}
		}

		string GetSharedArguments_Global(CppConfiguration Configuration, bool bOptimizeForSize, string Architecture, bool bEnableShadowVariableWarnings, bool bShadowVariableWarningsAsErrors, bool bEnableUndefinedIdentifierWarnings, bool bUndefinedIdentifierWarningsAsErrors)
		{
			string Result = " ";

			if (Architecture == "-win32") // simulator
			{
				return Result;
			}

			Result += " -fno-exceptions";

			Result += " -Wdelete-non-virtual-dtor";
			Result += " -Wno-unused-value"; // appErrorf triggers this
			Result += " -Wno-switch"; // many unhandled cases
			Result += " -Wno-tautological-constant-out-of-range-compare"; // disables some warnings about comparisons from TCHAR being a char
			// this hides the "warning : comparison of unsigned expression < 0 is always false" type warnings due to constant comparisons, which are possible with template arguments
			Result += " -Wno-tautological-compare";
			Result += " -Wno-inconsistent-missing-override"; // as of 1.35.0, overriding a member function but not marked as 'override' triggers warnings
			Result += " -Wno-expansion-to-defined"; // 1.36.11
			Result += " -Wno-undefined-var-template"; // 1.36.11
			Result += " -Wno-nonportable-include-path"; // 1.36.11

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

			// --------------------------------------------------------------------------------

			if (Configuration == CppConfiguration.Debug)
			{
				Result += " -O0";
			}
			else if (bOptimizeForSize)
			{
				Result += " -Oz";
			}
			else if (Configuration == CppConfiguration.Development)
			{
				Result += " -O2";
			}
			else if (Configuration == CppConfiguration.Shipping)
			{
				Result += " -O3";
			}

			// --------------------------------------------------------------------------------

			// JavaScript option overrides (see src/settings.js)
			if (enableSIMD)
			{
				Result += " -msse2 -s SIMD=1";
			}

			if (enableMultithreading)
			{
				Result += " -s USE_PTHREADS=1";
			}

			// --------------------------------------------------------------------------------
			// normally, these option are for linking -- but it using here to force recompile when
			if (targetingWasm) // flipping between asmjs and wasm
			{
				Result += " -s BINARYEN=1";
			}
			else
			{
				Result += " -s BINARYEN=0";
			}
			if (targetWebGL2) // flipping between webgl1 and webgl2
			{
				Result += " -s USE_WEBGL2=1";
			}
			else
			{
				Result += " -s USE_WEBGL2=0";
			}
			// --------------------------------------------------------------------------------

			// Expect that Emscripten SDK has been properly set up ahead in time (with emsdk and prebundled toolchains this is always the case)
			// This speeds up builds a tiny bit.
			Environment.SetEnvironmentVariable("EMCC_SKIP_SANITY_CHECK", "1");

			// THESE ARE TEST/DEBUGGING
//			Environment.SetEnvironmentVariable("EMCC_DEBUG", "1");
//			Environment.SetEnvironmentVariable("EMCC_CORES", "8");
//			Environment.SetEnvironmentVariable("EMCC_OPTIMIZE_NORMALLY", "1");

			// Linux builds needs this - or else system clang will be attempted to be picked up instead of UE4's
			// TODO: test on other platforms to remove this if() check
			if (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Linux)
			{
				Environment.SetEnvironmentVariable(HTML5SDKInfo.PLATFORM_USER_HOME, HTML5SDKInfo.HTML5Intermediatory);
			}
			return Result;
		}

		string GetCLArguments_Global(CppCompileEnvironment CompileEnvironment)
		{
			string Result = GetSharedArguments_Global(CompileEnvironment.Configuration, CompileEnvironment.bOptimizeForSize, CompileEnvironment.Architecture, CompileEnvironment.bEnableShadowVariableWarnings, CompileEnvironment.bShadowVariableWarningsAsErrors, CompileEnvironment.bEnableUndefinedIdentifierWarnings, CompileEnvironment.bUndefinedIdentifierWarningsAsErrors);

			if (CompileEnvironment.Architecture != "-win32")  // ! simulator
			{
				Result += " -Wno-reorder"; // we disable constructor order warnings.
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
			string Result = GetSharedArguments_Global(LinkEnvironment.Configuration, LinkEnvironment.bOptimizeForSize, LinkEnvironment.Architecture, false, false, false, false);

			/* N.B. When editing link flags in this function, UnrealBuildTool does not seem to automatically pick them up and do an incremental
			 *	relink only of UE4Game.js (at least when building blueprints projects). Therefore after editing, delete the old build
			 *	outputs to force UE4 to relink:
			 *
			 *    > rm Engine/Binaries/HTML5/UE4Game.js*
			 */

			if (LinkEnvironment.Architecture != "-win32") // ! simulator
			{
				// suppress link time warnings
				Result += " -Wno-ignored-attributes"; // function alias that always gets resolved
				Result += " -Wno-parentheses"; // precedence order
				Result += " -Wno-shift-count-overflow"; // 64bit is more than enough for shift 32

				// enable verbose mode
				Result += " -v";

				// do we want debug info?
				if (LinkEnvironment.Configuration == CppConfiguration.Debug || LinkEnvironment.bCreateDebugInfo)
				{
					// TODO: Would like to have -g2 enabled here, but the UE4 manifest currently requires that UE4Game.js.symbols
					// is always generated to the build, but that file is redundant if -g2 is passed (i.e. --emit-symbol-map gets ignored)
					// so in order to enable -g2 builds, the UE4 packager should be made aware that .symbols file might not always exist.
//					Result += " -g2";

					// As a lightweight alternative, just retain function names in output.
					Result += " --profiling-funcs";

					// dump headers: http://stackoverflow.com/questions/42308/tool-to-track-include-dependencies
//					Result += " -H";
				}
				else if (LinkEnvironment.Configuration == CppConfiguration.Development)
				{
					// Development builds always have their function names intact.
					Result += " --profiling-funcs";
				}

				// Emit a .symbols map file of the minified function names. (on -g2 builds this has no effect)
				Result += " --emit-symbol-map";

				if (LinkEnvironment.Configuration != CppConfiguration.Debug)
				{
					if (LinkEnvironment.bOptimizeForSize) Result += " -s OUTLINING_LIMIT=40000";
					else Result += " -s OUTLINING_LIMIT=110000";
				}

				if (LinkEnvironment.Configuration == CppConfiguration.Debug || LinkEnvironment.Configuration == CppConfiguration.Development)
				{
					// check for alignment/etc checking
//					Result += " -s SAFE_HEAP=1";
					//Result += " -s CHECK_HEAP_ALIGN=1";
					//Result += " -s SAFE_DYNCALLS=1";

					// enable assertions in non-Shipping/Release builds
					Result += " -s ASSERTIONS=1";
					Result += " -s GL_ASSERTIONS=1";

					// In non-shipping builds, don't run ctol evaller, it can take a bit of extra time.
					Result += " -s EVAL_CTORS=0";
				}

				if (targetingWasm)
				{
					Result += " -s BINARYEN=1 -s ALLOW_MEMORY_GROWTH=1";
//					Result += " -s BINARYEN_METHOD=\\'native-wasm\\'";
//					Result += " -s BINARYEN_MEM_MAX=-1";
				}
				else
				{
					// Memory init file is an asm.js only needed construct, in wasm the global data section is embedded in the wasm module,
					// so this flag is not needed there.
					Result += " --memory-init-file 1";

					// Separate the asm.js code to its own file so that browsers can optimize memory usage for the script files for debugging.
					Result += " -Wno-separate-asm";
					Result += " --separate-asm";
				}

				// we have to specify the full amount of memory with Asm.JS.
				// For Wasm, this is only the initial size, and the size can freely grow after that.
//				Result += " -s TOTAL_MEMORY=256*1024*1024";

				// no need for exceptions
				Result += " -s DISABLE_EXCEPTION_CATCHING=1";

				if (targetWebGL2)
				{
					// Enable targeting WebGL 2 when available.
					Result += " -s USE_WEBGL2=1";

					// Also enable WebGL 1 emulation in WebGL 2 contexts. This adds backwards compatibility related features to WebGL 2,
					// such as:
					//  - keep supporting GL_EXT_shader_texture_lod extension in GLSLES 1.00 shaders
					//  - support for WebGL1 unsized internal texture formats
					//  - mask the GL_HALF_FLOAT_OES != GL_HALF_FLOAT mixup
					Result += " -s WEBGL2_BACKWARDS_COMPATIBILITY_EMULATION=1";
//					Result += " -s FULL_ES3=1";
				}
//				else
//				{
//					Result += " -s FULL_ES2=1";
//				}

				// The HTML page template precreates the WebGL context, so instruct the runtime to hook into that if available.
				Result += " -s GL_PREINITIALIZED_CONTEXT=1";

				// export console command handler. Export main func too because default exports ( e.g Main ) are overridden if we use custom exported functions.
				Result += " -s EXPORTED_FUNCTIONS=\"['_main', '_on_fatal']\"";

				Result += " -s NO_EXIT_RUNTIME=1";

				Result += " -s ERROR_ON_UNDEFINED_SYMBOLS=1";

				if (bEnableTracing)
				{
					Result += " --tracing";
				}

				Result += " -s CASE_INSENSITIVE_FS=1";
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
				Log.TraceInformation(ConvertedExpression);	// To create clickable vs link
//				Log.TraceInformation(Output);				// To preserve readable output log
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

					if (Item.ToString().EndsWith(".js"))
						ReponseLines.Add(string.Format(" --js-library \"{0}\"", Item.AbsolutePath));


					// WARNING: With --pre-js and --post-js, the order in which these directives are passed to
					// the compiler is very critical, because that dictates the order in which they are appended.
					//
					// Set environment variable [ EMCC_DEBUG=1 ] to see the linker order used in packaging.
					//     See GetSharedArguments_Global() above to set this environment variable

					else if (Item.ToString().EndsWith(".jspre"))
						ReponseLines.Add(string.Format(" --pre-js \"{0}\"", Item.AbsolutePath));

					else if (Item.ToString().EndsWith(".jspost"))
						ReponseLines.Add(string.Format(" --post-js \"{0}\"", Item.AbsolutePath));


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
			ReponseLines.Add(string.Format(" --save-bc \"{0}\"", OutputBC.AbsolutePath));

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
				if (targetingWasm)
				{
					BuildProducts.Add(Binary.Config.OutputFilePath.ChangeExtension("wasm"), BuildProductType.RequiredResource);
				}
				else
				{
					BuildProducts.Add(Binary.Config.OutputFilePath + ".mem", BuildProductType.RequiredResource);
					BuildProducts.Add(Binary.Config.OutputFilePath.ChangeExtension("asm.js"), BuildProductType.RequiredResource);
					// TODO: add "_asm.js"
				}
				BuildProducts.Add(Binary.Config.OutputFilePath + ".symbols", BuildProductType.RequiredResource);
			}
		}
	};
}
