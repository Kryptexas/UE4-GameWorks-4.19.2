// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Security.AccessControl;

namespace UnrealBuildTool
{
	class MacToolChain : RemoteToolChain
	{
		public override void RegisterToolChain()
		{
			RegisterRemoteToolChain(UnrealTargetPlatform.Mac, CPPTargetPlatform.Mac);
		}

		/***********************************************************************
		 * NOTE:
		 *  Do NOT change the defaults to set your values, instead you should set the environment variables
		 *  properly in your system, as other tools make use of them to work properly!
		 *  The defaults are there simply for examples so you know what to put in your env vars...
		 ***********************************************************************

		/** Which version of the Mac OS SDK to target at build time */
		public static string MacOSSDKVersion = "10.9";

		/** Which version of the Mac OS X to allow at run time */
		public static string MacOSVersion = "10.9";

		/** Minimum version of Mac OS X to actually run on, running on earlier versions will display the system minimum version error dialog & exit. */
		public static string MinMacOSVersion = "10.9.2";

		/** Which developer directory to root from */
		private static string DeveloperDir = "/Applications/Xcode.app/Contents/Developer/";

		/** Which compiler frontend to use */
		private static string MacCompiler = "clang++";

		/** Which linker frontend to use */
		private static string MacLinker = "clang++";

		/** Which archiver to use */
		private static string MacArchiver = "libtool";


		/** Track which scripts need to be deleted before appending to */
		private bool bHasWipedCopyDylibScript = false;
		private bool bHasWipedFixDylibScript = false;

		private static List<FileItem> BundleDependencies = new List<FileItem>();

		public List<string> BuiltBinaries = new List<string>();

		private static List<FileItem> CrossReferencedFiles = new List<FileItem>();

		public override void SetUpGlobalEnvironment()
		{
			base.SetUpGlobalEnvironment();

			if (!ProjectFileGenerator.bGenerateProjectFiles)
			{
				if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
				{
					Log.TraceInformation("Compiling with Mac SDK {0} on Mac {1}", MacOSSDKVersion, RemoteServerName);
				}
				else
				{
					Log.TraceInformation("Compiling with Mac SDK {0}", MacOSSDKVersion);
				}
			}
		}

		static string GetCompileArguments_Global(CPPEnvironment CompileEnvironment)
		{
			string Result = "";

			Result += " -fmessage-length=0";
			Result += " -pipe";
			Result += " -Wno-trigraphs";
			Result += " -fpascal-strings";
			Result += " -mdynamic-no-pic";
			Result += " -Wreturn-type";
			Result += " -Wno-format";
			Result += " -fexceptions";
			Result += " -fno-threadsafe-statics";
			Result += " -Wno-deprecated-writable-strings";
			Result += " -Wno-unused-value";
			Result += " -Wno-switch-enum";
			Result += " -Wno-logical-op-parentheses";	// needed for external headers we can't change
			Result += " -Wno-null-arithmetic";			// needed for external headers we can't change
			Result += " -Wno-return-type-c-linkage";	// needed for PhysX
			Result += " -Wno-ignored-attributes";		// needed for nvtesslib
			Result += " -Wno-uninitialized";
			Result += " -Wno-tautological-compare";
			Result += " -Wno-switch";
			Result += " -Wno-invalid-offsetof"; // needed to suppress warnings about using offsetof on non-POD types.
			Result += " -fasm-blocks";
			Result += " -c";

			Result += " -arch x86_64";
			Result += " -isysroot " + DeveloperDir + "Platforms/MacOSX.platform/Developer/SDKs/MacOSX" + MacOSSDKVersion + ".sdk";
			Result += " -mmacosx-version-min=" + MacOSVersion;

			// Optimize non- debug builds.
			if (CompileEnvironment.Config.Target.Configuration != CPPTargetConfiguration.Debug)
			{
				Result += " -O3";
			}
			else
			{
				Result += " -O0";
			}

			// Create DWARF format debug info if wanted,
			if (CompileEnvironment.Config.bCreateDebugInfo)
			{
				Result += " -gdwarf-2";
			}

			return Result;
		}

		static string GetCompileArguments_CPP()
		{
			string Result = "";
			Result += " -x objective-c++";
			Result += " -fobjc-abi-version=2";
			Result += " -fobjc-legacy-dispatch";
			Result += " -fno-rtti";
			Result += " -std=c++11";
			Result += " -stdlib=libc++";
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
			Result += " -stdlib=libc++";
			return Result;
		}

		static string GetCompileArguments_M()
		{
			string Result = "";
			Result += " -x objective-c";
			Result += " -fobjc-abi-version=2";
			Result += " -fobjc-legacy-dispatch";
			Result += " -std=c++11";
			Result += " -stdlib=libc++";
			return Result;
		}

		static string GetCompileArguments_C()
		{
			string Result = "";
			Result += " -x c";
			return Result;
		}

		static string GetCompileArguments_PCH()
		{
			string Result = "";
			Result += " -x objective-c++-header";
			Result += " -fobjc-abi-version=2";
			Result += " -fobjc-legacy-dispatch";
			Result += " -fno-rtti";
			Result += " -std=c++11";
			Result += " -stdlib=libc++";
			return Result;
		}

		static string GetLinkArguments_Global(LinkEnvironment LinkEnvironment)
		{
			string Result = "";

			Result += " -arch x86_64";
			Result += " -isysroot " + DeveloperDir + "Platforms/MacOSX.platform/Developer/SDKs/MacOSX" + MacOSSDKVersion + ".sdk";
			Result += " -mmacosx-version-min=" + MacOSVersion;
			Result += " -dead_strip";

			if (LinkEnvironment.Config.bIsBuildingDLL)
			{
				Result += " -dynamiclib";
			}

			// Needed to make sure install_name_tool will be able to update paths in Mach-O headers
			Result += " -headerpad_max_install_names";

			Result += " -lc++"; // for STL used in HLSLCC

			foreach (string Framework in LinkEnvironment.Config.Frameworks)
			{
				Result += " -framework " + Framework;
			}
			foreach (UEBuildFramework Framework in LinkEnvironment.Config.AdditionalFrameworks)
			{
				Result += " -framework " + Framework.FrameworkName;
			}
			foreach (string Framework in LinkEnvironment.Config.WeakFrameworks)
			{
				Result += " -weak_framework " + Framework;
			}

			return Result;
		}

		static string GetArchiveArguments_Global(LinkEnvironment LinkEnvironment)
		{
			string Result = "";

			Result += " -static";

			return Result;
		}

		public override CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, List<FileItem> SourceFiles, string ModuleName)
		{
			string Arguments = GetCompileArguments_Global(CompileEnvironment);
			string PCHArguments = "";

			if (CompileEnvironment.Config.PrecompiledHeaderAction != PrecompiledHeaderAction.Create)
			{
				Arguments += " -Werror";
			}

			if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Include)
			{
				// Add the precompiled header file's path to the include path so GCC can find it.
				// This needs to be before the other include paths to ensure GCC uses it instead of the source header file.
				var PrecompiledFileExtension = UEBuildPlatform.BuildPlatformDictionary[UnrealTargetPlatform.Mac].GetBinaryExtension(UEBuildBinaryType.PrecompiledHeader);
				PCHArguments += string.Format(" -include \"{0}\"", CompileEnvironment.PrecompiledHeaderFile.AbsolutePath.Replace(PrecompiledFileExtension, ""));
			}

			// Add include paths to the argument list.
			List<string> AllIncludes = CompileEnvironment.Config.IncludePaths;
			AllIncludes.AddRange(CompileEnvironment.Config.SystemIncludePaths);
			foreach (string IncludePath in AllIncludes)
			{
				Arguments += string.Format(" -I\"{0}\"", ConvertPath(Path.GetFullPath(IncludePath)));

				if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
				{
					// sync any third party headers we may need
					if (IncludePath.Contains("ThirdParty"))
					{
						string[] FileList = Directory.GetFiles(IncludePath, "*.h", SearchOption.AllDirectories);
						foreach (string File in FileList)
						{
							FileItem ExternalDependency = FileItem.GetItemByPath(File);
							LocalToRemoteFileItem(ExternalDependency, true);
						}

						FileList = Directory.GetFiles(IncludePath, "*.cpp", SearchOption.AllDirectories);
						foreach (string File in FileList)
						{
							FileItem ExternalDependency = FileItem.GetItemByPath(File);
							LocalToRemoteFileItem(ExternalDependency, true);
						}
					}
				}
			}

			foreach (string Definition in CompileEnvironment.Config.Definitions)
			{
				Arguments += string.Format(" -D\"{0}\"", Definition);
			}

			CPPOutput Result = new CPPOutput();
			// Create a compile action for each source file.
			foreach (FileItem SourceFile in SourceFiles)
			{
				Action CompileAction = new Action(ActionType.Compile);
				string FileArguments = "";
				string Extension = Path.GetExtension(SourceFile.AbsolutePath).ToUpperInvariant();

				if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Create)
				{
					// Compile the file as a C++ PCH.
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
					// Compile the file as Objective-C++ code.
					FileArguments += GetCompileArguments_M();
				}
				else
				{
					// Compile the file as C++ code.
					FileArguments += GetCompileArguments_CPP();

					// only use PCH for .cpp files
					FileArguments += PCHArguments;
				}

				// Add the C++ source file and its included files to the prerequisite item list.
				CompileAction.PrerequisiteItems.Add(SourceFile);

				if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
				{
					QueueFileForBatchUpload(SourceFile);
				}

				foreach (FileItem IncludedFile in CompileEnvironment.GetIncludeDependencies(SourceFile))
				{
					if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
					{
						QueueFileForBatchUpload(IncludedFile);
					}

					CompileAction.PrerequisiteItems.Add(IncludedFile);
				}

				if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Create)
				{
					var PrecompiledHeaderExtension = UEBuildPlatform.BuildPlatformDictionary[UnrealTargetPlatform.Mac].GetBinaryExtension(UEBuildBinaryType.PrecompiledHeader);
					// Add the precompiled header file to the produced item list.
					FileItem PrecompiledHeaderFile = FileItem.GetItemByPath(
						Path.Combine(
							CompileEnvironment.Config.OutputDirectory,
							Path.GetFileName(SourceFile.AbsolutePath) + PrecompiledHeaderExtension
							)
						);

					FileItem RemotePrecompiledHeaderFile = LocalToRemoteFileItem(PrecompiledHeaderFile, false);
					CompileAction.ProducedItems.Add(RemotePrecompiledHeaderFile);
					Result.PrecompiledHeaderFile = RemotePrecompiledHeaderFile;

					// Add the parameters needed to compile the precompiled header file to the command-line.
					FileArguments += string.Format(" -o \"{0}\"", RemotePrecompiledHeaderFile.AbsolutePath, false);
				}
				else
				{
					if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Include)
					{
						CompileAction.bIsUsingPCH = true;
						CompileAction.PrerequisiteItems.Add(CompileEnvironment.PrecompiledHeaderFile);
					}
					var ObjectFileExtension = UEBuildPlatform.BuildPlatformDictionary[UnrealTargetPlatform.Mac].GetBinaryExtension(UEBuildBinaryType.Object);
					// Add the object file to the produced item list.
					FileItem ObjectFile = FileItem.GetItemByPath(
						Path.Combine(
							CompileEnvironment.Config.OutputDirectory,
							Path.GetFileName(SourceFile.AbsolutePath) + ObjectFileExtension
							)
						);

					FileItem RemoteObjectFile = LocalToRemoteFileItem(ObjectFile, false);
					CompileAction.ProducedItems.Add(RemoteObjectFile);
					Result.ObjectFiles.Add(RemoteObjectFile);
					FileArguments += string.Format(" -o \"{0}\"", RemoteObjectFile.AbsolutePath, false);
				}

				// Add the source file path to the command-line.
				FileArguments += string.Format(" \"{0}\"", ConvertPath(SourceFile.AbsolutePath), false);

				if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
				{
					CompileAction.ActionHandler = new Action.BlockingActionHandler(RPCUtilHelper.RPCActionHandler);
				}

				CompileAction.WorkingDirectory = GetMacDevSrcRoot();
				
				string CompilerBundleIdentifier = Environment.GetEnvironmentVariable("GCC_VERSION");
				string StaticAnalysisMode = Environment.GetEnvironmentVariable("CLANG_STATIC_ANALYZER_MODE");
				if(CompilerBundleIdentifier == null || CompilerBundleIdentifier == "" 
					|| CompilerBundleIdentifier == "com.apple.compilers.llvm.clang.1_0" 
					|| (StaticAnalysisMode != null && StaticAnalysisMode != ""))
				{
					CompileAction.CommandPath = MacCompiler;
					if(StaticAnalysisMode != null && StaticAnalysisMode != "")
					{
						FileArguments = " --analyze " + FileArguments;
					}
				}
				else
				{
					CompileAction.CommandPath = null;
					string UserDir = Environment.GetEnvironmentVariable("HOME");
					string XcodePlugins = UserDir + "/Library/Application Support/Developer/Shared/Xcode/Plug-ins/";
					if(Directory.Exists(XcodePlugins))
					{
						IEnumerable<string> Plugins = Directory.EnumerateDirectories(XcodePlugins, "*.xcplugin");
						foreach(string Plugin in Plugins)
						{
							string Resources = Plugin + "/Contents/Resources";
							string[] CompilerSpecs = Directory.GetFiles(Resources, "*.xcspec");
							if(CompilerSpecs.Length > 0)
							{
								IEnumerable<string> Lines = File.ReadLines(CompilerSpecs[0]);
								bool bFindExecPath = false;
								bool bFoundExecPath = false;
								foreach(string Line in Lines)
								{
									if(!bFindExecPath && Line.Contains(CompilerBundleIdentifier))
									{
										bFindExecPath = true;
									}
									else if(bFindExecPath && Line.Contains("ExecPath"))
									{
										char[] CharsToTrim = {';', '"'};
										string ExecPath = Line.Trim().Substring(12).TrimEnd(CharsToTrim);
										CompileAction.CommandPath = ExecPath;
										bFoundExecPath = true;
										break;
									}
								}
								if(bFoundExecPath)
								{
									break;
								}
							}
						}
					}
					else
					{
						throw new BuildException("Couldn't find Xcode compiler plugins path: {0}", XcodePlugins);
					}
					
					if(CompileAction.CommandPath == null)
					{
						throw new BuildException("Couldn't find Xcode compiler: {0}", CompilerBundleIdentifier);
					}
				}
				CompileAction.CommandArguments = Arguments + FileArguments + CompileEnvironment.Config.AdditionalArguments;
				CompileAction.CommandDescription = "Compile";
				CompileAction.StatusDescription = Path.GetFileName(SourceFile.AbsolutePath);
				CompileAction.StatusDetailedDescription = SourceFile.Description;
				CompileAction.bIsGCCCompiler = true;
				// We're already distributing the command by execution on Mac.
				CompileAction.bCanExecuteRemotely = false;
				CompileAction.OutputEventHandler = new DataReceivedEventHandler(RemoteOutputReceivedEventHandler);
			}
			return Result;
		}

		private void AppendMacLine(StreamWriter Writer, string Format, params object[] Arg)
		{
			string PreLine = String.Format(Format, Arg);
			Writer.Write(PreLine + "\n");
		}

		private int LoadEngineCL()
		{
			string[] VersionHeader = Utils.ReadAllText("../Source/Runtime/Launch/Resources/Version.h").Replace("\r\n", "\n").Replace("\t", " ").Split('\n');
			foreach (string Line in VersionHeader)
			{
				if (Line.StartsWith("#define ENGINE_VERSION "))
				{
					return int.Parse(Line.Split(' ')[2]);
				}
			}
			return 0;
		}

		private string LoadEngineDisplayVersion(bool bIgnorePatchVersion = false)
		{
			string[] VersionHeader = Utils.ReadAllText("../Source/Runtime/Launch/Resources/Version.h").Replace("\r\n", "\n").Replace("\t", " ").Split('\n');
			string EngineVersionMajor = "4";
			string EngineVersionMinor = "0";
			string EngineVersionPatch = "0";
			foreach (string Line in VersionHeader)
			{
				if (Line.StartsWith("#define ENGINE_MAJOR_VERSION "))
				{
					EngineVersionMajor = Line.Split(' ')[2];
				}
				else if (Line.StartsWith("#define ENGINE_MINOR_VERSION "))
				{
					EngineVersionMinor = Line.Split(' ')[2];
				}
				else if (Line.StartsWith("#define ENGINE_PATCH_VERSION ") && !bIgnorePatchVersion)
				{
					EngineVersionPatch = Line.Split(' ')[2];
				}
			}
			return EngineVersionMajor + "." + EngineVersionMinor + "." + EngineVersionPatch;
		}

		private string LoadLauncherDisplayVersion()
		{
			string[] VersionHeader = Utils.ReadAllText("../Source/Programs/NoRedist/UnrealEngineLauncher/Private/Version.h").Replace("\r\n", "\n").Replace("\t", " ").Split('\n');
			string LauncherVersionMajor = "1";
			string LauncherVersionMinor = "0";
			string LauncherVersionPatch = "0";
			foreach (string Line in VersionHeader)
			{
				if (Line.StartsWith("#define LAUNCHER_MAJOR_VERSION "))
				{
					LauncherVersionMajor = Line.Split(' ')[2];
				}
				else if (Line.StartsWith("#define LAUNCHER_MINOR_VERSION "))
				{
					LauncherVersionMinor = Line.Split(' ')[2];
				}
				else if (Line.StartsWith("#define LAUNCHER_PATCH_VERSION "))
				{
					LauncherVersionPatch = Line.Split(' ')[2];
				}
			}
			return LauncherVersionMajor + "." + LauncherVersionMinor + "." + LauncherVersionPatch;
		}

		private string LoadEngineAPIVersion()
		{
			int CL = 0;
			foreach (string Line in File.ReadAllLines("../Source/Runtime/Core/Public/Modules/ModuleVersion.h"))
			{
				string[] Tokens = Line.Split(' ', '\t');
				if (Tokens[0] == "#define" && Tokens[1] == "MODULE_API_VERSION")
				{
					if(Tokens[2] == "BUILT_FROM_CHANGELIST")
					{
						CL = LoadEngineCL();
					}
					else
					{
						CL = int.Parse(Tokens[2]);
					}
					break;
				}
			}
			return String.Format("{0}.{1}.{2}", CL / (100 * 100), (CL / 100) % 100, CL % 100);
		}

		public override FileItem LinkFiles(LinkEnvironment LinkEnvironment, bool bBuildImportLibraryOnly)
		{
			bool bIsBuildingLibrary = LinkEnvironment.Config.bIsBuildingLibrary || bBuildImportLibraryOnly;

			// Create an action that invokes the linker.
			Action LinkAction = new Action(ActionType.Link);

			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				LinkAction.ActionHandler = new Action.BlockingActionHandler(RPCUtilHelper.RPCActionHandler);
			}

			LinkAction.WorkingDirectory = GetMacDevSrcRoot();
			LinkAction.CommandPath = "/bin/sh";
			LinkAction.CommandDescription = "Link";

			string EngineAPIVersion = LoadEngineAPIVersion();
			string EngineDisplayVersion = LoadEngineDisplayVersion(true);
			string VersionArg = LinkEnvironment.Config.bIsBuildingDLL ? " -current_version " + EngineAPIVersion + " -compatibility_version " + EngineDisplayVersion : "";

			string Linker = bIsBuildingLibrary ? MacArchiver : MacLinker;
			string LinkCommand = "xcrun " + Linker + VersionArg + " " + (bIsBuildingLibrary ? GetArchiveArguments_Global(LinkEnvironment) : GetLinkArguments_Global(LinkEnvironment));

			// Tell the action that we're building an import library here and it should conditionally be
			// ignored as a prerequisite for other actions
			LinkAction.bProducesImportLibrary = !Utils.IsRunningOnMono && (bIsBuildingLibrary || LinkEnvironment.Config.bIsBuildingDLL);

			// Add the output file as a production of the link action.
			FileItem OutputFile = FileItem.GetItemByPath(Path.GetFullPath(LinkEnvironment.Config.OutputFilePath));
			OutputFile.bNeedsHotReloadNumbersDLLCleanUp = LinkEnvironment.Config.bIsBuildingDLL;

			FileItem RemoteOutputFile = LocalToRemoteFileItem(OutputFile, false);

			// To solve the problem with cross dependencies, for now we create a broken dylib that does not link with other engine dylibs.
			// This is fixed in later step, FixDylibDependencies. For this and to know what libraries to copy whilst creating an app bundle,
			// we gather the list of engine dylibs.
			List<string> EngineAndGameLibraries = new List<string>();

			string DylibsPath = "@rpath";

			string AbsolutePath = OutputFile.AbsolutePath.Replace("\\", "/");
			if (!AbsolutePath.Contains("/Engine/Binaries/Mac/"))
			{
				DylibsPath = AbsolutePath.Contains("/Plugins/") ? "@rpath" : "@loader_path";
			}
			if (!bIsBuildingLibrary)
			{
				LinkCommand += " -rpath @loader_path/ -rpath @executable_path/";
			}

			List<string> ThirdPartyLibraries = new List<string>();

			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				// Add any additional files that we'll need in order to link the app
				foreach (string AdditionalShadowFile in LinkEnvironment.Config.AdditionalShadowFiles)
				{
					FileItem ShadowFile = FileItem.GetExistingItemByPath(AdditionalShadowFile);
					if (ShadowFile != null)
					{
						QueueFileForBatchUpload(ShadowFile);
						LinkAction.PrerequisiteItems.Add(ShadowFile);
					}
					else
					{
						throw new BuildException("Couldn't find required additional file to shadow: {0}", AdditionalShadowFile);
					}
				}
			}

			if (!bIsBuildingLibrary || LinkEnvironment.Config.bIncludeDependentLibrariesInLibrary)
			{
				List<string> RPaths = new List<string>();

				// Add the additional libraries to the argument list.
				foreach (string AdditionalLibrary in LinkEnvironment.Config.AdditionalLibraries)
				{
					// Can't link dynamic libraries when creating a static one
					if (bIsBuildingLibrary && (Path.GetExtension(AdditionalLibrary) == ".dylib" || AdditionalLibrary == "z"))
					{
						continue;
					}

					if (Path.GetFileName(AdditionalLibrary).StartsWith("lib"))
					{
						LinkCommand += string.Format(" \"{0}\"", ConvertPath(Path.GetFullPath(AdditionalLibrary)));
						if (Path.GetExtension(AdditionalLibrary) == ".dylib")
						{
							ThirdPartyLibraries.Add(AdditionalLibrary);
						}

						if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
						{
							// copy over libs we may need
							FileItem ShadowFile = FileItem.GetExistingItemByPath(AdditionalLibrary);
							if (ShadowFile != null)
							{
								QueueFileForBatchUpload(ShadowFile);
								//							LinkAction.PrerequisiteItems.Add(ShadowFile);
							}
						}
					}
					else if (Path.GetDirectoryName(AdditionalLibrary) != "" &&
						(Path.GetDirectoryName(AdditionalLibrary).Contains("Binaries/Mac") ||
						Path.GetDirectoryName(AdditionalLibrary).Contains("Binaries\\Mac")))
					{
						// It's an engine or game dylib. Save it for later
						EngineAndGameLibraries.Add(ConvertPath(Path.GetFullPath(AdditionalLibrary)));

						if (!Utils.IsRunningOnMono)
						{
							FileItem EngineLibDependency = FileItem.GetItemByPath(Path.GetFullPath(AdditionalLibrary));
							LinkAction.PrerequisiteItems.Add(EngineLibDependency);
							FileItem RemoteEngineLibDependency = FileItem.GetRemoteItemByPath(ConvertPath(Path.GetFullPath(AdditionalLibrary)), UnrealTargetPlatform.Mac);
							LinkAction.PrerequisiteItems.Add(RemoteEngineLibDependency);
							//Log.TraceInformation("Adding {0} / {1} as a prereq to {2}", EngineLibDependency.AbsolutePath, RemoteEngineLibDependency.AbsolutePath, RemoteOutputFile.AbsolutePath);
						}
						else if (LinkEnvironment.Config.bIsCrossReferenced == false)
						{
							FileItem EngineLibDependency = FileItem.GetItemByPath(Path.GetFullPath(AdditionalLibrary));
							LinkAction.PrerequisiteItems.Add(EngineLibDependency);
						}
					}
					else if (AdditionalLibrary.Contains(".framework/Versions"))
					{
						LinkCommand += string.Format(" " + AdditionalLibrary);
					}
					else
					{
						LinkCommand += string.Format(" -l{0}", AdditionalLibrary);
					}

					if ((AdditionalLibrary.Contains("/Plugins/") || AdditionalLibrary.Contains("/Binaries/ThirdParty/")) && Path.GetDirectoryName(AdditionalLibrary) != Path.GetDirectoryName(AbsolutePath))
					{
						string RelativePath = Utils.MakePathRelativeTo(Path.GetDirectoryName(AdditionalLibrary), Path.GetDirectoryName(AbsolutePath));
						if (!RelativePath.Contains(Path.GetDirectoryName(AdditionalLibrary)) && !RPaths.Contains(RelativePath))
						{
							RPaths.Add(RelativePath);
							LinkCommand += " -rpath \"@loader_path/" + RelativePath + "\"";
						}
					}
				}

				foreach (string AdditionalLibrary in LinkEnvironment.Config.DelayLoadDLLs)
				{
					// Can't link dynamic libraries when creating a static one
					if (bIsBuildingLibrary && (Path.GetExtension(AdditionalLibrary) == ".dylib" || AdditionalLibrary == "z"))
					{
						continue;
					}

					LinkCommand += string.Format(" -weak_library \"{0}\"", ConvertPath(Path.GetFullPath(AdditionalLibrary)));

					if ((AdditionalLibrary.Contains("/Plugins/") || AdditionalLibrary.Contains("/Binaries/ThirdParty/")) && Path.GetDirectoryName(AdditionalLibrary) != Path.GetDirectoryName(AbsolutePath))
					{
						string RelativePath = Utils.MakePathRelativeTo(Path.GetDirectoryName(AdditionalLibrary), Path.GetDirectoryName(AbsolutePath));
						if (!RelativePath.Contains(Path.GetDirectoryName(AdditionalLibrary)) && !RPaths.Contains(RelativePath))
						{
							RPaths.Add(RelativePath);
							LinkCommand += " -rpath \"@loader_path/" + RelativePath + "\"";
						}
					}
				}
			}

			// Add the input files to a response file, and pass the response file on the command-line.
			List<string> InputFileNames = new List<string>();
			foreach (FileItem InputFile in LinkEnvironment.InputFiles)
			{
				InputFileNames.Add(string.Format("\"{0}\"", InputFile.AbsolutePath));
				LinkAction.PrerequisiteItems.Add(InputFile);
			}

			if (bIsBuildingLibrary)
			{
				foreach (string Filename in InputFileNames)
				{
					LinkCommand += " " + Filename;
				}
				// @todo rocket lib: the -filelist command should take a response file (see else condition), except that it just says it can't
				// find the file that's in there. Rocket.lib may overflow the commandline by putting all files on the commandline, so this 
				// may be needed:
				// LinkAction.CommandArguments += string.Format(" -filelist \"{0}\"", ConvertPath(ResponsePath));
			}
			else
			{
				// Write the list of input files to a response file, with a tempfilename, on remote machine
				string ResponsePath = Path.Combine(LinkEnvironment.Config.IntermediateDirectory, Path.GetFileName(OutputFile.AbsolutePath) + ".response");
				
				// Never create response files when we are only generating IntelliSense data
				if (!ProjectFileGenerator.bGenerateProjectFiles)
				{
					ResponseFile.Create(ResponsePath, InputFileNames);

					if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
					{
						RPCUtilHelper.CopyFile(ResponsePath, ConvertPath(ResponsePath), true);
					}
				}

				LinkCommand += string.Format(" @\"{0}\"", ConvertPath(ResponsePath));
			}

			if (LinkEnvironment.Config.bIsBuildingDLL)
			{
				// Add the output file to the command-line.
				LinkCommand += string.Format(" -install_name {0}/{1}", DylibsPath, Path.GetFileName(OutputFile.AbsolutePath));
			}

			if (!bIsBuildingLibrary)
			{
				if (UnrealBuildTool.RunningRocket() || (Utils.IsRunningOnMono && LinkEnvironment.Config.bIsCrossReferenced == false))
				{
					foreach (string Library in EngineAndGameLibraries)
					{
						string LibraryPath = Library;
						if (!File.Exists(Library))
						{
							string LibraryDir = Path.GetDirectoryName(Library);
							string LibraryName = Path.GetFileName(Library);
							string AppBundleName = "UE4Editor";
							if (LibraryName.Contains("UE4Editor-Mac-"))
							{
								string[] Parts = LibraryName.Split('-');
								AppBundleName += "-" + Parts[1] + "-" + Parts[2];
							}
							AppBundleName += ".app";
							LibraryPath = LibraryDir + "/" + AppBundleName + "/Contents/MacOS/" + LibraryName;
							if (!File.Exists(LibraryPath))
							{
								LibraryPath = Library;
							}
						}
						LinkCommand += " \"" + LibraryPath + "\"";
					}
				}
				else
				{
					// Tell linker to ignore unresolved symbols, so we don't have a problem with cross dependent dylibs that do not exist yet.
					// This is fixed in later step, FixDylibDependencies.
					LinkCommand += string.Format(" -undefined dynamic_lookup");
					if (Utils.IsRunningOnMono)
					{
						CrossReferencedFiles.Add( OutputFile );
					}
				}
			}

			// Add the output file to the command-line.
			LinkCommand += string.Format(" -o \"{0}\"", RemoteOutputFile.AbsolutePath);

			// Add the additional arguments specified by the environment.
			LinkCommand += LinkEnvironment.Config.AdditionalArguments;

			if (!bIsBuildingLibrary)
			{
				// Fix the paths for third party libs
				foreach (string Library in ThirdPartyLibraries)
				{
					string LibraryFileName = Path.GetFileName(Library);
					LinkCommand += "; xcrun install_name_tool -change " + LibraryFileName + " " + DylibsPath + "/" + LibraryFileName + " \"" + ConvertPath(OutputFile.AbsolutePath) + "\"";
				}
			}

			LinkAction.CommandArguments = "-c '" + LinkCommand + "'";

			// Only execute linking on the local Mac.
			LinkAction.bCanExecuteRemotely = false;

			LinkAction.StatusDescription = Path.GetFileName(OutputFile.AbsolutePath);
			LinkAction.OutputEventHandler = new DataReceivedEventHandler(RemoteOutputReceivedEventHandler);
			
			LinkAction.ProducedItems.Add(RemoteOutputFile);

			if (!Directory.Exists(LinkEnvironment.Config.IntermediateDirectory))
			{
				return OutputFile;
			}

			if (!bIsBuildingLibrary)
			{
				// Prepare a script that will run later, once every dylibs and the executable are created. This script will be called by action created in FixDylibDependencies()
				string FixDylibDepsScriptPath = Path.Combine(LinkEnvironment.Config.LocalShadowDirectory, "FixDylibDependencies.sh");
				if (!bHasWipedFixDylibScript)
				{
					if (File.Exists(FixDylibDepsScriptPath))
					{
						File.Delete(FixDylibDepsScriptPath);
					}
					bHasWipedFixDylibScript = true;
				}

				if (!Directory.Exists(LinkEnvironment.Config.LocalShadowDirectory))
				{
					Directory.CreateDirectory(LinkEnvironment.Config.LocalShadowDirectory);
				}

				StreamWriter FixDylibDepsScript = File.AppendText(FixDylibDepsScriptPath);

				if (LinkEnvironment.Config.bIsCrossReferenced || !Utils.IsRunningOnMono)
				{
					string EngineAndGameLibrariesString = "";
					foreach (string Library in EngineAndGameLibraries)
					{
						EngineAndGameLibrariesString += " \"" + Library + "\"";
					}
					string FixDylibLine = string.Format("TIMESTAMP=`stat -n -f \"%Sm\" -t \"%Y%m%d%H%M.%S\" \"{0}\"`; ", RemoteOutputFile.AbsolutePath);
					FixDylibLine += LinkCommand.Replace("-undefined dynamic_lookup", EngineAndGameLibrariesString).Replace("$", "\\$");
					FixDylibLine += string.Format("; touch -t $TIMESTAMP \"{0}\"; if [[ $? -ne 0 ]]; then exit 1; fi", RemoteOutputFile.AbsolutePath);
					AppendMacLine(FixDylibDepsScript, FixDylibLine);
				}

				FixDylibDepsScript.Close();

				// Prepare a script that will be called by CreateAppBundle.sh to copy all necessary third party dylibs to the app bundle
				// This is done this way as CreateAppBundle.sh script can be created before all the libraries are processed, so
				// at the time of it's creation we don't have the full list of third party dylibs all modules need.
				string DylibCopyScriptPath = Path.Combine(LinkEnvironment.Config.IntermediateDirectory, "DylibCopy.sh");
				if (!bHasWipedCopyDylibScript)
				{
					if (File.Exists(DylibCopyScriptPath))
					{
						File.Delete(DylibCopyScriptPath);
					}
					bHasWipedCopyDylibScript = true;
				}
				string ExistingScript = File.Exists(DylibCopyScriptPath) ? File.ReadAllText(DylibCopyScriptPath) : "";
				StreamWriter DylibCopyScript = File.AppendText(DylibCopyScriptPath);
				foreach (string Library in ThirdPartyLibraries)
				{
					string CopyCommandLineEntry = string.Format("cp -f \"{0}\" \"$1.app/Contents/MacOS\"", ConvertPath(Path.GetFullPath(Library)).Replace("$", "\\$"));
					if (!ExistingScript.Contains(CopyCommandLineEntry))
					{
						AppendMacLine(DylibCopyScript, CopyCommandLineEntry);
					}
				}
				DylibCopyScript.Close();

				// For non-console application, prepare a script that will create the app bundle. It'll be run by CreateAppBundle action
				if (!LinkEnvironment.Config.bIsBuildingDLL && !LinkEnvironment.Config.bIsBuildingLibrary && !LinkEnvironment.Config.bIsBuildingConsoleApplication)
				{
					string CreateAppBundleScriptPath = Path.Combine(LinkEnvironment.Config.IntermediateDirectory, "CreateAppBundle.sh");
					StreamWriter CreateAppBundleScript = File.CreateText(CreateAppBundleScriptPath);
					AppendMacLine(CreateAppBundleScript, "#!/bin/sh");
					string BinariesPath = Path.GetDirectoryName(OutputFile.AbsolutePath);
					BinariesPath = Path.GetDirectoryName(BinariesPath.Substring(0, BinariesPath.IndexOf(".app")));
					AppendMacLine(CreateAppBundleScript, "cd \"{0}\"", ConvertPath(BinariesPath).Replace("$", "\\$"));

					string ExeName = Path.GetFileName(OutputFile.AbsolutePath);
					string[] ExeNameParts = ExeName.Split('-');
					string GameName = ExeNameParts[0];

					AppendMacLine(CreateAppBundleScript, "mkdir -p \"{0}.app/Contents/MacOS\"", ExeName);
					AppendMacLine(CreateAppBundleScript, "mkdir -p \"{0}.app/Contents/Resources/RadioEffectUnit.component/Contents/MacOS\"", ExeName);
					AppendMacLine(CreateAppBundleScript, "mkdir -p \"{0}.app/Contents/Resources/RadioEffectUnit.component/Contents/Resources/English.lproj\"", ExeName);

					// Copy third party dylibs by calling additional script prepared earlier
					AppendMacLine(CreateAppBundleScript, "sh \"{0}\" \"{1}\"", ConvertPath(DylibCopyScriptPath).Replace("$", "\\$"), ExeName);

					string IconName = "UE4";
					string BundleVersion = ExeName.StartsWith("UnrealEngineLauncher") ? LoadLauncherDisplayVersion() : LoadEngineDisplayVersion();
					string EngineSourcePath = ConvertPath(Directory.GetCurrentDirectory()).Replace("$", "\\$");

					string UProjectFilePath = UProjectInfo.GetProjectFilePath(GameName);
					string CustomResourcesPath = "";
					if (string.IsNullOrEmpty(UProjectFilePath))
					{
						string[] TargetFiles = Directory.GetFiles(Directory.GetCurrentDirectory(), GameName + ".Target.cs", SearchOption.AllDirectories);
						if (TargetFiles.Length == 1)
						{
							CustomResourcesPath = Path.GetDirectoryName(TargetFiles[0]) + "/Resources/Mac";
						}
						else
						{
							Log.TraceWarning("Found {0} Target.cs files for {1}", TargetFiles.Length, GameName);
						}
					}
					else
					{
						CustomResourcesPath = Path.GetDirectoryName(UProjectFilePath) + "/Source/" + GameName + "/Resources/Mac";
					}

					// Copy resources
					string CustomIcon = CustomResourcesPath + "/" + GameName + ".icns";
					if (!File.Exists(CustomIcon))
					{
						CustomIcon = EngineSourcePath + "/Runtime/Launch/Resources/Mac/" + IconName + ".icns";
					}
					AppendMacLine(CreateAppBundleScript, "cp -f \"{0}\" \"{2}.app/Contents/Resources/{1}.icns\"", CustomIcon, IconName, ExeName);

					if (ExeName.StartsWith("UE4Editor"))
					{
					}

					if (ExeName.StartsWith("UE4Editor"))
					{
						AppendMacLine(CreateAppBundleScript, "cp -f \"{0}/Runtime/Launch/Resources/Mac/UProject.icns\" \"{1}.app/Contents/Resources/UProject.icns\"", EngineSourcePath, ExeName);
					}

					string InfoPlistFile = CustomResourcesPath + "/Info.plist";
					if (!File.Exists(InfoPlistFile))
					{
						InfoPlistFile = EngineSourcePath + "/Runtime/Launch/Resources/Mac/" + (GameName.EndsWith("Editor") ? "Info-Editor.plist" : "Info.plist");
					}
					AppendMacLine(CreateAppBundleScript, "cp -f \"{0}\" \"{1}.app/Contents/Info.plist\"", InfoPlistFile, ExeName);

					// Fix contents of Info.plist
					AppendMacLine(CreateAppBundleScript, "sed -i \"\" \"s/\\${0}/{1}/g\" \"{1}.app/Contents/Info.plist\"", "{EXECUTABLE_NAME}", ExeName);
					AppendMacLine(CreateAppBundleScript, "sed -i \"\" \"s/\\${0}/{1}/g\" \"{2}.app/Contents/Info.plist\"", "{APP_NAME}", GameName, ExeName);
					AppendMacLine(CreateAppBundleScript, "sed -i \"\" \"s/\\${0}/{1}/g\" \"{2}.app/Contents/Info.plist\"", "{MACOSX_DEPLOYMENT_TARGET}", MinMacOSVersion, ExeName);
					AppendMacLine(CreateAppBundleScript, "sed -i \"\" \"s/\\${0}/{1}/g\" \"{2}.app/Contents/Info.plist\"", "{ICON_NAME}", IconName, ExeName);
					AppendMacLine(CreateAppBundleScript, "sed -i \"\" \"s/\\${0}/{1}/g\" \"{2}.app/Contents/Info.plist\"", "{BUNDLE_VERSION}", BundleVersion, ExeName);

					// Generate PkgInfo file
					AppendMacLine(CreateAppBundleScript, "echo 'echo -n \"APPL????\"' | bash > \"{0}.app/Contents/PkgInfo\"", ExeName);

					// Copy RadioEffect component
					AppendMacLine(CreateAppBundleScript, "cp -f \"{0}/ThirdParty/Mac/RadioEffectUnit/RadioEffectUnit.component/Contents/MacOS/RadioEffectUnit\" \"{1}.app/Contents/Resources/RadioEffectUnit.component/Contents/MacOS/RadioEffectUnit\"", EngineSourcePath, ExeName);
					AppendMacLine(CreateAppBundleScript, "cp -f \"{0}/ThirdParty/Mac/RadioEffectUnit/RadioEffectUnit.component/Contents/Resources/English.lproj/Localizable.strings\" \"{1}.app/Contents/Resources/RadioEffectUnit.component/Contents/Resources/English.lproj/Localizable.strings\"", EngineSourcePath, ExeName);
					AppendMacLine(CreateAppBundleScript, "cp -f \"{0}/ThirdParty/Mac/RadioEffectUnit/RadioEffectUnit.component/Contents/Info.plist\" \"{1}.app/Contents/Resources/RadioEffectUnit.component/Contents/Info.plist\"", EngineSourcePath, ExeName);

					// Make sure OS X knows the bundle was updated
					AppendMacLine(CreateAppBundleScript, "touch -c \"{0}.app\"", ExeName);

					CreateAppBundleScript.Close();

					// copy over some needed files
					// @todo mac: Make a QueueDirectoryForBatchUpload
					QueueFileForBatchUpload(FileItem.GetItemByFullPath(Path.GetFullPath("../../Engine/Source/Runtime/Launch/Resources/Mac/UE4.icns")));
					QueueFileForBatchUpload(FileItem.GetItemByFullPath(Path.GetFullPath("../../Engine/Source/Runtime/Launch/Resources/Mac/UProject.icns")));
					QueueFileForBatchUpload(FileItem.GetItemByFullPath(Path.GetFullPath("../../Engine/Source/Runtime/Launch/Resources/Mac/Info.plist")));
					QueueFileForBatchUpload(FileItem.GetItemByFullPath(Path.GetFullPath("../../Engine/Source/ThirdParty/Mac/RadioEffectUnit/RadioEffectUnit.component/Contents/MacOS/RadioEffectUnit")));
					QueueFileForBatchUpload(FileItem.GetItemByFullPath(Path.GetFullPath("../../Engine/Source/ThirdParty/Mac/RadioEffectUnit/RadioEffectUnit.component/Contents/Resources/English.lproj/Localizable.strings")));
					QueueFileForBatchUpload(FileItem.GetItemByFullPath(Path.GetFullPath("../../Engine/Source/ThirdParty/Mac/RadioEffectUnit/RadioEffectUnit.component/Contents/Info.plist")));
					QueueFileForBatchUpload(FileItem.GetItemByFullPath(Path.Combine(LinkEnvironment.Config.IntermediateDirectory, "DylibCopy.sh")));
				}
			}

			// For Mac, generate the dSYM file if the config file is set to do so
			if (BuildConfiguration.bGeneratedSYMFile == true)
			{
				Log.TraceInformation("Generating the dSYM file - this will add some time to your build...");
				RemoteOutputFile = GenerateDebugInfo(RemoteOutputFile);
			}

			return RemoteOutputFile;
		}

		FileItem FixDylibDependencies(LinkEnvironment LinkEnvironment, FileItem Executable)
		{
			Action LinkAction = new Action(ActionType.Link);
			LinkAction.WorkingDirectory = Path.GetFullPath(".");
			LinkAction.CommandPath = "/bin/sh";
			LinkAction.CommandDescription = "";

			// Call the FixDylibDependencies.sh script which will link the dylibs and the main executable, this time proper ones, as it's called
			// once all are already created, so the cross dependency problem no longer prevents linking.
			// The script is deleted after it's executed so it's empty when we start appending link commands for the next executable.
			FileItem FixDylibDepsScript = FileItem.GetItemByFullPath(Path.Combine(LinkEnvironment.Config.LocalShadowDirectory, "FixDylibDependencies.sh"));
			FileItem RemoteFixDylibDepsScript = LocalToRemoteFileItem(FixDylibDepsScript, true);

			LinkAction.CommandArguments = "-c 'chmod +x \"" + RemoteFixDylibDepsScript.AbsolutePath + "\"; \"" + RemoteFixDylibDepsScript.AbsolutePath + "\"; if [[ $? -ne 0 ]]; then exit 1; fi; ";

			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				LinkAction.ActionHandler = new Action.BlockingActionHandler(RPCUtilHelper.RPCActionHandler);
			}

			// Make sure this action is executed after all the dylibs and the main executable are created

			foreach (FileItem Dependency in BundleDependencies)
			{
				LinkAction.PrerequisiteItems.Add(Dependency);
			}

			BundleDependencies.Clear();

			LinkAction.StatusDescription = string.Format("Fixing dylib dependencies for {0}", Path.GetFileName(Executable.AbsolutePath));
			LinkAction.bCanExecuteRemotely = false;

			FileItem OutputFile = FileItem.GetItemByPath(Path.Combine(LinkEnvironment.Config.LocalShadowDirectory, Path.GetFileNameWithoutExtension(Executable.AbsolutePath) + ".link"));
			FileItem RemoteOutputFile = LocalToRemoteFileItem(OutputFile, false);

			LinkAction.CommandArguments += "echo \"Dummy\" >> \"" + RemoteOutputFile.AbsolutePath + "\"";
			LinkAction.CommandArguments += "'";

			LinkAction.ProducedItems.Add(RemoteOutputFile);

			return RemoteOutputFile;
		}

		/**
		 * Generates debug info for a given executable
		 * 
		 * @param MachOBinary FileItem describing the executable or dylib to generate debug info for
		 */
		public FileItem GenerateDebugInfo(FileItem MachOBinary)
		{
			// Make a file item for the source and destination files
			string AdditionalExtension = Path.GetExtension(MachOBinary.AbsolutePath) == ".dylib" ? "" : ".app";
			string FullDestPathRoot = MachOBinary.AbsolutePath + AdditionalExtension + ".dSYM";
			string FullDestPath = FullDestPathRoot;
			FileItem OutputFile = FileItem.GetItemByPath(FullDestPath);
			FileItem DestFile = LocalToRemoteFileItem(OutputFile, false);

			// Make the compile action
			Action GenDebugAction = new Action(ActionType.Link);
			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				GenDebugAction.ActionHandler = new Action.BlockingActionHandler(RPCUtilHelper.RPCActionHandler);
			}
			GenDebugAction.WorkingDirectory = Path.GetFullPath(".");
			GenDebugAction.CommandPath = "sh";

			// note that the source and dest are switched from a copy command
			GenDebugAction.CommandArguments = string.Format("-c '{0}usr/bin/xcrun dsymutil {1} -o {2}'",
				DeveloperDir,
				MachOBinary.AbsolutePath,
				FullDestPathRoot,
				Path.GetFileName(MachOBinary.AbsolutePath)+AdditionalExtension);
			GenDebugAction.PrerequisiteItems.Add(MachOBinary);
			GenDebugAction.ProducedItems.Add(DestFile);
			GenDebugAction.StatusDescription = GenDebugAction.CommandArguments;
			GenDebugAction.bCanExecuteRemotely = false;

			return DestFile;
		}

		/**
		 * Creates app bundle for a given executable
		 * 
		 * @param Executable FileItem describing the executable to generate app bundle for
		 */
		FileItem CreateAppBundle(LinkEnvironment LinkEnvironment, FileItem Executable, FileItem FixDylibOutputFile)
		{
			// Make a file item for the source and destination files
			string FullDestPath = Executable.AbsolutePath.Substring(0, Executable.AbsolutePath.IndexOf(".app") + 4);
			FileItem DestFile = FileItem.GetItemByPath(FullDestPath);
			FileItem RemoteDestFile = LocalToRemoteFileItem(DestFile, false);

			// Make the compile action
			Action CreateAppBundleAction = new Action(ActionType.CreateAppBundle);
			CreateAppBundleAction.WorkingDirectory = Path.GetFullPath(".");
			CreateAppBundleAction.CommandPath = "/bin/sh";
			CreateAppBundleAction.CommandDescription = "";

			// make path to the script
			FileItem BundleScript = FileItem.GetItemByFullPath(Path.Combine(LinkEnvironment.Config.IntermediateDirectory, "CreateAppBundle.sh"));
			FileItem RemoteBundleScript = LocalToRemoteFileItem(BundleScript, true);

		
			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				CreateAppBundleAction.ActionHandler = new Action.BlockingActionHandler(RPCUtilHelper.RPCActionHandler);
			}

			CreateAppBundleAction.CommandArguments = "\"" + RemoteBundleScript.AbsolutePath + "\"";
			CreateAppBundleAction.PrerequisiteItems.Add(FixDylibOutputFile);
			CreateAppBundleAction.ProducedItems.Add(RemoteDestFile);
			CreateAppBundleAction.StatusDescription = string.Format("Creating app bundle: {0}.app", Path.GetFileName(Executable.AbsolutePath));
			CreateAppBundleAction.bCanExecuteRemotely = false;

			return RemoteDestFile;
		}

		static public void SetupBundleDependencies(List<UEBuildBinary> Binaries, string GameName)
		{
			foreach (UEBuildBinary Binary in Binaries)
			{
				BundleDependencies.Add(FileItem.GetItemByPath(Binary.ToString()));
			}
		}
		
		static public void FixBundleBinariesPaths(UEBuildTarget Target, List<UEBuildBinary> Binaries)
		{
			string BundleContentsPath = Target.OutputPath + ".app/Contents/";
			foreach (UEBuildBinary Binary in Binaries)
			{
				string BinaryFileName = Path.GetFileName(Binary.Config.OutputFilePath);
				if (BinaryFileName.EndsWith(".dylib"))
				{
					// Only dylibs from the same folder as the executable should be moved to the bundle. UE4Editor-*Game* dylibs and plugins will be loaded
					// from their Binaries/Mac folders.
					string DylibDir = Path.GetDirectoryName(Path.GetFullPath(Binary.Config.OutputFilePath));
					string ExeDir = Path.GetDirectoryName(Path.GetFullPath(Target.OutputPath));
					if (DylibDir.StartsWith(ExeDir))
					{
						// get the subdir, which is the DylibDir - ExeDir
						// @todo: This is commented out because the code in ApplePlatformFile.cpp for finding frameworks doesn't handle subdirectories
						// at all yet, and it doesn't seem that the subdirectory actually matters in the Mac case, because the JunkManifest doesn't
						// run over every single .app directory to delete them anyway - and the platforms Mac supports aren't the type to need that
						// kind of protection to be deleted if they aren't supported for a current user that has the dylibs locally.
						// string SubDir = DylibDir.Replace(ExeDir, "");
						string SubDir = "";
						Binary.Config.OutputFilePath = BundleContentsPath + "MacOS" + SubDir + "/" + BinaryFileName;
					}
				}
				else if (!BinaryFileName.EndsWith(".a"))
				{
					Binary.Config.OutputFilePath += ".app/Contents/MacOS/" + BinaryFileName;
				}
			}
		}

		static private string BundleContentsDirectory = "";

		static public void AddAppBundleContentsToManifest(ref FileManifest Manifest, UEBuildBinary Binary)
		{
			if (Binary.Target.GlobalLinkEnvironment.Config.bIsBuildingConsoleApplication)
			{
				return;
			}

			if (BundleContentsDirectory == "" && Binary.Config.Type == UEBuildBinaryType.Executable)
			{
				BundleContentsDirectory = Path.GetDirectoryName(Path.GetDirectoryName(Binary.Config.OutputFilePath)) + "/";
			}

			// We need to know what third party dylibs would be copied to the bundle
			var Modules = Binary.GetAllDependencyModules(bIncludeDynamicallyLoaded: false, bForceCircular: false);
			var BinaryLinkEnvironment = Binary.Target.GlobalLinkEnvironment.DeepCopy();
			var BinaryDependencies = new List<UEBuildBinary>();
			var LinkEnvironmentVisitedModules = new Dictionary<UEBuildModule, bool>();
			foreach (var Module in Modules)
			{
				Module.SetupPrivateLinkEnvironment(ref BinaryLinkEnvironment, ref BinaryDependencies, ref LinkEnvironmentVisitedModules);
			}

			foreach (string AdditionalLibrary in BinaryLinkEnvironment.Config.AdditionalLibraries)
			{
				string LibName = Path.GetFileName(AdditionalLibrary);
				if (LibName.StartsWith("lib"))
				{
					if (Path.GetExtension(AdditionalLibrary) == ".dylib")
					{
						string Entry = BundleContentsDirectory + "MacOS/" + LibName;
						if (!Manifest.FileManifestItems.Contains(Path.GetFullPath(Entry)))
						{
							Manifest.AddFileName(Entry);
						}
					}
				}
			}

			if (Binary.Config.Type == UEBuildBinaryType.Executable)
			{
				// And we also need all the resources
				Manifest.AddFileName(BundleContentsDirectory + "Info.plist");
				Manifest.AddFileName(BundleContentsDirectory + "PkgInfo");
				Manifest.AddFileName(BundleContentsDirectory + "Resources/UE4.icns");
				Manifest.AddFileName(BundleContentsDirectory + "Resources/RadioEffectUnit.component/Contents/MacOS/RadioEffectUnit");
				Manifest.AddFileName(BundleContentsDirectory + "Resources/RadioEffectUnit.component/Contents/Resources/English.lproj/Localizable.strings");
				Manifest.AddFileName(BundleContentsDirectory + "Resources/RadioEffectUnit.component/Contents/Info.plist");

				if (Binary.Config.TargetName.StartsWith("UE4Editor"))
				{
					Manifest.AddFileName(BundleContentsDirectory + "Resources/UProject.icns");
				}
			}
		}

		// @todo Mac: Full implementation.
		public override void CompileCSharpProject(CSharpEnvironment CompileEnvironment, string ProjectFileName, string DestinationFile)
		{
			string ProjectDirectory = Path.GetDirectoryName(Path.GetFullPath(ProjectFileName));

			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				RPCUtilHelper.CopyFile(ProjectFileName, ConvertPath(ProjectFileName), true);
				RPCUtilHelper.CopyFile("Engine/Source/Programs/DotNETCommon/MetaData.cs", ConvertPath("Engine/Source/Programs/DotNETCommon/MetaData.cs"), true);

				string[] FileList = Directory.GetFiles(ProjectDirectory, "*.cs", SearchOption.AllDirectories);
				foreach (string File in FileList)
				{
					RPCUtilHelper.CopyFile(File, ConvertPath(File), true);
				}
			}

			string XBuildArgs = "/verbosity:quiet /nologo /target:Rebuild /property:Configuration=Development /property:Platform=AnyCPU " + Path.GetFileName(ProjectFileName) + " /property:TargetFrameworkVersion=v4.5 /tv:4.0";

			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				RPCUtilHelper.Command(ConvertPath(ProjectDirectory), "xbuild", XBuildArgs, null);
			}
			else
			{
				Process XBuildProcess = new Process();
				XBuildProcess.StartInfo.WorkingDirectory = ProjectDirectory;
				XBuildProcess.StartInfo.FileName = "sh";
				XBuildProcess.StartInfo.Arguments = "-c 'xbuild " + XBuildArgs + " |grep -i error; if [ $? -ne 1 ]; then exit 1; else exit 0; fi'";
				XBuildProcess.OutputDataReceived += new DataReceivedEventHandler(OutputReceivedDataEventHandler);
				XBuildProcess.ErrorDataReceived += new DataReceivedEventHandler(OutputReceivedDataEventHandler);
				Utils.RunLocalProcess(XBuildProcess);
			}
		}

		public override void PreBuildSync()
		{
			BuiltBinaries = new List<string>();

			base.PreBuildSync();
		}

		public override void PostBuildSync(UEBuildTarget Target)
		{
			foreach (UEBuildBinary Binary in Target.AppBinaries)
			{
				BuiltBinaries.Add(Path.GetFullPath(Binary.ToString()));
			}

			base.PostBuildSync(Target);
		}

		public override ICollection<FileItem> PostBuild (FileItem Executable, LinkEnvironment BinaryLinkEnvironment)
		{
			var OutputFiles = base.PostBuild (Executable, BinaryLinkEnvironment);

			// if building for Mac on a Mac, use actions to finalize the builds (otherwise, we use Deploy)
			if (ExternalExecution.GetRuntimePlatform () != UnrealTargetPlatform.Mac) {
				return OutputFiles;
			}

			if (BinaryLinkEnvironment.Config.bIsBuildingDLL || BinaryLinkEnvironment.Config.bIsBuildingLibrary) {
				return OutputFiles;
			}
			FileItem FixDylibOutputFile = FixDylibDependencies(BinaryLinkEnvironment, Executable);
			OutputFiles.Add(FixDylibOutputFile);
			if (BinaryLinkEnvironment.Config.bIsBuildingConsoleApplication)
				return OutputFiles;

			OutputFiles.Add(CreateAppBundle(BinaryLinkEnvironment, Executable, FixDylibOutputFile));
			return OutputFiles;
		}
	};
}
