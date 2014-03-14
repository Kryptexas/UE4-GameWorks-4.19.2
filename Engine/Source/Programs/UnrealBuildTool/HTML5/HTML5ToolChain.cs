// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

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
		// cache the location of SDK tools
		static string EMCCPath;
        static string EMLinkPath;
		static string PythonPath;

		static private bool bEnableFastIteration = UnrealBuildTool.CommandLineContains("-fastiteration");

		public override void RegisterToolChain()
		{
			// Make sure the SDK is installed
            // look up installed SDK. 
            string BaseSDKPath = ""; 
			if (!Utils.IsRunningOnMono)
			{
				Microsoft.Win32.RegistryKey localKey = Microsoft.Win32.RegistryKey.OpenBaseKey(Microsoft.Win32.RegistryHive.LocalMachine, Microsoft.Win32.RegistryView.Registry64); 
				Microsoft.Win32.RegistryKey key = localKey.OpenSubKey("Software\\Emscripten64"); 
	            if (key != null)
	            {
	                string sdkdir = key.GetValue("Install_Dir") as string;
	                // emscripten path is the highest numbered directory 
	                DirectoryInfo dInfo = new DirectoryInfo(sdkdir + "\\emscripten");
	                string Latest_Ver = (from S in dInfo.GetDirectories() select S.Name).ToList().Last();
	                BaseSDKPath = sdkdir + @"\emscripten\" + Latest_Ver;
	            }
	            else
	            {
	                BaseSDKPath = Environment.GetEnvironmentVariable("EMSCRIPTEN");    
	            }
			}
			else
			{
	            BaseSDKPath = Environment.GetEnvironmentVariable("EMSCRIPTEN");
			}

			if (!String.IsNullOrEmpty(BaseSDKPath))
			{
				BaseSDKPath = BaseSDKPath.Replace("\"", "");
				if (!String.IsNullOrEmpty(BaseSDKPath))
                {
					EMCCPath = Path.Combine(BaseSDKPath, "emcc");
                    EMLinkPath = Path.Combine(BaseSDKPath, "emlink.py");
					// also figure out where python lives (if no envvar, assume it's in the path)
					PythonPath = Environment.GetEnvironmentVariable("PYTHON");
					if (PythonPath == null)
					{
						PythonPath = Utils.IsRunningOnMono ? "python" : "python.exe";
					}
                    EMCCPath = "\"" + EMCCPath + "\""; 

					// set some environment variable we'll need

					//Environment.SetEnvironmentVariable("EMCC_DEBUG", "cache");
					Environment.SetEnvironmentVariable("EMCC_CORES", "8");
					Environment.SetEnvironmentVariable("EMCC_FORCE_STDLIBS", "1");
					Environment.SetEnvironmentVariable("EMCC_OPTIMIZE_NORMALLY", "1");

					// finally register the toolchain that is now ready to go
                    Log.TraceVerbose("        Registered for {0}", CPPTargetPlatform.HTML5.ToString());
					UEToolChain.RegisterPlatformToolChain(CPPTargetPlatform.HTML5, this);
				}
			}
		}

		static string GetSharedArguments_Global(CPPTargetConfiguration TargetConfiguration, string Architecture)
		{
			string Result = " ";

			if (Architecture == "-win32")
			{
                return Result;
            }

			// 			Result += " -funsigned-char";
			// 			Result += " -fno-strict-aliasing";
			Result += " -fno-exceptions";
			// 			Result += " -fno-short-enums";

			Result += " -Wno-unused-value"; // appErrorf triggers this
			Result += " -Wno-switch"; // many unhandled cases
			Result += " -Wno-tautological-constant-out-of-range-compare"; // disables some warnings about comparisons from TCHAR being a char
			// this hides the "warning : comparison of unsigned expression < 0 is always false" type warnings due to constant comparisons, which are possible with template arguments
			Result += " -Wno-tautological-compare";

			// okay, in UE4, we'd fix the code for these, but in UE3, not worth it
			Result += " -Wno-logical-op-parentheses"; // appErrorf triggers this
			Result += " -Wno-array-bounds"; // some VectorLoads go past the end of the array, but it's okay in that case
            Result += " -Wno-invalid-offsetof"; // too many warnings kills windows clang. 
            

			// JavsScript option overrides (see src/settings.js)

			// we have to specify the full amount of memory with Asm.JS (1.5 G)
            // I wonder if there's a per game way to change this. 
            Result += " -s TOTAL_MEMORY=1610612736";

			// no need for exceptions
			Result += " -s DISABLE_EXCEPTION_CATCHING=1";
			// enable checking for missing functions at link time as opposed to runtime
			Result += " -s WARN_ON_UNDEFINED_SYMBOLS=1";
			// we want full ES2
			Result += " -s FULL_ES2=1 ";
			// don't need UTF8 string support, and it slows string ops down
			Result += " -s UTF_STRING_SUPPORT=0";
            Result += "  ";

			if (TargetConfiguration == CPPTargetConfiguration.Debug)
			{
				Result += " -O0";
			}
			if (TargetConfiguration == CPPTargetConfiguration.Debug || TargetConfiguration == CPPTargetConfiguration.Development)
			{
				Result += " -s GL_ASSERTIONS=1 ";
			}
			if (TargetConfiguration == CPPTargetConfiguration.Development)
			{
                Result += " -O2 -s ASM_JS=1 -s OUTLINING_LIMIT=110000";
			}
			if (TargetConfiguration == CPPTargetConfiguration.Shipping)
			{
				Result += " -O2 -s ASM_JS=1 -s OUTLINING_LIMIT=110000";
			}

			// NOTE: This may slow down the compiler's startup time!
            if ( !bEnableFastIteration )
			{ 
                Result += " --memory-init-file 1";
			}

			return Result;
		}
		
		static string GetCLArguments_Global(CPPEnvironment CompileEnvironment)
		{
			string Result = GetSharedArguments_Global(CompileEnvironment.Config.TargetConfiguration, CompileEnvironment.Config.TargetArchitecture);

			if (CompileEnvironment.Config.TargetArchitecture != "-win32")
			{
				// do we want debug info?
				/*			if (CompileEnvironment.Config.bCreateDebugInfo)
							{
								 Result += " -g";
							}*/

				Result += " -Wno-warn-absolute-paths ";
			}

			return Result;
		}

		static string GetCLArguments_CPP(CPPEnvironment CompileEnvironment)
		{
			string Result = "";

			if (CompileEnvironment.Config.TargetArchitecture != "-win32")
			{
				Result = " -std=c++11";
			}

			return Result;
		}

		static string GetCLArguments_C(string Architecture)
		{
			string Result = "";
			return Result;
		}

		static string GetLinkArguments(LinkEnvironment LinkEnvironment)
		{
			string Result = GetSharedArguments_Global(LinkEnvironment.Config.TargetConfiguration, LinkEnvironment.Config.TargetArchitecture);

			if (LinkEnvironment.Config.TargetArchitecture != "-win32")
			{

				// enable verbose mode
				Result += " -v";

				if (LinkEnvironment.Config.TargetConfiguration != CPPTargetConfiguration.Shipping)
				{
					// enable caching of intermediate date
					Result += " --jcache -g";
				}

				if (LinkEnvironment.Config.TargetConfiguration == CPPTargetConfiguration.Debug)
				{
					// check for alignment/etc checking
					//Result += " -s SAFE_HEAP=1";
					//Result += " -s CHECK_HEAP_ALIGN=1";
					//Result += " -s SAFE_DYNCALLS=1";

					// enable assertions in non-Shipping/Release builds
					Result += " -s ASSERTIONS=1";
				}

				Result += " -s CASE_INSENSITIVE_FS=1 ";

				Result += " --js-library Runtime/Core/Public/HTML5/HTML5DebugLogging.js ";

				string BaseSDKPath = Environment.GetEnvironmentVariable("EMSCRIPTEN");
				Result += " --js-library \"" + BaseSDKPath + "/Src/library_openal.js\" ";
			}

			return Result;
		}

		static string GetLibArguments(LinkEnvironment LinkEnvironment)
		{
			string Result = "";

			if (LinkEnvironment.Config.TargetArchitecture == "-win32")
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
				if (LinkEnvironment.Config.TargetConfiguration == CPPTargetConfiguration.Shipping)
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

		public override CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, List<FileItem> SourceFiles, string ModuleName)
		{
			if (CompileEnvironment.Config.TargetArchitecture == "-win32")
			{
				return base.CompileCPPFiles(CompileEnvironment, SourceFiles, ModuleName);
			}

			string Arguments = GetCLArguments_Global(CompileEnvironment);
			string BaseSDKPath = Environment.GetEnvironmentVariable("EMSCRIPTEN");

			CPPOutput Result = new CPPOutput();

			// Add include paths to the argument list.
			foreach (string IncludePath in CompileEnvironment.Config.IncludePaths)
			{
				Arguments += string.Format(" -I\"{0}\"", IncludePath);
			}
			foreach (string IncludePath in CompileEnvironment.Config.SystemIncludePaths)
			{
				Arguments += string.Format(" -I\"{0}\"", IncludePath);
			}

            if ( ModuleName == "Launch" ) 
                Arguments += string.Format(" -I\"{0}\"", BaseSDKPath + "/system/lib/libcxxabi/include" );
        
			// Add preprocessor definitions to the argument list.
			foreach (string Definition in CompileEnvironment.Config.Definitions)
			{
				Arguments += string.Format(" -D{0}", Definition);
			}

			// Create a compile action for each source file.
            if (ModuleName == "Launch")
                SourceFiles.Add(FileItem.GetItemByPath(BaseSDKPath + "/system/lib/libcxxabi/src/cxa_demangle.cpp")); 
        
			foreach (FileItem SourceFile in SourceFiles)
			{
				Action CompileAction = new Action(ActionType.Compile);
				bool bIsPlainCFile = Path.GetExtension(SourceFile.AbsolutePath).ToUpperInvariant() == ".C";
                
				// Add the C++ source file and its included files to the prerequisite item list.
				CompileAction.PrerequisiteItems.Add(SourceFile);
				foreach (FileItem IncludedFile in CompileEnvironment.GetIncludeDependencies(SourceFile))
				{
					CompileAction.PrerequisiteItems.Add(IncludedFile);
				}

				// Add the source file path to the command-line.
                string bfastlinkstring = bEnableFastIteration ? "" : " -c ";
                string FileArguments = string.Format(bfastlinkstring + " \"{0}\"", SourceFile.AbsolutePath);

            	// Add the object file to the produced item list.
				FileItem ObjectFile = FileItem.GetItemByPath(
					Path.Combine(
						CompileEnvironment.Config.OutputDirectory,
						Path.GetFileName(SourceFile.AbsolutePath) + (bEnableFastIteration ? ".js" : ".o")
						)
					);
				CompileAction.ProducedItems.Add(ObjectFile);
				FileArguments += string.Format(" -o \"{0}\"", ObjectFile.AbsolutePath);

				// Add C or C++ specific compiler arguments.
				if (bIsPlainCFile)
				{
					FileArguments += GetCLArguments_C(CompileEnvironment.Config.TargetArchitecture);
				}
				else
				{
					FileArguments += GetCLArguments_CPP(CompileEnvironment);
				}

				CompileAction.WorkingDirectory = Path.GetFullPath(".");
				CompileAction.CommandPath = PythonPath;
                
                string fastlinkString = SourceFile.Info.FullName.Contains("Launch") ?  " -s MAIN_MODULE=1 " : "-s SIDE_MODULE=1";
                CompileAction.CommandArguments = EMCCPath + Arguments + (bEnableFastIteration ? fastlinkString : "" )+ FileArguments + CompileEnvironment.Config.AdditionalArguments;
               
                System.Console.WriteLine(CompileAction.CommandArguments); 
				CompileAction.StatusDescription = Path.GetFileName(SourceFile.AbsolutePath);
				CompileAction.StatusDetailedDescription = SourceFile.Description;
				CompileAction.OutputEventHandler = new DataReceivedEventHandler(CompileOutputReceivedDataEventHandler);

				// Don't farm out creation of precomputed headers as it is the critical path task.
				CompileAction.bCanExecuteRemotely = CompileEnvironment.Config.PrecompiledHeaderAction != PrecompiledHeaderAction.Create;

				// this is the final output of the compile step (a .abc file)
				Result.ObjectFiles.Add(ObjectFile);

				// VC++ always outputs the source file name being compiled, so we don't need to emit this ourselves
				CompileAction.bShouldOutputStatusDescription = true;

				// Don't farm out creation of precompiled headers as it is the critical path task.
				CompileAction.bCanExecuteRemotely =
					CompileEnvironment.Config.PrecompiledHeaderAction != PrecompiledHeaderAction.Create ||
					BuildConfiguration.bAllowRemotelyCompiledPCHs;
			}
        
			return Result;
		}

		public override CPPOutput CompileRCFiles(CPPEnvironment Environment, List<FileItem> RCFiles)
		{
			CPPOutput Result = new CPPOutput();

			if (Environment.Config.TargetArchitecture == "-win32")
			{
				return base.CompileRCFiles(Environment, RCFiles);
			}

			return Result;
		}

		/**
		 * Translates clang output warning/error messages into vs-clickable messages
		 * 
		 * @param	sender		Sending object
		 * @param	e			Event arguments (In this case, the line of string output)
		 */
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

		public override FileItem LinkFiles(LinkEnvironment LinkEnvironment, bool bBuildImportLibraryOnly)
		{
			if (LinkEnvironment.Config.TargetArchitecture == "-win32")
            {
                return base.LinkFiles(LinkEnvironment, bBuildImportLibraryOnly);
            }

			FileItem OutputFile;

			// Make the final javascript file
			Action LinkAction = new Action(ActionType.Link);

			LinkAction.bCanExecuteRemotely = false;
			LinkAction.WorkingDirectory = Path.GetFullPath(".");
			LinkAction.CommandPath = PythonPath;
			LinkAction.CommandArguments = bEnableFastIteration ? EMLinkPath : EMCCPath;
		    LinkAction.CommandArguments += GetLinkArguments(LinkEnvironment);

			// Add the input files to a response file, and pass the response file on the command-line.
			List<string> InputFileNames = new List<string>();
			string ResponseFileName = Path.Combine(LinkEnvironment.Config.OutputDirectory, Path.GetFileName(LinkEnvironment.Config.OutputFilePath) + ".response");
			foreach (FileItem InputFile in LinkEnvironment.InputFiles)
			{
                System.Console.WriteLine("File  {0} ", InputFile.AbsolutePath);
                string fastlinkString = InputFile.AbsolutePath.Contains("Launch.cpp") ?  " -m " : " -s ";
                LinkAction.CommandArguments += string.Format((bEnableFastIteration ? fastlinkString : "") + " \"{0}\"", InputFile.AbsolutePath);
				LinkAction.PrerequisiteItems.Add(InputFile);
			}
            foreach (string InputFile in LinkEnvironment.Config.AdditionalLibraries)
            {
                FileItem Item = FileItem.GetExistingItemByPath(InputFile);
                if (Item != null)
                {
                    LinkAction.CommandArguments += string.Format(" \"{0}\"", Item.AbsolutePath);
                    LinkAction.PrerequisiteItems.Add(Item);
                }
            }
			// make the file we will create
			OutputFile = FileItem.GetItemByPath(LinkEnvironment.Config.OutputFilePath);
			LinkAction.ProducedItems.Add(OutputFile);
			LinkAction.CommandArguments += string.Format(" -o \"{0}\"", OutputFile.AbsolutePath);
            if ( !bEnableFastIteration ) 
            { 
 			    FileItem OutputBC = FileItem.GetItemByPath(LinkEnvironment.Config.OutputFilePath.Replace(".js", ".bc").Replace(".html", ".bc"));
 			    LinkAction.ProducedItems.Add(OutputBC);
 			    LinkAction.CommandArguments += string.Format(" --save-bc \"{0}\"", OutputBC.AbsolutePath);
            } 

     		LinkAction.StatusDescription = Path.GetFileName(OutputFile.AbsolutePath);
			LinkAction.OutputEventHandler = new DataReceivedEventHandler(RemoteOutputReceivedEventHandler);

			return OutputFile;
		}

		public override void CompileCSharpProject(CSharpEnvironment CompileEnvironment, string ProjectFileName, string DestinationFile)
		{
			throw new BuildException("HTML5 cannot compile C# files");
		}
	};
}
