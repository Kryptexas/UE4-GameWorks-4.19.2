// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace UnrealBuildTool.Linux
{
    class LinuxToolChain : UEToolChain
    {
        public override void RegisterToolChain()
        {
            BaseLinuxPath = Environment.GetEnvironmentVariable("LINUX_ROOT");

            // don't register if we don't have an LINUX_ROOT specified
            if (String.IsNullOrEmpty(BaseLinuxPath))
                return;

            BaseLinuxPath = BaseLinuxPath.Replace("\"", "");

            // set up the path to our toolchains
            ClangPath = Path.Combine(BaseLinuxPath, @"bin\Clang++.exe");
            ArPath = Path.Combine(BaseLinuxPath, @"bin\x86_64-unknown-linux-gnu-ar.exe");
            RanlibPath = Path.Combine(BaseLinuxPath, @"bin\x86_64-unknown-linux-gnu-ranlib.exe");

            // Register this tool chain for both Linux
            if (BuildConfiguration.bPrintDebugInfo)
            {
                Console.WriteLine("        Registered for {0}", CPPTargetPlatform.Linux.ToString());
            }
            UEToolChain.RegisterPlatformToolChain(CPPTargetPlatform.Linux, this);
        }

        static string GetCLArguments_Global(CPPEnvironment CompileEnvironment)
        {
            string Result = "";

            // build up the commandline common to C and C++
            Result += " -c";
            Result += " -pipe";
            Result += " -fomit-frame-pointer";
            Result += " -fno-exceptions";               // no exceptions
            Result += " -funwind-tables";               // generate unwind tables as they seem to be needed for stack tracing (why??)
            Result += " -Wall -Werror";
            Result += " -Wsequence-point";              // additional warning not normally included in Wall: warns if order of operations is ambigious
            //Result += " -Wunreachable-code";            // additional warning not normally included in Wall: warns if there is code that will never be executed - not helpful due to bIsGCC and similar 
            //Result += " -Wshadow";                      // additional warning not normally included in Wall: warns if there variable/typedef shadows some other variable - not helpful because we have gobs of code that shadows variables
            Result += " -mmmx -msse -msse2";            // allows use of SIMD intrinsics
            Result += " -fno-math-errno";               // do not assume that math ops have side effects
            Result += " -fdiagnostics-format=msvc";     // make diagnostics compatible with MSVC

            Result += " -Wno-unused-variable";
            // this will hide the warnings about static functions in headers that aren't used in every single .cpp file
            Result += " -Wno-unused-function";
            // this hides the "enumeration value 'XXXXX' not handled in switch [-Wswitch]" warnings - we should maybe remove this at some point and add UE_LOG(, Fatal, ) to default cases
            Result += " -Wno-switch";
            // this hides the "warning : comparison of unsigned expression < 0 is always false" type warnings due to constant comparisons, which are possible with template arguments
            Result += " -Wno-tautological-compare";
            Result += " -Wno-unknown-pragmas";			// Slate triggers this (with its optimize on/off pragmas)
            Result += " -Wno-unused-private-field";     // MultichannelTcpSocket.h triggers this, possibly more
			Result += " -Wno-invalid-offsetof"; // needed to suppress warnings about using offsetof on non-POD types.

            //Result += " -DOPERATOR_NEW_INLINE=FORCENOINLINE";

            // shipping builds will cause this warning with "ensure", so disable only in those case
            if (CompileEnvironment.Config.TargetConfiguration == CPPTargetConfiguration.Shipping)
            {
                Result += " -Wno-unused-value";
            }

            // debug info
            if (CompileEnvironment.Config.bCreateDebugInfo)
            {
                Result += " -g3";
            }
            else if (CompileEnvironment.Config.TargetConfiguration < CPPTargetConfiguration.Shipping)
            {
                Result += " -gline-tables-only"; // include debug info for meaningful callstacks
            }
            else
            {
                // in shipping, strip as much info as possible
                Result += " -g0";
                Result += " -fvisibility=hidden";           // prevents from exporting all symbols (reduces the size of the binary)
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

            //Result += " -v";                            // for better error diagnosis

            Result += " -target x86_64-unknown-linux-gnu";        // Set target triple
            Result += " -D _LINUX64";
            Result += String.Format(" --sysroot={0}", BaseLinuxPath);

            return Result;
        }

        static string GetCompileArguments_CPP()
        {
            string Result = "";
            Result += " -x c++";
            Result += " -std=c++11";
            return Result;
        }

        static string GetCompileArguments_C()
        {
            string Result = "";
            Result += " -x c";
            return Result;
        }

        static string GetCompileArguments_MM()
        {
            string Result = "";
            Result += " -x objective-c++";
            Result += " -fobjc-abi-version=2";
            Result += " -fobjc-legacy-dispatch";
            Result += " -fno-rtti";
            Result += " -std=c++11";
            return Result;
        }

        static string GetCompileArguments_M()
        {
            string Result = "";
            Result += " -x objective-c";
            Result += " -fobjc-abi-version=2";
            Result += " -fobjc-legacy-dispatch";
            Result += " -std=c++11";
            return Result;
        }

        static string GetCompileArguments_PCH()
        {
            string Result = "";
            Result += " -x c++-header";
            Result += " -std=c++11";
            return Result;
        }

        static string GetLinkArguments(LinkEnvironment LinkEnvironment)
        {
            string Result = "";

            // debugging symbols
            if (LinkEnvironment.Config.TargetConfiguration < CPPTargetConfiguration.Shipping)
            {
                Result += " -rdynamic";
            }
            else
            {
                Result += " -Wl,-s"; // Strip binaries in Shipping
            }

            if (LinkEnvironment.Config.bIsBuildingDLL)
            {
                Result += " -shared";
            }
            Result += " -v";

            // ignore unresolved symbols in shared libs (handy sometimes)
            //Result += " -Wl,--unresolved-symbols=ignore-in-shared-libs";

            Result += " -target x86_64-unknown-linux-gnu";        // Set target triple
            Result += String.Format(" --sysroot={0}", BaseLinuxPath);

            return Result;
        }

		static string GetArchiveArguments(LinkEnvironment LinkEnvironment)
		{
			return " rc";
		}

        public static void CompileOutputReceivedDataEventHandler(Object Sender, DataReceivedEventArgs e)
        {
            string Output = e.Data;
            if (String.IsNullOrEmpty(Output))
            {
                return;
            }

            // Need to match following for clickable links
            string RegexFilePath = @"^[A-Z]\:([\\\/][A-Za-z0-9_\-\.]*)+\.(cpp|c|mm|m|hpp|h)";
            string RegexLineNumber = @"\:\d+\:\d+\:";
            string RegexDescription = @"(\serror:\s|\swarning:\s|\snote:\s).*";

            // Get Matches
            string MatchFilePath = Regex.Match(Output, RegexFilePath).Value.Replace("Engine\\Source\\..\\..\\", "");
            string MatchLineNumber = Regex.Match(Output, RegexLineNumber).Value;
            string MatchDescription = Regex.Match(Output, RegexDescription).Value;

            // If any of the above matches failed, do nothing
            if (MatchFilePath.Length == 0 ||
                MatchLineNumber.Length == 0 ||
                MatchDescription.Length == 0)
            {
                Console.WriteLine(Output);
                return;
            }

            // Convert Path
            string RegexStrippedPath = @"\\Engine\\.*"; //@"(Engine\/|[A-Za-z0-9_\-\.]*\/).*";
            string ConvertedFilePath = Regex.Match(MatchFilePath, RegexStrippedPath).Value;
            ConvertedFilePath = Path.GetFullPath("..\\.." + ConvertedFilePath);

            // Extract Line + Column Number
            string ConvertedLineNumber = Regex.Match(MatchLineNumber, @"\d+").Value;
            string ConvertedColumnNumber = Regex.Match(MatchLineNumber, @"(?<=:\d+:)\d+").Value;

            // Write output
            string ConvertedExpression = "  " + ConvertedFilePath + "(" + ConvertedLineNumber + "," + ConvertedColumnNumber + "):" + MatchDescription;
            Console.WriteLine(ConvertedExpression); // To create clickable vs link
        }

        // cache the location of NDK tools
        static string BaseLinuxPath;
        static string ClangPath;
        static string ArPath;
        static string RanlibPath;

        public override CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, List<FileItem> SourceFiles, string ModuleName)
        {
            string Arguments = GetCLArguments_Global(CompileEnvironment);
            string PCHArguments = "";

            if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Include)
            {
                // Add the precompiled header file's path to the include path so Clang can find it.
                // This needs to be before the other include paths to ensure Clang uses it instead of the source header file.
                PCHArguments += string.Format(" -include \"{0}\"", CompileEnvironment.PrecompiledHeaderFile.AbsolutePath.Replace(".gch", ""));
            }

            // Add include paths to the argument list.
            foreach (string IncludePath in CompileEnvironment.Config.IncludePaths)
            {
                Arguments += string.Format(" -I\"{0}\"", IncludePath);
            }
            foreach (string IncludePath in CompileEnvironment.Config.SystemIncludePaths)
            {
                Arguments += string.Format(" -I\"{0}\"", IncludePath);
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
                string Extension = Path.GetExtension(SourceFile.AbsolutePath).ToUpperInvariant();

                // Add C or C++ specific compiler arguments.
                if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Create)
                {
                    FileArguments += GetCompileArguments_PCH();
                }
                else if (Extension == ".C")
                {
                    // Compile the file as C code.
                    FileArguments += GetCompileArguments_C();
                }
                else if (Extension == ".CC")
                {
                    // Compile the file as C++ code.
                    FileArguments += GetCompileArguments_CPP();
                }
                else if (Extension == ".MM")
                {
                    // Compile the file as Objective-C++ code.
                    FileArguments += GetCompileArguments_MM();
                }
                else if (Extension == ".M")
                {
                    // Compile the file as Objective-C code.
                    FileArguments += GetCompileArguments_M();
                }
                else
                {
                    FileArguments += GetCompileArguments_CPP();

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

                CompileAction.WorkingDirectory = Path.GetFullPath(".");
                CompileAction.CommandPath = ClangPath;
                CompileAction.CommandArguments = Arguments + FileArguments + CompileEnvironment.Config.AdditionalArguments;
                CompileAction.StatusDescription = string.Format("{0}", Path.GetFileName(SourceFile.AbsolutePath));
                CompileAction.StatusDetailedDescription = SourceFile.Description;
                CompileAction.bIsGCCCompiler = true;

                // Don't farm out creation of pre-compiled headers as it is the critical path task.
                CompileAction.bCanExecuteRemotely =
                    CompileEnvironment.Config.PrecompiledHeaderAction != PrecompiledHeaderAction.Create ||
                    BuildConfiguration.bAllowRemotelyCompiledPCHs;

                CompileAction.OutputEventHandler = new DataReceivedEventHandler(CompileOutputReceivedDataEventHandler);
            }

            return Result;
        }

        /** Creates an action to archive all the .o files into single .a file */
        public FileItem CreateArchiveAndIndex(LinkEnvironment LinkEnvironment)
        {
            // Create an archive action
            Action ArchiveAction = new Action(ActionType.Link);
            ArchiveAction.WorkingDirectory = Path.GetFullPath(".");
            ArchiveAction.CommandPath = "cmd.exe";
            ArchiveAction.CommandArguments = "/c ";

            // this will produce a final library
            ArchiveAction.bProducesImportLibrary = true;

            // Add the output file as a production of the link action.
            FileItem OutputFile = FileItem.GetItemByPath(LinkEnvironment.Config.OutputFilePath);
            ArchiveAction.ProducedItems.Add(OutputFile);
            ArchiveAction.StatusDescription = string.Format("{0}", Path.GetFileName(OutputFile.AbsolutePath));
            ArchiveAction.CommandArguments += string.Format("{0} {1} \"{2}\"",  ArPath, GetArchiveArguments(LinkEnvironment), OutputFile.AbsolutePath);

            // Add the input files to a response file, and pass the response file on the command-line.
            List<string> InputFileNames = new List<string>();
            foreach (FileItem InputFile in LinkEnvironment.InputFiles)
            {
                string InputAbsolutePath = InputFile.AbsolutePath.Replace("\\", "/");
                InputFileNames.Add(string.Format("\"{0}\"", InputAbsolutePath));
                ArchiveAction.PrerequisiteItems.Add(InputFile);
                ArchiveAction.CommandArguments += string.Format(" \"{0}\"", InputAbsolutePath);
            }

            // add ranlib
            ArchiveAction.CommandArguments += string.Format(" && {0} \"{1}\"", RanlibPath, OutputFile.AbsolutePath);

            // Add the additional arguments specified by the environment.
            ArchiveAction.CommandArguments += LinkEnvironment.Config.AdditionalArguments;
            ArchiveAction.CommandArguments.Replace("\\", "/");

            // Only execute linking on the local PC.
            ArchiveAction.bCanExecuteRemotely = false;

            return OutputFile;
        }

        public override FileItem LinkFiles(LinkEnvironment LinkEnvironment, bool bBuildImportLibraryOnly)
        {
            if (LinkEnvironment.Config.bIsBuildingLibrary || bBuildImportLibraryOnly)
            {
                return CreateArchiveAndIndex(LinkEnvironment);
            }

            // Create an action that invokes the linker.
            Action LinkAction = new Action(ActionType.Link);
            LinkAction.WorkingDirectory = Path.GetFullPath(".");
            LinkAction.CommandPath = ClangPath;

            // Get link arguments.
            LinkAction.CommandArguments = GetLinkArguments(LinkEnvironment);

            // Tell the action that we're building an import library here and it should conditionally be
            // ignored as a prerequisite for other actions
            LinkAction.bProducesImportLibrary = LinkEnvironment.Config.bIsBuildingDLL;

            // Add the output file as a production of the link action.
            FileItem OutputFile = FileItem.GetItemByPath(LinkEnvironment.Config.OutputFilePath);
            LinkAction.ProducedItems.Add(OutputFile);
            LinkAction.StatusDescription = string.Format("{0}", Path.GetFileName(OutputFile.AbsolutePath));

            // Add the output file to the command-line.
            LinkAction.CommandArguments += string.Format(" -o \"{0}\"", OutputFile.AbsolutePath);

            // Add the input files to a response file, and pass the response file on the command-line.
            List<string> InputFileNames = new List<string>();
            foreach (FileItem InputFile in LinkEnvironment.InputFiles)
            {
                InputFileNames.Add(string.Format("\"{0}\"", InputFile.AbsolutePath.Replace("\\", "/")));
                LinkAction.PrerequisiteItems.Add(InputFile);
            }

            string ResponseFileName = GetResponseFileName(LinkEnvironment, OutputFile);
            LinkAction.CommandArguments += string.Format(" -Wl,@\"{0}\"", ResponseFile.Create(ResponseFileName, InputFileNames));

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
					LinkAction.PrerequisiteItems.Add(FileItem.GetItemByFullPath(AdditionalLibrary));
				}
			}
            LinkAction.CommandArguments += " -lrt"; // needed for clock_gettime()
            LinkAction.CommandArguments += string.Format(" -Wl,--end-group");

            // Add the additional arguments specified by the environment.
            LinkAction.CommandArguments += LinkEnvironment.Config.AdditionalArguments;
            LinkAction.CommandArguments.Replace("\\", "/");

            // Only execute linking on the local PC.
            LinkAction.bCanExecuteRemotely = false;

            return OutputFile;
        }

        public override void CompileCSharpProject(CSharpEnvironment CompileEnvironment, string ProjectFileName, string DestinationFile)
        {
            throw new BuildException("Linux cannot compile C# files");
        }
    }
}
