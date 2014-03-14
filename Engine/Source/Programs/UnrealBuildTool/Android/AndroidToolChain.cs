// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace UnrealBuildTool
{
	class AndroidToolChain : UEToolChain
	{
		static private bool bHasNDKExtensionsCompiled = false;

		static public string GetNdkApiLevel()
		{
			// default to looking on disk for latest API level
			string Target = Utils.GetStringEnvironmentVariable("ue.AndroidNdkApiTarget", "latest");

			if (Target == "latest")
			{
				// get a list of NDK platforms
				string PlatformsDir = Environment.ExpandEnvironmentVariables("%NDKROOT%\\platforms");
				if (!Directory.Exists(PlatformsDir))
				{
					throw new BuildException("No platforms found in {0}", PlatformsDir);
				}

				// return the largest of them
				Target = GetLargestApiLevel(Directory.GetDirectories(PlatformsDir));
			}

			return Target;
		}

		static public string GetLargestApiLevel(string[] ApiLevels)
		{
			int LargestLevel = 0;
			string LargestString = null;
			
			// look for largest integer
			foreach (string Level in ApiLevels)
			{
				string LocalLevel = Path.GetFileName(Level);
				string[] Tokens = LocalLevel.Split("-".ToCharArray());
				if (Tokens.Length >= 2)
				{
					int ParsedLevel = int.Parse(Tokens[1]);
					// bigger? remember it
					if (ParsedLevel > LargestLevel)
					{
						LargestLevel = ParsedLevel;
						LargestString = LocalLevel;
					}
				}
			}

			return LargestString;
		}

		public override void RegisterToolChain()
		{
			string NDKPath = Environment.GetEnvironmentVariable("NDKROOT");

			// don't register if we don't have an NDKROOT specified
			if (String.IsNullOrEmpty(NDKPath))
			{
				return;
			}

			NDKPath = NDKPath.Replace("\"", "");

			string ClangVersion = "";
			string GccVersion = "";

			// prefer clang 3.3, but fall back to 3.1 if needed for now
			if (Directory.Exists(Path.Combine(NDKPath, @"toolchains\llvm-3.3")))
			{
				ClangVersion = "3.3";
				GccVersion = "4.8";
			}
			else if (Directory.Exists(Path.Combine(NDKPath, @"toolchains\llvm-3.1")))
			{
				ClangVersion = "3.1";
				GccVersion = "4.6";
			}
			else
			{
				return;
			}

            string ArchitecturePath = "";
            string ArchitecturePathWindows32 = @"prebuilt\windows";
            string ArchitecturePathWindows64 = @"prebuilt\windows-x86_64";

            if (Directory.Exists(Path.Combine(NDKPath, ArchitecturePathWindows64)))
            {
                Log.TraceVerbose("        Found Windows 64 bit versions of toolchain");
                ArchitecturePath = ArchitecturePathWindows64;
            }
            else if (Directory.Exists(Path.Combine(NDKPath, ArchitecturePathWindows32)))
            {
                Log.TraceVerbose("        Found Windows 32 bit versions of toolchain");
                ArchitecturePath = ArchitecturePathWindows32;
            }
            else
            {
                Log.TraceVerbose("        Did not find 32 bit or 64 bit versions of toolchain");
                return;
            }

			// set up the path to our toolchains
			ClangPath = Path.Combine(NDKPath, @"toolchains\llvm-" + ClangVersion, ArchitecturePath, @"bin\clang++.exe");
            ArPathArm = Path.Combine(NDKPath, @"toolchains\arm-linux-androideabi-" + GccVersion, ArchitecturePath, @"bin\arm-linux-androideabi-ar.exe");		//@todo android: use llvm-ar.exe instead?
            ArPathx86 = Path.Combine(NDKPath, @"toolchains\x86-" + GccVersion, ArchitecturePath, @"bin\i686-linux-android-ar.exe");							//@todo android: verify x86 toolchain


			// toolchain params
			ToolchainParamsArm = "-target armv7-none-linux-androideabi" +
								   " --sysroot=\"" + Path.Combine(NDKPath, "platforms", GetNdkApiLevel(), "arch-arm") + "\"" + 
                                   " -gcc-toolchain \"" + Path.Combine(NDKPath, @"toolchains\arm-linux-androideabi-" + GccVersion, ArchitecturePath) + "\"";
			ToolchainParamsx86 = "-target i686-none-linux-android" +
								   " --sysroot=\"" + Path.Combine(NDKPath, "platforms", GetNdkApiLevel(), "arch-x86") + "\"" +
                                   " -gcc-toolchain \"" + Path.Combine(NDKPath, @"toolchains\x86-" + GccVersion, ArchitecturePath) + "\"";

			// Register this tool chain
			Log.TraceVerbose("        Registered for {0}", CPPTargetPlatform.Android.ToString());
			UEToolChain.RegisterPlatformToolChain(CPPTargetPlatform.Android, this);
		}

		static string GetCLArguments_Global(CPPEnvironment CompileEnvironment)
		{
			string Result = "";
			
			Result += (CompileEnvironment.Config.TargetArchitecture == "-armv7") ? ToolchainParamsArm : ToolchainParamsx86;

			// build up the commandline common to C and C++
			Result += " -c";
			Result += " -fdiagnostics-format=msvc";
			Result += " -Wall";

			Result += " -Wno-unused-variable";
			// this will hide the warnings about static functions in headers that aren't used in every single .cpp file
			Result += " -Wno-unused-function";
			// this hides the "enumeration value 'XXXXX' not handled in switch [-Wswitch]" warnings - we should maybe remove this at some point and add UE_LOG(, Fatal, ) to default cases
			Result += " -Wno-switch";
			// this hides the "warning : comparison of unsigned expression < 0 is always false" type warnings due to constant comparisons, which are possible with template arguments
			Result += " -Wno-tautological-compare";
			//This will prevent the issue of warnings for unused private variables.
			Result += " -Wno-unused-private-field";
			Result += " -Wno-local-type-template-args";	// engine triggers this
			Result += " -Wno-return-type-c-linkage";	// needed for PhysX
			Result += " -Wno-reorder";					// member initialization order
			Result += " -Wno-unknown-pragmas";			// probably should kill this one, sign of another issue in PhysX?
			Result += " -Wno-invalid-offsetof";			// needed to suppress warnings about using offsetof on non-POD types.

			// shipping builds will cause this warning with "ensure", so disable only in those case
			if (CompileEnvironment.Config.TargetConfiguration == CPPTargetConfiguration.Shipping)
			{
				Result += " -Wno-unused-value";
			}

			// debug info
			if (CompileEnvironment.Config.bCreateDebugInfo)
			{
				Result += " -g2 -gdwarf-2";
			}

			// optimization level
			if (CompileEnvironment.Config.TargetConfiguration == CPPTargetConfiguration.Debug)
			{
				Result += " -O0";
			}
			else
			{
				Result += " -O3";
			}

			//@todo android: these are copied verbatim from UE3 and probably need adjustment
			if (CompileEnvironment.Config.TargetArchitecture == "-armv7")
			{
		//		Result += " -mthumb-interwork";			// Generates code which supports calling between ARM and Thumb instructions, w/o it you can't reliability use both together 
				Result += " -funwind-tables";			// Just generates any needed static data, affects no code 
				Result += " -fstack-protector";			// Emits extra code to check for buffer overflows
		//		Result += " -mlong-calls";				// Perform function calls by first loading the address of the function into a reg and then performing the subroutine call
				Result += " -fno-strict-aliasing";		// Prevents unwanted or invalid optimizations that could produce incorrect code
				Result += " -fpic";						// Generates position-independent code (PIC) suitable for use in a shared library
				Result += " -fno-exceptions";			// Do not enable exception handling, generates extra code needed to propagate exceptions
				Result += " -fno-rtti";					// 
				Result += " -fno-short-enums";			// Do not allocate to an enum type only as many bytes as it needs for the declared range of possible values
		//		Result += " -finline-limit=64";			// GCC limits the size of functions that can be inlined, this flag allows coarse control of this limit
		//		Result += " -Wno-psabi";				// Warn when G++ generates code that is probably not compatible with the vendor-neutral C++ ABI

				Result += " -march=armv7-a";
				Result += " -mfloat-abi=softfp";
				Result += " -mfpu=vfpv3-d16";			//@todo android: UE3 was just vfp. arm7a should all support v3 with 16 registers

				// Some switches interfere with on-device debugging
				if (CompileEnvironment.Config.TargetConfiguration != CPPTargetConfiguration.Debug)
				{
					Result += " -ffunction-sections";   // Places each function in its own section of the output file, linker may be able to perform opts to improve locality of reference
				}

				Result += " -fsigned-char";				// Treat chars as signed //@todo android: any concerns about ABI compatibility with libs here?
			}
			else if (CompileEnvironment.Config.TargetArchitecture == "-x86")
			{
				Result += " -fstrict-aliasing";
				Result += " -funswitch-loops";
				Result += " -finline-limit=128";
				Result += " -fno-omit-frame-pointer";
				Result += " -fno-strict-aliasing";
				Result += " -fno-short-enums";
				Result += " -fno-exceptions";
				Result += " -fno-rtti";
				Result += " -march=atom";
			}

			return Result;
		}

		static string GetCompileArguments_CPP(bool bDisableOptimizations)
		{
			string Result = "";

			Result += " -x c++";
			Result += " -std=c++11";

			// optimization level
			if (bDisableOptimizations)
			{
				Result += " -O0";
			}
			else
			{
				Result += " -O3";
			}

			return Result;
		}

		static string GetCompileArguments_C(bool bDisableOptimizations)
		{
			string Result = "";

			Result += " -x c";

			// optimization level
			if (bDisableOptimizations)
			{
				Result += " -O0";
			}
			else
			{
				Result += " -O3";
			}

			return Result;
		}

		static string GetCompileArguments_PCH(bool bDisableOptimizations)
		{
			string Result = "";

			Result += " -x c++-header";
			Result += " -std=c++11";
			
			// optimization level
			if (bDisableOptimizations)
			{
				Result += " -O0";
			}
			else
			{
				Result += " -O3";
			}

			return Result;
		}

		static string GetLinkArguments(LinkEnvironment LinkEnvironment)
		{
			string Result = "";
			
			Result += (LinkEnvironment.Config.TargetArchitecture == "-armv7") ? ToolchainParamsArm : ToolchainParamsx86;

			Result += " -nostdlib";
			Result += " -Wl,-shared,-Bsymbolic";
			Result += " -Wl,--no-undefined";

			if (LinkEnvironment.Config.TargetArchitecture == "-armv7")
			{
				Result += " -march=armv7-a";
				Result += " -Wl,--fix-cortex-a8";		// required to route around a CPU bug in some Cortex-A8 implementations
			}
			else if (LinkEnvironment.Config.TargetArchitecture == "-x86")
			{
				Result += " -march=atom";
			}

			return Result;
		}

		static string GetArArguments(LinkEnvironment LinkEnvironment)
		{
			string Result = "";

			Result += " -r";

			return Result;
		}

		public static void CompileOutputReceivedDataEventHandler(Object Sender, DataReceivedEventArgs Line)
		{
			if ((Line != null) && (Line.Data != null) && (Line.Data != ""))
			{

				bool bWasHandled = false;
				// does it look like an error? something like this:
				//     2>Core/Inc/UnStats.h:478:3: error : no matching constructor for initialization of 'FStatCommonData'

				try
				{
					if (!Line.Data.StartsWith(" ") && !Line.Data.StartsWith(","))
					{
						// if we split on colon, an error will have at least 4 tokens
						string[] Tokens = Line.Data.Split("(".ToCharArray());
						if (Tokens.Length > 1)
						{
							// make sure what follows the parens is what we expdect
							string[] ErrorPos = Tokens[1].Split(")".ToCharArray());

							string Filename = Path.GetFullPath(Tokens[0]);
							// build up the final string
							string Output = string.Format("{0}({1}", Filename, Tokens[1], Line.Data[0]);
							for (int T = 3; T < Tokens.Length; T++)
							{
								Output += Tokens[T];
								if (T < Tokens.Length - 1)
								{
									Output += "(";
								}
							}

							// output the result
							Log.TraceInformation(Output);
							bWasHandled = true;
						}
					}
				}
				catch (Exception)
				{
					bWasHandled = false;
				}

				// write if not properly handled
				if (!bWasHandled)
				{
					Log.TraceWarning("{0}", Line.Data);
				}
			}
		}

		static void ConditionallyAddNDKSourceFiles(List<FileItem> SourceFiles)
		{
			if (!bHasNDKExtensionsCompiled)
			{
				SourceFiles.Add(FileItem.GetItemByPath(Environment.GetEnvironmentVariable("NDKROOT") + "/sources/android/native_app_glue/android_native_app_glue.c"));
				SourceFiles.Add(FileItem.GetItemByPath(Environment.GetEnvironmentVariable("NDKROOT") + "/sources/android/cpufeatures/cpu-features.c"));
			}
			bHasNDKExtensionsCompiled = true;
		}

		// cache the location of NDK tools
		static string ClangPath;
		static string ToolchainParamsArm;
		static string ToolchainParamsx86;
		static string ArPathArm;
		static string ArPathx86;

		static private bool bHasPrintedApiLevel = false;
		public override CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, List<FileItem> SourceFiles, string ModuleName)
		{
			if (!bHasPrintedApiLevel)
			{
				Console.WriteLine("Compiling with NDK API '{0}'", GetNdkApiLevel());
				bHasPrintedApiLevel = true;
			}
	
			string Arguments = GetCLArguments_Global(CompileEnvironment);
			string PCHArguments = "";

			if (CompileEnvironment.Config.PrecompiledHeaderAction != PrecompiledHeaderAction.Create)
			{
				Arguments += " -Werror";

			}
			if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Include)
			{
				// Add the precompiled header file's path to the include path so Clang can find it.
				// This needs to be before the other include paths to ensure Clang uses it instead of the source header file.
				PCHArguments += string.Format(" -include \"{0}\"", CompileEnvironment.PrecompiledHeaderFile.AbsolutePath.Replace(".gch", ""));
			}

			// Add include paths to the argument list.
			foreach (string IncludePath in CompileEnvironment.Config.SystemIncludePaths)
			{
				Arguments += string.Format(" -I\"{0}\"", IncludePath);
			}
			foreach (string IncludePath in CompileEnvironment.Config.IncludePaths)
			{
				// we make this absolute because there are some edge cases when a code-based rocket project on the same dtive
				// as the engine will make relative paths that make clang fail to compile. Absolute will succeeed.
				Arguments += string.Format(" -I\"{0}\"", Path.GetFullPath(IncludePath));
			}

			// Directly added NDK files for NDK extensions
			if (!UnrealBuildTool.RunningRocket())
			{
				ConditionallyAddNDKSourceFiles(SourceFiles);
			}

			// Add preprocessor definitions to the argument list.
			foreach (string Definition in CompileEnvironment.Config.Definitions)
			{
				Arguments += string.Format(" -D \"{0}\"", Definition);
			}

			// Create a compile action for each source file.
			CPPOutput Result = new CPPOutput();
			foreach (FileItem SourceFile in SourceFiles)
			{
				Action CompileAction = new Action(ActionType.Compile);
				string FileArguments = "";
				bool bIsPlainCFile = Path.GetExtension(SourceFile.AbsolutePath).ToUpperInvariant() == ".C";

				// should we disable optimizations on this file?
				// @todo android - We wouldn't need this if we could disable optimizations per function (via pragma)
				bool bDisableOptimizations = false;// SourceFile.AbsolutePath.ToUpperInvariant().IndexOf("\\SLATE\\") != -1;
				if (bDisableOptimizations && CompileEnvironment.Config.TargetConfiguration != CPPTargetConfiguration.Debug)
				{
					Log.TraceWarning("Disabling optimizations on {0}", SourceFile.AbsolutePath);
				}

				bDisableOptimizations = bDisableOptimizations || CompileEnvironment.Config.TargetConfiguration == CPPTargetConfiguration.Debug;

				// Add C or C++ specific compiler arguments.
				if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Create)
				{
					FileArguments += GetCompileArguments_PCH(bDisableOptimizations);
				}
				else if (bIsPlainCFile)
				{
					FileArguments += GetCompileArguments_C(bDisableOptimizations);
				}
				else
				{
					FileArguments += GetCompileArguments_CPP(bDisableOptimizations);

					// only use PCH for .cpp files
					FileArguments += PCHArguments;
				}

				// Add the C++ source file and its included files to the prerequisite item list.
				CompileAction.PrerequisiteItems.Add(SourceFile);
				foreach (FileItem IncludedFile in CompileEnvironment.GetIncludeDependencies(SourceFile))
				{
					CompileAction.PrerequisiteItems.Add(IncludedFile);
				}

				if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Create)
				{
					// Add the precompiled header file to the produced item list.
					FileItem PrecompiledHeaderFile = FileItem.GetItemByPath(
						Path.Combine(
							CompileEnvironment.Config.OutputDirectory,
							Path.GetFileName(SourceFile.AbsolutePath) + ".gch"
							)
						);

					CompileAction.ProducedItems.Add(PrecompiledHeaderFile);
					Result.PrecompiledHeaderFile = PrecompiledHeaderFile;

					// Add the parameters needed to compile the precompiled header file to the command-line.
					FileArguments += string.Format(" -o \"{0}\"", PrecompiledHeaderFile.AbsolutePath, false);
				}
				else
				{
					if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Include)
					{
						CompileAction.bIsUsingPCH = true;
						CompileAction.PrerequisiteItems.Add(CompileEnvironment.PrecompiledHeaderFile);
					}

					// Add the object file to the produced item list.
					FileItem ObjectFile = FileItem.GetItemByPath(
						Path.Combine(
							CompileEnvironment.Config.OutputDirectory,
							Path.GetFileName(SourceFile.AbsolutePath) + ".o"
							)
						);
					CompileAction.ProducedItems.Add(ObjectFile);
					Result.ObjectFiles.Add(ObjectFile);

					FileArguments += string.Format(" -o \"{0}\"", ObjectFile.AbsolutePath, false);
				}

				// Add the source file path to the command-line.
				FileArguments += string.Format(" \"{0}\"", SourceFile.AbsolutePath);

				// Build a full argument list
				string AllArguments = Arguments + FileArguments + CompileEnvironment.Config.AdditionalArguments;
				AllArguments = ActionThread.ExpandEnvironmentVariables(AllArguments);
				AllArguments = AllArguments.Replace("\\", "/");				

				// Create the response file
				string ResponseFileName = CompileAction.ProducedItems[0].AbsolutePath + ".response";
				string ResponseArgument = string.Format("@\"{0}\"", ResponseFile.Create(ResponseFileName, new List<string>{ AllArguments } ));

				CompileAction.WorkingDirectory = Path.GetFullPath(".");
				CompileAction.CommandPath = ClangPath;
				CompileAction.CommandArguments = ResponseArgument;
				CompileAction.StatusDescription = string.Format("{0}", Path.GetFileName(SourceFile.AbsolutePath));
				CompileAction.StatusDetailedDescription = SourceFile.Description;

				CompileAction.OutputEventHandler = new DataReceivedEventHandler(CompileOutputReceivedDataEventHandler);

				// VC++ always outputs the source file name being compiled, so we don't need to emit this ourselves
				CompileAction.bShouldOutputStatusDescription = true;

				// Don't farm out creation of pre-compiled headers as it is the critical path task.
				CompileAction.bCanExecuteRemotely =
					CompileEnvironment.Config.PrecompiledHeaderAction != PrecompiledHeaderAction.Create ||
					BuildConfiguration.bAllowRemotelyCompiledPCHs;
			}

			return Result;
		}

		public override FileItem LinkFiles(LinkEnvironment LinkEnvironment, bool bBuildImportLibraryOnly)
		{
			// Create an action that invokes the linker.
			Action LinkAction = new Action(ActionType.Link);
			LinkAction.WorkingDirectory = Path.GetFullPath(".");

			if (LinkEnvironment.Config.bIsBuildingLibrary)
			{
				LinkAction.CommandPath = (LinkEnvironment.Config.TargetArchitecture == "-armv7") ? ArPathArm : ArPathx86;
			}
			else
			{
				LinkAction.CommandPath = ClangPath;
			}

			// Get link arguments.
			LinkAction.CommandArguments = LinkEnvironment.Config.bIsBuildingLibrary ? GetArArguments(LinkEnvironment) : GetLinkArguments(LinkEnvironment);

			// Add the output file as a production of the link action.
			FileItem OutputFile = FileItem.GetItemByPath(LinkEnvironment.Config.OutputFilePath);
			LinkAction.ProducedItems.Add(OutputFile);
			LinkAction.StatusDescription = string.Format("{0}", Path.GetFileName(OutputFile.AbsolutePath));

			// Add the output file to the command-line.
			if (LinkEnvironment.Config.bIsBuildingLibrary)
			{
				LinkAction.CommandArguments += string.Format(" \"{0}\"", OutputFile.AbsolutePath);
			}
			else
			{
				LinkAction.CommandArguments += string.Format(" -o \"{0}\"", OutputFile.AbsolutePath);
			}

			// Add the input files to a response file, and pass the response file on the command-line.
			List<string> InputFileNames = new List<string>();
			foreach (FileItem InputFile in LinkEnvironment.InputFiles)
			{
				InputFileNames.Add(string.Format("\"{0}\"", InputFile.AbsolutePath.Replace("\\", "/")));
				LinkAction.PrerequisiteItems.Add(InputFile);
			}

			string ResponseFileName = GetResponseFileName( LinkEnvironment, OutputFile );
			LinkAction.CommandArguments += string.Format(" @\"{0}\"", ResponseFile.Create(ResponseFileName, InputFileNames));

			// libs don't link in other libs
			if (!LinkEnvironment.Config.bIsBuildingLibrary)
			{
				// Add the library paths to the argument list.
				foreach (string LibraryPath in LinkEnvironment.Config.LibraryPaths)
				{
					LinkAction.CommandArguments += string.Format(" -L\"{0}\"", LibraryPath);
				}

				// add libraries in a library group
				LinkAction.CommandArguments += string.Format(" -Wl,--start-group");
				foreach (string AdditionalLibrary in LinkEnvironment.Config.AdditionalLibraries)
				{
					if (String.IsNullOrEmpty(Path.GetDirectoryName(AdditionalLibrary)))
					{
						LinkAction.CommandArguments += string.Format(" \"-l{0}\"", AdditionalLibrary);
					}
					else
					{
						// full pathed libs are compiled by us, so we depend on linking them
						LinkAction.CommandArguments += string.Format(" \"{0}\"", AdditionalLibrary);
						LinkAction.PrerequisiteItems.Add(FileItem.GetItemByPath(AdditionalLibrary));
					}
				}
				LinkAction.CommandArguments += string.Format(" -Wl,--end-group");
			}

			// Add the additional arguments specified by the environment.
			LinkAction.CommandArguments += LinkEnvironment.Config.AdditionalArguments;
			LinkAction.CommandArguments = LinkAction.CommandArguments.Replace("\\", "/");

			// Only execute linking on the local PC.
			LinkAction.bCanExecuteRemotely = false;

			return OutputFile;
		}

		public override void CompileCSharpProject(CSharpEnvironment CompileEnvironment, string ProjectFileName, string DestinationFile)
		{
			throw new BuildException("Android cannot compile C# files");
		}

		public static void OutputReceivedDataEventHandler(Object Sender, DataReceivedEventArgs Line)
		{
			if ((Line != null) && (Line.Data != null))
			{
				Log.TraceInformation(Line.Data);
			}
		}
	};
}
