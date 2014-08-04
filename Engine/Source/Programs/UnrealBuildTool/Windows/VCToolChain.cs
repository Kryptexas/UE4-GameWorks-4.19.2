// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;
using System.Text;

namespace UnrealBuildTool
{
	public class VCToolChain : UEToolChain
	{
		public override void RegisterToolChain()
		{
			// Register this tool chain for both Win64 and Win32
			Log.TraceVerbose("        Registered for {0}", CPPTargetPlatform.Win64.ToString());
			UEToolChain.RegisterPlatformToolChain(CPPTargetPlatform.Win64, this);
			Log.TraceVerbose("        Registered for {0}", CPPTargetPlatform.Win32.ToString());
			UEToolChain.RegisterPlatformToolChain(CPPTargetPlatform.Win32, this);
		}

		static void AppendCLArguments_Global(CPPEnvironment CompileEnvironment, StringBuilder Arguments)
		{
			// @todo fastubt: Use should be using StringBuilder instead of concatenation in all of these toolchain files.  On Mono, string builder is dramatically faster.


			// @todo fastubt: We can use '/showIncludes' to accelerate outdatedness checking, as the compiler will discover all indirect includes itself.  But, the spew is pretty noisy!
			//		-> If no files in source file chain have changed, even if the build product is outdated, we can skip using /showIncludes (relies on cache surviving)
			// Result += " /showIncludes";

			if( WindowsPlatform.bCompileWithClang )
			{ 
				// Result += " -###";	// @todo clang: Print Clang command-lines (instead of outputting compile results!)

				// @todo clang: We're impersonating the Visual C++ compiler by setting MSC_VER and _MSC_FULL_VER to values that MSVC would set
				string VersionString;
				switch( WindowsPlatform.Compiler )
				{
					case WindowsCompiler.VisualStudio2012:
						VersionString = "1700";
						break;
		
					case WindowsCompiler.VisualStudio2013:
						VersionString = "1800";
						break;

					default:
						throw new BuildException( "Unexpected value for WindowsPlatform.Compiler: " + WindowsPlatform.Compiler.ToString() );
				}
				Arguments.Append(" -fmsc-version=" + VersionString);
				Arguments.Append(" /D_MSC_FULL_VER=" + VersionString + "00000");
			}


			if( BuildConfiguration.bEnableCodeAnalysis )
			{
				Arguments.Append(" /analyze");

				// Don't cause analyze warnings to be errors
				Arguments.Append(" /analyze:WX-");

				// Report functions that use a LOT of stack space.  You can lower this value if you
				// want more aggressive checking for functions that use a lot of stack memory.
				Arguments.Append(" /analyze:stacksize81940");

				// Don't bother generating code, only analyze code (may report fewer warnings though.)
				//Arguments.Append(" /analyze:only");
			}

			// Prevents the compiler from displaying its logo for each invocation.
			Arguments.Append(" /nologo");

			// Enable intrinsic functions.
			Arguments.Append(" /Oi");

			// Enable for static code analysis (where supported). Not treating analysis warnings as errors.
			// Arguments.Append(" /analyze:WX-");
			
			if( !WindowsPlatform.bCompileWithClang )	// @todo clang: These options are not supported with clang-cl yet
			{
				if (CompileEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64)
				{
					// Pack struct members on 8-byte boundaries.
					Arguments.Append(" /Zp8");
				}
				else
				{
					// Pack struct members on 4-byte boundaries.
					Arguments.Append(" /Zp4");
				}

				// Separate functions for linker.
				Arguments.Append(" /Gy");

				// Relaxes floating point precision semantics to allow more optimization.
				Arguments.Append(" /fp:fast");
			}

			// Compile into an .obj file, and skip linking.
			Arguments.Append(" /c");

			// Allow 800% of the default memory allocation limit.
			Arguments.Append(" /Zm800");

			if( !WindowsPlatform.bCompileWithClang )	// @todo clang: Not supported in clang-cl yet
			{
				// Allow large object files to avoid hitting the 2^16 section limit when running with -StressTestUnity.
				Arguments.Append(" /bigobj");
			}

			// Disable "The file contains a character that cannot be represented in the current code page" warning for non-US windows.
			Arguments.Append(" /wd4819");

			if( BuildConfiguration.bUseSharedPCHs )
			{
				// @todo SharedPCH: Disable warning about PCH defines not matching .cpp defines.  We "cheat" these defines a little
				// bit to make shared PCHs work.  But it's totally safe.  Trust us.
				Arguments.Append(" /wd4651");

				// @todo SharedPCH: Disable warning about redefining *API macros.  The PCH header is compiled with various DLLIMPORTs, but
				// when a module that uses that PCH header *IS* one of those imports, that module is compiled with EXPORTS, so the macro
				// is redefined on the command-line.  We need to clobber those defines to make shared PCHs work properly!
				Arguments.Append(" /wd4005");
			}

			// If compiling as a DLL, set the relevant defines
			if (CompileEnvironment.Config.bIsBuildingDLL)
			{
				Arguments.Append(" /D _WINDLL");
			}

			// When targeting Windows XP with Visual Studio 2012+, we need to tell the compiler to use the older Windows SDK that works
			// with Windows XP (http://blogs.msdn.com/b/vcblog/archive/2012/10/08/10357555.aspx)
			if( WindowsPlatform.SupportWindowsXP )
			{
				if( WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013 )
				{
					Arguments.Append(" /D_USING_V120_SDK71_");
				}
				else if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2012)
				{
					Arguments.Append(" /D_USING_V110_SDK71_");
				}
			}


			// Handle Common Language Runtime support (C++/CLI)
			if (CompileEnvironment.Config.CLRMode == CPPCLRMode.CLREnabled)
			{
				Arguments.Append(" /clr");

				// Don't use default lib path, we override it explicitly to use the 4.0 reference assemblies.
				Arguments.Append(" /clr:nostdlib");
			}

			//
			//	Debug
			//
			if (CompileEnvironment.Config.Target.Configuration == CPPTargetConfiguration.Debug)
			{
				// Disable compiler optimization.
				Arguments.Append(" /Od");

				// Favor code size (especially useful for embedded platforms).
				Arguments.Append(" /Os");

				// Allow inline method expansion unless E&C support is requested
				if( !BuildConfiguration.bSupportEditAndContinue )
				{
					Arguments.Append(" /Ob2");
				}

				if ((CompileEnvironment.Config.Target.Platform == CPPTargetPlatform.Win32) || 
					(CompileEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64))
				{
					// Runtime stack checks are not allowed when compiling for CLR
					if (CompileEnvironment.Config.CLRMode == CPPCLRMode.CLRDisabled)
					{
						Arguments.Append(" /RTCs");
					}
				}
			}
			//
			//	Development and LTCG
			//
			else
			{
				// Maximum optimizations if desired.
				if( CompileEnvironment.Config.OptimizeCode >= ModuleRules.CodeOptimization.InNonDebugBuilds )
				{
					Arguments.Append(" /Ox");

					// Allow optimized code to be debugged more easily.  This makes PDBs a bit larger, but doesn't noticeably affect
					// compile times.  The executable code is not affected at all by this switch, only the debugging information.
					Arguments.Append(" /d2Zi+");
				}
				
				// Favor code speed.
				Arguments.Append(" /Ot");

				// Only omit frame pointers on the PC (which is implied by /Ox) if wanted.
				if ( BuildConfiguration.bOmitFramePointers == false
				&& ((CompileEnvironment.Config.Target.Platform == CPPTargetPlatform.Win32) ||
					(CompileEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64)))
				{
					Arguments.Append(" /Oy-");
				}

				// Allow inline method expansion
				Arguments.Append(" /Ob2");

				//
				// LTCG
				//
				if (CompileEnvironment.Config.Target.Configuration == CPPTargetConfiguration.Shipping)
				{
					if( BuildConfiguration.bAllowLTCG )
					{
						// Enable link-time code generation.
						Arguments.Append(" /GL");
					}
				}
			}

			//
			//	PC
			//
			if ((CompileEnvironment.Config.Target.Platform == CPPTargetPlatform.Win32) || 
				(CompileEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64))
			{
				// SSE options are not allowed when using CLR compilation or the 64 bit toolchain
				// (both enable SSE2 automatically)
				if (CompileEnvironment.Config.CLRMode == CPPCLRMode.CLRDisabled &&
					CompileEnvironment.Config.Target.Platform != CPPTargetPlatform.Win64)
				{
					// @todo Clang: Doesn't make sense to the Clang compiler
					if( !WindowsPlatform.bCompileWithClang )
					{
						// Allow the compiler to generate SSE2 instructions.
						Arguments.Append(" /arch:SSE2");
					}
				}

				// Prompt the user before reporting internal errors to Microsoft.
				Arguments.Append(" /errorReport:prompt");

				if (CompileEnvironment.Config.CLRMode == CPPCLRMode.CLRDisabled)
				{
					// Enable C++ exceptions when building with the editor or when building UHT.
					if ((CompileEnvironment.Config.bEnableExceptions || UEBuildConfiguration.bBuildEditor || UEBuildConfiguration.bForceEnableExceptions)
						&& !WindowsPlatform.bCompileWithClang)	// @todo clang: Clang compiler frontend doesn't understand /EHsc yet
					{
						// Enable C++ exception handling, but not C exceptions.
						Arguments.Append(" /EHsc");
					}
					else
					{
						// This is required to disable exception handling in VC platform headers.
						CompileEnvironment.Config.Definitions.Add("_HAS_EXCEPTIONS=0");
					}
				}
				else
				{
					// For C++/CLI all exceptions must be left enabled
					Arguments.Append(" /EHa");
				}
            }

            // @todo - find a better place for this. 
            if (CompileEnvironment.Config.Target.Platform == CPPTargetPlatform.HTML5)
            {
                Arguments.Append(" /EHsc"); 
            }
            // If enabled, create debug information.
			if (CompileEnvironment.Config.bCreateDebugInfo)
			{
				if( WindowsPlatform.bCompileWithClang )	// @todo clang: /Zx options are not supported in clang-cl yet
				{
					// Manually enable full debug info.  This is basically DWARF3 debug data.  MSVC won't know what to do with this and
					// will just strip it out of executables linked using the MSVC linker.
					Arguments.Append(" -Xclang -g");
				}
				else
				{
					// Store debug info in .pdb files.
					if( BuildConfiguration.bUsePDBFiles )
					{
						// Create debug info suitable for E&C if wanted.
						if( BuildConfiguration.bSupportEditAndContinue &&
							// We only need to do this in debug as that's the only configuration that supports E&C.
							CompileEnvironment.Config.Target.Configuration == CPPTargetConfiguration.Debug)
						{
							Arguments.Append(" /ZI");
						}
						// Regular PDB debug information.
						else
						{
							Arguments.Append(" /Zi");
						}
                        // We need to add this so VS won't lock the PDB file and prevent synchronous updates. This forces serialization through MSPDBSRV.exe.
                        // See http://msdn.microsoft.com/en-us/library/dn502518.aspx for deeper discussion of /FS switch.
                        if (BuildConfiguration.bUseIncrementalLinking)
                        {
                            Arguments.Append(" /FS");
                        }
					}
					// Store C7-format debug info in the .obj files, which is faster.
					else
					{
						Arguments.Append(" /Z7");
					}
				}
			}

			// Specify the appropriate runtime library based on the platform and config.
			if( CompileEnvironment.Config.Target.Configuration == CPPTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT )
			{
				Arguments.Append(" /MDd");
			}
			else
			{
				Arguments.Append(" /MD");
			}
		}

		static void AppendCLArguments_CPP( CPPEnvironment CompileEnvironment, StringBuilder Arguments )
		{
			// Explicitly compile the file as C++.
			Arguments.Append(" /TP");

			if (!CompileEnvironment.Config.bEnableBufferSecurityChecks)
			{
				// This will disable buffer security checks (which are enabled by default) that the MS compiler adds around arrays on the stack,
				// Which can add some performance overhead, especially in performance intensive code
				// Only disable this if you know what you are doing, because it will be disabled for the entire module!
				Arguments.Append(" /GS-");
			}

			// C++/CLI requires that RTTI is left enabled
			if (CompileEnvironment.Config.CLRMode == CPPCLRMode.CLRDisabled)
			{
				if (CompileEnvironment.Config.bUseRTTI)
				{
					// Enable C++ RTTI.
					Arguments.Append(" /GR");
				}
				else
				{
					// Disable C++ RTTI.
					Arguments.Append(" /GR-");
				}
			}

			// Level 4 warnings.
			Arguments.Append(" /W4");

			if( WindowsPlatform.bCompileWithClang )
			{
				// Disable specific warnings that cause problems with Clang
				// NOTE: These must appear after we set the MSVC warning level

				// Allow Microsoft-specific syntax to slide, even though it may be non-standard.  Needed for Windows headers.
				Arguments.Append(" -Wno-microsoft");								

				// @todo clang: Kind of a shame to turn these off.  We'd like to catch unused variables, but it is tricky with how our assertion macros work.
				Arguments.Append(" -Wno-unused-variable");
				Arguments.Append(" -Wno-unused-function");
				Arguments.Append(" -Wno-unused-private-field");
				Arguments.Append(" -Wno-unused-value");

				Arguments.Append(" -Wno-inline-new-delete");	// @todo clang: We declare operator new as inline.  Clang doesn't seem to like that.

				// @todo clang: Disabled warnings were copied from MacToolChain for the most part
				Arguments.Append(" -Wno-deprecated-declarations");
				Arguments.Append(" -Wno-deprecated-writable-strings");
				Arguments.Append(" -Wno-deprecated-register");
				Arguments.Append(" -Wno-switch-enum");
				Arguments.Append(" -Wno-logical-op-parentheses");	// needed for external headers we shan't change
				Arguments.Append(" -Wno-null-arithmetic");			// needed for external headers we shan't change
				Arguments.Append(" -Wno-deprecated-declarations");	// needed for wxWidgets
				Arguments.Append(" -Wno-return-type-c-linkage");	// needed for PhysX
				Arguments.Append(" -Wno-ignored-attributes");		// needed for nvtesslib
				Arguments.Append(" -Wno-uninitialized");
				Arguments.Append(" -Wno-tautological-compare");
				Arguments.Append(" -Wno-switch");
				Arguments.Append(" -Wno-invalid-offsetof"); // needed to suppress warnings about using offsetof on non-POD types.
			}
		}
		
		static void AppendCLArguments_C(StringBuilder Arguments)
		{
			// Explicitly compile the file as C.
			Arguments.Append(" /TC");
		
			// Level 0 warnings.  Needed for external C projects that produce warnings at higher warning levels.
			Arguments.Append(" /W0");
		}

		static void AppendLinkArguments(LinkEnvironment LinkEnvironment, StringBuilder Arguments)
		{
			// Don't create a side-by-side manifest file for the executable.
			Arguments.Append(" /MANIFEST:NO");

			// Prevents the linker from displaying its logo for each invocation.
			Arguments.Append(" /NOLOGO");

			if (LinkEnvironment.Config.bCreateDebugInfo)
			{
				// Output debug info for the linked executable.
				Arguments.Append(" /DEBUG");
			}

			// Prompt the user before reporting internal errors to Microsoft.
			Arguments.Append(" /errorReport:prompt");

			//
			//	PC
			//
			if ((LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win32) || 
				(LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64))
			{
				// Set machine type/ architecture to be 64 bit.
				if (LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64)
				{
					Arguments.Append(" /MACHINE:x64");
				}
				// 32 bit executable/ target.
				else
				{
					Arguments.Append(" /MACHINE:x86");
				}

				{
					if (LinkEnvironment.Config.bIsBuildingConsoleApplication)
					{
						Arguments.Append(" /SUBSYSTEM:CONSOLE");
					}
					else
					{
						Arguments.Append(" /SUBSYSTEM:WINDOWS");
					}

					// When targeting Windows XP in Visual Studio 2012+, we need to tell the linker we are going to support execution
					// on that older platform.  The compiler defaults to version 6.0+.  We'll modify the SUBSYSTEM parameter here.
					if( WindowsPlatform.SupportWindowsXP )
					{
						Arguments.Append(LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64 ? ",5.02" : ",5.01");
					}
				}

				if (LinkEnvironment.Config.bIsBuildingConsoleApplication && !LinkEnvironment.Config.bIsBuildingDLL && !String.IsNullOrEmpty( LinkEnvironment.Config.WindowsEntryPointOverride ) )
				{
					// Use overridden entry point
					Arguments.Append(" /ENTRY:" + LinkEnvironment.Config.WindowsEntryPointOverride);
				}

				// Allow the OS to load the EXE at different base addresses than its preferred base address.
				Arguments.Append(" /FIXED:No");

				// Option is only relevant with 32 bit toolchain.
				if (LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win32)
				{
					// Disables the 2GB address space limit on 64-bit Windows and 32-bit Windows with /3GB specified in boot.ini
					Arguments.Append(" /LARGEADDRESSAWARE");
				}

				// Explicitly declare that the executable is compatible with Data Execution Prevention.
				Arguments.Append(" /NXCOMPAT");

				// Set the default stack size.
				Arguments.Append(" /STACK:5000000");

				// E&C can't use /SAFESEH.  Also, /SAFESEH isn't compatible with 64-bit linking
				if( !BuildConfiguration.bSupportEditAndContinue &&
					LinkEnvironment.Config.Target.Platform != CPPTargetPlatform.Win64)
				{
					// Generates a table of Safe Exception Handlers.  Documentation isn't clear whether they actually mean
					// Structured Exception Handlers.
					Arguments.Append(" /SAFESEH");
				}

				// Allow delay-loaded DLLs to be explicitly unloaded.
				Arguments.Append(" /DELAY:UNLOAD");

				if (LinkEnvironment.Config.bIsBuildingDLL)
				{
					Arguments.Append(" /DLL");
				}

				if (LinkEnvironment.Config.CLRMode == CPPCLRMode.CLREnabled)
				{
					// DLLs built with managed code aren't allowed to have entry points as they will try to initialize
					// complex static variables.  Managed code isn't allowed to run during DLLMain, we can't allow
					// these variables to be initialized here.
					if (LinkEnvironment.Config.bIsBuildingDLL)
					{
						// NOTE: This appears to only be needed if we want to get call stacks for debugging exit crashes related to the above
						//		Result += " /NOENTRY /NODEFAULTLIB:nochkclr.obj";
					}
				}
			}

			// Don't embed the full PDB path in the binary when building Rocket executables; the location on disk won't match the user's install directory.
			if(UnrealBuildTool.BuildingRocket())
			{
				Arguments.Append(" /PDBALTPATH:%_PDB%");
			}

			//
			//	Shipping & LTCG
			//
			if( BuildConfiguration.bAllowLTCG &&
				LinkEnvironment.Config.Target.Configuration == CPPTargetConfiguration.Shipping)
			{
				// Use link-time code generation.
				Arguments.Append(" /LTCG");

				// This is where we add in the PGO-Lite linkorder.txt if we are using PGO-Lite
				//Result += " /ORDER:@linkorder.txt";
				//Result += " /VERBOSE";
			}

			//
			//	Shipping binary
			//
			if (LinkEnvironment.Config.Target.Configuration == CPPTargetConfiguration.Shipping)
			{
				// Generate an EXE checksum.
				Arguments.Append(" /RELEASE");

				// Eliminate unreferenced symbols.
				Arguments.Append(" /OPT:REF");

				// Remove redundant COMDATs.
				Arguments.Append(" /OPT:ICF");
			}
			//
			//	Regular development binary. 
			//
			else
			{
				// Keep symbols that are unreferenced.
				Arguments.Append(" /OPT:NOREF");

				// Disable identical COMDAT folding.
				Arguments.Append(" /OPT:NOICF");
			}

			// Enable incremental linking if wanted.
			// NOTE: Don't bother using incremental linking for C++/CLI projects, as that's not supported and the option
			//		 will silently be ignored anyway
			if (BuildConfiguration.bUseIncrementalLinking && LinkEnvironment.Config.CLRMode != CPPCLRMode.CLREnabled)
			{
				Arguments.Append(" /INCREMENTAL");
			}
			else
			{
				Arguments.Append(" /INCREMENTAL:NO");
			}

			// Disable 
			//LINK : warning LNK4199: /DELAYLOAD:nvtt_64.dll ignored; no imports found from nvtt_64.dll
			// type warning as we leverage the DelayLoad option to put third-party DLLs into a 
			// non-standard location. This requires the module(s) that use said DLL to ensure that it 
			// is loaded prior to using it.
			Arguments.Append(" /ignore:4199");

			// Suppress warnings about missing PDB files for statically linked libraries.  We often don't want to distribute
			// PDB files for these libraries.
			Arguments.Append(" /ignore:4099");		// warning LNK4099: PDB '<file>' was not found with '<file>'
		}

		static void AppendLibArguments(LinkEnvironment LinkEnvironment, StringBuilder Arguments)
		{
			// Prevents the linker from displaying its logo for each invocation.
			Arguments.Append(" /NOLOGO");

			// Prompt the user before reporting internal errors to Microsoft.
			Arguments.Append(" /errorReport:prompt");

			//
			//	PC
			//
			if (LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win32 || LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64)
			{
				// Set machine type/ architecture to be 64 bit.
				if (LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64)
				{
					Arguments.Append(" /MACHINE:x64");
				}
				// 32 bit executable/ target.
				else
				{
					Arguments.Append(" /MACHINE:x86");
				}

				{
					if (LinkEnvironment.Config.bIsBuildingConsoleApplication)
					{
						Arguments.Append(" /SUBSYSTEM:CONSOLE");
					}
					else
					{
						Arguments.Append(" /SUBSYSTEM:WINDOWS");
					}

					// When targeting Windows XP in Visual Studio 2012+, we need to tell the linker we are going to support execution
					// on that older platform.  The compiler defaults to version 6.0+.  We'll modify the SUBSYSTEM parameter here.
					if( WindowsPlatform.SupportWindowsXP )
					{
						Arguments.Append(LinkEnvironment.Config.Target.Platform == CPPTargetPlatform.Win64 ? ",5.02" : ",5.01");
					}
				}
			}

			//
			//	Shipping & LTCG
			//
			if (LinkEnvironment.Config.Target.Configuration == CPPTargetConfiguration.Shipping)
			{
				// Use link-time code generation.
				Arguments.Append(" /LTCG");
			}
		}

		public override CPPOutput CompileCPPFiles(UEBuildTarget Target, CPPEnvironment CompileEnvironment, List<FileItem> SourceFiles, string ModuleName)
		{
			StringBuilder Arguments = new StringBuilder();
			AppendCLArguments_Global(CompileEnvironment, Arguments);

			// Add include paths to the argument list.
			foreach (string IncludePath in CompileEnvironment.Config.IncludePaths)
			{
				Arguments.AppendFormat(" /I \"{0}\"", IncludePath);
			}
			foreach (string IncludePath in CompileEnvironment.Config.SystemIncludePaths)
			{
				if( WindowsPlatform.bCompileWithClang )
				{
					// @todo Clang: Clang uses a special command-line syntax for system headers.  This is used for two reasons.  The first is that Clang will automatically 
					// suppress compiler warnings in headers found in these directories, such as the DirectX SDK headers.  The other reason this is important is in the case 
					// where there the same header include path is passed as both a regular include path and a system include path (extracted from INCLUDE environment).  In 
					// this case Clang will ignore any earlier occurrence of the include path, preventing a system header include path from overriding a different system 
					// include path set later on by a module.  NOTE: When passing "-Xclang", these options will always appear at the end of the command-line string, meaning
					// they will be forced to appear *after* all environment-variable-extracted includes.  This is technically okay though.
					Arguments.AppendFormat(" -Xclang -internal-isystem -Xclang \"{0}\"", IncludePath);
				}
				else
				{
					Arguments.AppendFormat(" /I \"{0}\"", IncludePath);
				}
			}


			if (CompileEnvironment.Config.CLRMode == CPPCLRMode.CLREnabled)
			{
				// Add .NET framework assembly paths.  This is needed so that C++/CLI projects
				// can reference assemblies with #using, without having to hard code a path in the
				// .cpp file to the assembly's location.				
				foreach (string AssemblyPath in CompileEnvironment.Config.SystemDotNetAssemblyPaths)
				{
					Arguments.AppendFormat(" /AI \"{0}\"", AssemblyPath);
				}

				// Add explicit .NET framework assembly references				
				foreach (string AssemblyName in CompileEnvironment.Config.FrameworkAssemblyDependencies)
				{
					Arguments.AppendFormat( " /FU \"{0}\"", AssemblyName );
				}

				// Add private assembly references				
				foreach( PrivateAssemblyInfo CurAssemblyInfo in CompileEnvironment.PrivateAssemblyDependencies )
				{
					Arguments.AppendFormat( " /FU \"{0}\"", CurAssemblyInfo.FileItem.AbsolutePath );
				}
			}


			// Add preprocessor definitions to the argument list.
			foreach (string Definition in CompileEnvironment.Config.Definitions)
			{
				// Escape all quotation marks so that they get properly passed with the command line.
				var DefinitionArgument = Definition.Contains("\"") ? Definition.Replace("\"", "\\\"") : Definition;
				Arguments.AppendFormat(" /D \"{0}\"", DefinitionArgument);
			}

			var BuildPlatform = UEBuildPlatform.GetBuildPlatformForCPPTargetPlatform(CompileEnvironment.Config.Target.Platform);

			// Create a compile action for each source file.
			CPPOutput Result = new CPPOutput();
			foreach (FileItem SourceFile in SourceFiles)
			{
				Action CompileAction = new Action(ActionType.Compile);
				StringBuilder FileArguments = new StringBuilder();
				bool bIsPlainCFile = Path.GetExtension(SourceFile.AbsolutePath).ToUpperInvariant() == ".C";

				// Add the C++ source file and its included files to the prerequisite item list.
				CompileAction.PrerequisiteItems.Add(SourceFile);
				{
					// @todo fastubt: What if one of the prerequisite files has become missing since it was updated in our cache? (usually, because a coder eliminated the source file)
					//		-> Two CASES:
					//				1) NOT WORKING: Non-unity file went away (SourceFile in this context).  That seems like an existing old use case.  Compile params or Response file should have changed?
					//				2) WORKING: Indirect file went away (unity'd original source file or include).  This would return a file that no longer exists and adds to the prerequiteitems list
					var IncludedFileList = CPPEnvironment.FindAndCacheAllIncludedFiles( Target, SourceFile, BuildPlatform, CompileEnvironment.GetIncludesPathsToSearch( SourceFile ), CompileEnvironment.IncludeFileSearchDictionary, bOnlyCachedDependencies:BuildConfiguration.bUseExperimentalFastDependencyScan );
					CompileAction.PrerequisiteItems.AddRange( IncludedFileList );
				}

				// If this is a CLR file then make sure our dependent assemblies are added as prerequisites
				if (CompileEnvironment.Config.CLRMode == CPPCLRMode.CLREnabled)
				{
					foreach( PrivateAssemblyInfo CurPrivateAssemblyDependency in CompileEnvironment.PrivateAssemblyDependencies )
					{
						CompileAction.PrerequisiteItems.Add( CurPrivateAssemblyDependency.FileItem );
					}
				}

				bool bEmitsObjectFile = true;
				if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Create)
				{
					// Generate a CPP File that just includes the precompiled header.
					string PCHCPPFilename = "PCH." + ModuleName + "." + Path.GetFileName(CompileEnvironment.Config.PrecompiledHeaderIncludeFilename) + ".cpp";
					string PCHCPPPath = Path.Combine(CompileEnvironment.Config.OutputDirectory, PCHCPPFilename);
					FileItem PCHCPPFile = FileItem.CreateIntermediateTextFile(
						PCHCPPPath,
						string.Format("#include \"{0}\"\r\n", CompileEnvironment.Config.PrecompiledHeaderIncludeFilename)
						);

					// Make sure the original source directory the PCH header file existed in is added as an include
					// path -- it might be a private PCH header and we need to make sure that its found!
					string OriginalPCHHeaderDirectory = Path.GetDirectoryName( SourceFile.AbsolutePath );
					FileArguments.AppendFormat(" /I \"{0}\"", OriginalPCHHeaderDirectory);

					var PrecompiledFileExtension = UEBuildPlatform.BuildPlatformDictionary[UnrealTargetPlatform.Win64].GetBinaryExtension(UEBuildBinaryType.PrecompiledHeader);
					// Add the precompiled header file to the produced items list.
					FileItem PrecompiledHeaderFile = FileItem.GetItemByPath(
						Path.Combine(
							CompileEnvironment.Config.OutputDirectory,
						Path.GetFileName(SourceFile.AbsolutePath) + PrecompiledFileExtension
							)
						);
					CompileAction.ProducedItems.Add(PrecompiledHeaderFile);
					Result.PrecompiledHeaderFile = PrecompiledHeaderFile;

					// @todo clang: Ideally clang-cl would support the regular MSVC arguments for PCH files, and we wouldn't need to do this manually
					if( WindowsPlatform.bCompileWithClang )
					{
						// Tell Clang to generate a PCH header
						FileArguments.Append(" -Xclang -x -Xclang c++-header");	// @todo clang: Doesn't work do to Clang-cl overriding us at the end of the command-line (see -### option output)
						FileArguments.AppendFormat(" /Fo\"{0}\"", PrecompiledHeaderFile.AbsolutePath);

						// Clang PCH generation doesn't create an .obj file to link in, unlike MSVC
						bEmitsObjectFile = false;
					}
					else
					{
						// Add the parameters needed to compile the precompiled header file to the command-line.
						FileArguments.AppendFormat(" /Yc\"{0}\"", CompileEnvironment.Config.PrecompiledHeaderIncludeFilename);
						FileArguments.AppendFormat(" /Fp\"{0}\"", PrecompiledHeaderFile.AbsolutePath);

						// If we're creating a PCH that will be used to compile source files for a library, we need
						// the compiled modules to retain a reference to PCH's module, so that debugging information
						// will be included in the library.  This is also required to avoid linker warning "LNK4206"
						// when linking an application that uses this library.
						if (CompileEnvironment.Config.bIsBuildingLibrary)
						{
							// NOTE: The symbol name we use here is arbitrary, and all that matters is that it is
							// unique per PCH module used in our library
							string FakeUniquePCHSymbolName = Path.GetFileNameWithoutExtension(CompileEnvironment.Config.PrecompiledHeaderIncludeFilename);
							FileArguments.AppendFormat(" /Yl{0}", FakeUniquePCHSymbolName);
						}
					}

					FileArguments.AppendFormat(" \"{0}\"", PCHCPPFile.AbsolutePath);

					CompileAction.StatusDescription = PCHCPPFilename;
				}
				else
				{
					if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Include)
					{
						CompileAction.bIsUsingPCH = true;
						CompileAction.PrerequisiteItems.Add(CompileEnvironment.PrecompiledHeaderFile);

						if( WindowsPlatform.bCompileWithClang )
						{
							// NOTE: With Clang, PCH headers are ALWAYS forcibly included!
							// NOTE: This needs to be before the other include paths to ensure Clang uses it instead of the source header file.
							FileArguments.AppendFormat(" /FI\"{0}\"", Path.ChangeExtension(CompileEnvironment.PrecompiledHeaderFile.AbsolutePath, null));
						}
						else
						{
							FileArguments.AppendFormat(" /Yu\"{0}\"", CompileEnvironment.Config.PCHHeaderNameInCode);
							FileArguments.AppendFormat(" /Fp\"{0}\"", CompileEnvironment.PrecompiledHeaderFile.AbsolutePath);

							// Is it unsafe to always force inclusion?  Clang is doing it, and .generated.cpp files
							// won't work otherwise, because they're not located in the context of the module,
							// so they can't access the module's PCH without an absolute path.
							//if( CompileEnvironment.Config.bForceIncludePrecompiledHeader )
							{
								// Force include the precompiled header file.  This is needed because we may have selected a
								// precompiled header that is different than the first direct include in the C++ source file, but
								// we still need to make sure that our precompiled header is the first thing included!
								FileArguments.AppendFormat( " /FI\"{0}\"", CompileEnvironment.Config.PCHHeaderNameInCode);
							}
						}
					}
					
					// Add the source file path to the command-line.
					FileArguments.AppendFormat(" \"{0}\"", SourceFile.AbsolutePath);

					CompileAction.StatusDescription = Path.GetFileName( SourceFile.AbsolutePath );
				}

				if( bEmitsObjectFile )
				{
					var ObjectFileExtension = UEBuildPlatform.BuildPlatformDictionary[UnrealTargetPlatform.Win64].GetBinaryExtension(UEBuildBinaryType.Object);
					// Add the object file to the produced item list.
					FileItem ObjectFile = FileItem.GetItemByPath(
						Path.Combine(
							CompileEnvironment.Config.OutputDirectory,
							Path.GetFileName(SourceFile.AbsolutePath) + ObjectFileExtension
							)
						);
					CompileAction.ProducedItems.Add(ObjectFile);
					Result.ObjectFiles.Add(ObjectFile);
					FileArguments.AppendFormat(" /Fo\"{0}\"", ObjectFile.AbsolutePath);
				}

				// Create PDB files if we were configured to do that.
				//
				// Also, when debug info is off and XGE is enabled, force PDBs, otherwise it will try to share
				// a PDB file, which causes PCH creation to be serial rather than parallel (when debug info is disabled)
				//		--> See https://udn.epicgames.com/lists/showpost.php?id=50619&list=unprog3
				if (BuildConfiguration.bUsePDBFiles ||
					(BuildConfiguration.bAllowXGE && !CompileEnvironment.Config.bCreateDebugInfo))
				{
					string PDBFileName;
					bool bActionProducesPDB = false;

					// All files using the same PCH are required to share the same PDB that was used when compiling the PCH
					if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Include)
					{
						PDBFileName = "PCH." + ModuleName + "." + Path.GetFileName(CompileEnvironment.Config.PrecompiledHeaderIncludeFilename);
					}
					// Files creating a PCH use a PDB per file.
					else if (CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Create)
					{
						PDBFileName = "PCH." + ModuleName + "." + Path.GetFileName(CompileEnvironment.Config.PrecompiledHeaderIncludeFilename);
						bActionProducesPDB = true;
					}
					// Ungrouped C++ files use a PDB per file.
					else if( !bIsPlainCFile )
					{
						PDBFileName = Path.GetFileName( SourceFile.AbsolutePath );
						bActionProducesPDB = true;
					}
					// Group all plain C files that doesn't use PCH into the same PDB
					else
					{
						PDBFileName = "MiscPlainC";
					}

					// @todo Clang: Clang doesn't emit PDB files even when debugging is enabled
					if( !WindowsPlatform.bCompileWithClang )
					{ 
						// Specify the PDB file that the compiler should write to.
						FileItem PDBFile = FileItem.GetItemByPath(
								Path.Combine(
									CompileEnvironment.Config.OutputDirectory,
									PDBFileName + ".pdb"
									)
								);
						FileArguments.AppendFormat(" /Fd\"{0}\"", PDBFile.AbsolutePath);

						// Only use the PDB as an output file if we want PDBs and this particular action is
						// the one that produces the PDB (as opposed to no debug info, where the above code
						// is needed, but not the output PDB, or when multiple files share a single PDB, so
						// only the action that generates it should count it as output directly)
						if( BuildConfiguration.bUsePDBFiles && bActionProducesPDB )
						{
							CompileAction.ProducedItems.Add(PDBFile);
							Result.DebugDataFiles.Add(PDBFile);
						}
					}
				}

				// Add C or C++ specific compiler arguments.
				if (bIsPlainCFile)
				{
					AppendCLArguments_C(FileArguments);
				}
				else
				{
					AppendCLArguments_CPP( CompileEnvironment, FileArguments );
				}

				CompileAction.WorkingDirectory = Path.GetFullPath(".");
				CompileAction.CommandPath = GetVCToolPath(CompileEnvironment.Config.Target.Platform, CompileEnvironment.Config.Target.Configuration, "cl");

				if( !WindowsPlatform.bCompileWithClang )
				{
					CompileAction.bIsVCCompiler = true;
				}
				CompileAction.CommandArguments = Arguments.ToString() + FileArguments.ToString() + CompileEnvironment.Config.AdditionalArguments;
				CompileAction.StatusDetailedDescription = SourceFile.Description;

				if( CompileEnvironment.Config.PrecompiledHeaderAction == PrecompiledHeaderAction.Create )
				{
					Log.TraceVerbose("Creating PCH: " + CompileEnvironment.Config.PrecompiledHeaderIncludeFilename);
					Log.TraceVerbose("     Command: " + CompileAction.CommandArguments);
				}
				else
				{
					Log.TraceVerbose("   Compiling: " + CompileAction.StatusDescription);
					Log.TraceVerbose("     Command: " + CompileAction.CommandArguments);
				}

				if( WindowsPlatform.bCompileWithClang )
				{
					// Clang doesn't print the file names by default, so we'll do it ourselves
					CompileAction.bShouldOutputStatusDescription = true;
				}
				else
				{
					// VC++ always outputs the source file name being compiled, so we don't need to emit this ourselves
					CompileAction.bShouldOutputStatusDescription = false;
				}

				// Don't farm out creation of precompiled headers as it is the critical path task.
				CompileAction.bCanExecuteRemotely =
					CompileEnvironment.Config.PrecompiledHeaderAction != PrecompiledHeaderAction.Create ||
					BuildConfiguration.bAllowRemotelyCompiledPCHs
                    ;

                // @todo: XGE has problems remote compiling C++/CLI files that use .NET Framework 4.0
				if (CompileEnvironment.Config.CLRMode == CPPCLRMode.CLREnabled)
				{
					CompileAction.bCanExecuteRemotely = false;
				}
			}
			return Result;
		}

		public override CPPOutput CompileRCFiles(UEBuildTarget Target, CPPEnvironment Environment, List<FileItem> RCFiles)
		{
			CPPOutput Result = new CPPOutput();

			var BuildPlatform = UEBuildPlatform.GetBuildPlatformForCPPTargetPlatform(Environment.Config.Target.Platform);

			foreach (FileItem RCFile in RCFiles)
			{
				Action CompileAction = new Action(ActionType.Compile);
				CompileAction.WorkingDirectory = Path.GetFullPath(".");
				CompileAction.CommandPath = GetVCToolPath(Environment.Config.Target.Platform, Environment.Config.Target.Configuration, "rc");
				CompileAction.StatusDescription = Path.GetFileName(RCFile.AbsolutePath);

				// Suppress header spew
				CompileAction.CommandArguments += " /nologo";

				// If we're compiling for 64-bit Windows, also add the _WIN64 definition to the resource
				// compiler so that we can switch on that in the .rc file using #ifdef.
				if (Environment.Config.Target.Platform == CPPTargetPlatform.Win64)
				{
					CompileAction.CommandArguments += " /D _WIN64";
				}

				// When targeting Windows XP with Visual Studio 2012+, we need to tell the compiler to use the older Windows SDK that works
				// with Windows XP (http://blogs.msdn.com/b/vcblog/archive/2012/10/08/10357555.aspx)
				if (WindowsPlatform.SupportWindowsXP)
				{
					if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
					{
						CompileAction.CommandArguments += " /D_USING_V120_SDK71_";
					}
					else if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2012)
					{
						CompileAction.CommandArguments += " /D_USING_V110_SDK71_";
					}
				}

				// Language
				CompileAction.CommandArguments += " /l 0x409";

				// Include paths.
				foreach (string IncludePath in Environment.Config.IncludePaths)
				{
					CompileAction.CommandArguments += string.Format( " /i \"{0}\"", IncludePath );
				}

				// System include paths.
				foreach( var SystemIncludePath in Environment.Config.SystemIncludePaths )
				{
					CompileAction.CommandArguments += string.Format( " /i \"{0}\"", SystemIncludePath );
				}

				// Preprocessor definitions.
				foreach (string Definition in Environment.Config.Definitions)
				{
					CompileAction.CommandArguments += string.Format(" /d \"{0}\"", Definition);
				}

				// Add the RES file to the produced item list.
				FileItem CompiledResourceFile = FileItem.GetItemByPath(
					Path.Combine(
						Environment.Config.OutputDirectory,
						Path.GetFileName(RCFile.AbsolutePath) + ".res"
						)
					);
				CompileAction.ProducedItems.Add(CompiledResourceFile);
				CompileAction.CommandArguments += string.Format(" /fo \"{0}\"", CompiledResourceFile.AbsolutePath);
				Result.ObjectFiles.Add(CompiledResourceFile);

				// Add the RC file as a prerequisite of the action.
				CompileAction.PrerequisiteItems.Add(RCFile);
				CompileAction.CommandArguments += string.Format(" \"{0}\"", RCFile.AbsolutePath);

				// Add the files included by the RC file as prerequisites of the action.
				{
					var IncludedFileList = CPPEnvironment.FindAndCacheAllIncludedFiles( Target, RCFile, BuildPlatform, Environment.GetIncludesPathsToSearch( RCFile ), Environment.IncludeFileSearchDictionary, bOnlyCachedDependencies:BuildConfiguration.bUseExperimentalFastDependencyScan );
					CompileAction.PrerequisiteItems.AddRange( IncludedFileList );
				}
			}

			return Result;
		}

		public override FileItem LinkFiles(LinkEnvironment LinkEnvironment, bool bBuildImportLibraryOnly)
		{
			if (LinkEnvironment.Config.bIsBuildingDotNetAssembly)
			{
				return FileItem.GetItemByPath(LinkEnvironment.Config.OutputFilePath);
			}

			bool bIsBuildingLibrary = LinkEnvironment.Config.bIsBuildingLibrary || bBuildImportLibraryOnly;
			bool bIncludeDependentLibrariesInLibrary = bIsBuildingLibrary && LinkEnvironment.Config.bIncludeDependentLibrariesInLibrary;

			// Get link arguments.
			StringBuilder Arguments = new StringBuilder();
			if(bIsBuildingLibrary)
			{
				AppendLibArguments(LinkEnvironment, Arguments);
			}
			else
			{
				AppendLinkArguments(LinkEnvironment, Arguments);
			}



			// If we're only building an import library, add the '/DEF' option that tells the LIB utility
			// to simply create a .LIB file and .EXP file, and don't bother validating imports
			if (bBuildImportLibraryOnly)
			{
				Arguments.Append(" /DEF");

				// Ensure that the import library references the correct filename for the linked binary.
				Arguments.AppendFormat(" /NAME:\"{0}\"", Path.GetFileName(LinkEnvironment.Config.OutputFilePath));
			}


			// Add delay loaded DLLs.
			if (!bIsBuildingLibrary)
			{
				// Delay-load these DLLs.
				foreach (string DelayLoadDLL in LinkEnvironment.Config.DelayLoadDLLs)
				{
					Arguments.AppendFormat(" /DELAYLOAD:\"{0}\"", DelayLoadDLL);
				}
			}

			// @todo UE4 DLL: Why do I need LIBPATHs to build only export libraries with /DEF? (tbbmalloc.lib)
			if (!LinkEnvironment.Config.bIsBuildingLibrary || (LinkEnvironment.Config.bIsBuildingLibrary && bIncludeDependentLibrariesInLibrary))
			{
				// Add the library paths to the argument list.
				foreach (string LibraryPath in LinkEnvironment.Config.LibraryPaths)
				{
					Arguments.AppendFormat(" /LIBPATH:\"{0}\"", LibraryPath);
				}

				// Add the excluded default libraries to the argument list.
				foreach (string ExcludedLibrary in LinkEnvironment.Config.ExcludedLibraries)
				{
					Arguments.AppendFormat(" /NODEFAULTLIB:\"{0}\"", ExcludedLibrary);
				}
			}

			// For targets that are cross-referenced, we don't want to write a LIB file during the link step as that
			// file will clobber the import library we went out of our way to generate during an earlier step.  This
			// file is not needed for our builds, but there is no way to prevent MSVC from generating it when
			// linking targets that have exports.  We don't want this to clobber our LIB file and invalidate the
			// existing timstamp, so instead we simply emit it with a different name
			string ImportLibraryFilePath = Path.Combine( LinkEnvironment.Config.IntermediateDirectory, 
														 Path.GetFileNameWithoutExtension(LinkEnvironment.Config.OutputFilePath) + ".lib" );

			if (LinkEnvironment.Config.bIsCrossReferenced && !bBuildImportLibraryOnly)
			{
				ImportLibraryFilePath += ".suppressed";
			}

			FileItem OutputFile;
			if (bBuildImportLibraryOnly)
			{
				OutputFile = FileItem.GetItemByPath(ImportLibraryFilePath);
			}
			else
			{
				OutputFile = FileItem.GetItemByPath(LinkEnvironment.Config.OutputFilePath);
				OutputFile.bNeedsHotReloadNumbersDLLCleanUp = LinkEnvironment.Config.bIsBuildingDLL;
			}

			List<FileItem> ProducedItems = new List<FileItem>();
			ProducedItems.Add(OutputFile);

			List<FileItem> PrerequisiteItems = new List<FileItem>();

			// Add the input files to a response file, and pass the response file on the command-line.
			List<string> InputFileNames = new List<string>();
			foreach (FileItem InputFile in LinkEnvironment.InputFiles)
			{
				InputFileNames.Add(string.Format("\"{0}\"", InputFile.AbsolutePath));
				PrerequisiteItems.Add(InputFile);
			}

			if (!bBuildImportLibraryOnly)
			{
				// Add input libraries as prerequisites, too!
				foreach (FileItem InputLibrary in LinkEnvironment.InputLibraries)
				{
					InputFileNames.Add(string.Format("\"{0}\"", InputLibrary.AbsolutePath));
					PrerequisiteItems.Add(InputLibrary);
				}
			}

			if (!bIsBuildingLibrary || (LinkEnvironment.Config.bIsBuildingLibrary && bIncludeDependentLibrariesInLibrary))
			{
				foreach (string AdditionalLibrary in LinkEnvironment.Config.AdditionalLibraries)
				{
					InputFileNames.Add(string.Format("\"{0}\"", AdditionalLibrary));

					// If the library file name has a relative path attached (rather than relying on additional
					// lib directories), then we'll add it to our prerequisites list.  This will allow UBT to detect
					// when the binary needs to be relinked because a dependent external library has changed.
					//if( !String.IsNullOrEmpty( Path.GetDirectoryName( AdditionalLibrary ) ) )
					{
						PrerequisiteItems.Add(FileItem.GetItemByPath(AdditionalLibrary));
					}
				}
			}

			// Create a response file for the linker
			string ResponseFileName = GetResponseFileName( LinkEnvironment, OutputFile );

			// Never create response files when we are only generating IntelliSense data
			if( !ProjectFileGenerator.bGenerateProjectFiles )
			{
				ResponseFile.Create( ResponseFileName, InputFileNames );
			}
			Arguments.AppendFormat(" @\"{0}\"", ResponseFileName);

			// Add the output file to the command-line.
			Arguments.AppendFormat(" /OUT:\"{0}\"", OutputFile.AbsolutePath);

			if (bBuildImportLibraryOnly || (LinkEnvironment.Config.bHasExports && !bIsBuildingLibrary))
			{
				// An export file is written to the output directory implicitly; add it to the produced items list.
				string ExportFilePath = Path.ChangeExtension(ImportLibraryFilePath, ".exp");
				FileItem ExportFile = FileItem.GetItemByPath(ExportFilePath);
				ProducedItems.Add(ExportFile);
			}

			if (!bIsBuildingLibrary)
			{
				// Xbox 360 LTCG does not seem to produce those.
				if (LinkEnvironment.Config.bHasExports &&
					(LinkEnvironment.Config.Target.Configuration != CPPTargetConfiguration.Shipping))
				{
					// Write the import library to the output directory for nFringe support.
					FileItem ImportLibraryFile = FileItem.GetItemByPath(ImportLibraryFilePath);
					Arguments.AppendFormat(" /IMPLIB:\"{0}\"", ImportLibraryFilePath);
					ProducedItems.Add(ImportLibraryFile);
				}

				if (LinkEnvironment.Config.bCreateDebugInfo)
				{
					// Write the PDB file to the output directory.			
					// @todo Clang: Clang doesn't emit PDB files even when debugging is enabled
					if( !WindowsPlatform.bCompileWithClang )
					{
						string PDBFilePath = Path.Combine(LinkEnvironment.Config.OutputDirectory, Path.GetFileNameWithoutExtension(OutputFile.AbsolutePath) + ".pdb");
						FileItem PDBFile = FileItem.GetItemByPath(PDBFilePath);
						Arguments.AppendFormat(" /PDB:\"{0}\"", PDBFilePath);
						ProducedItems.Add(PDBFile);
					}

					// Write the MAP file to the output directory.			
#if false					
					if (true)
					{
						string MAPFilePath = Path.Combine(LinkEnvironment.Config.OutputDirectory, Path.GetFileNameWithoutExtension(OutputFile.AbsolutePath) + ".map");
						FileItem MAPFile = FileItem.GetItemByPath(MAPFilePath);
						LinkAction.CommandArguments += string.Format(" /MAP:\"{0}\"", MAPFilePath);
						LinkAction.ProducedItems.Add(MAPFile);
					}
#endif
				}

				// Add the additional arguments specified by the environment.
				Arguments.Append(LinkEnvironment.Config.AdditionalArguments);
			}


			// Create an action that invokes the linker.
			Action LinkAction = new Action(ActionType.Link);
			LinkAction.WorkingDirectory = Path.GetFullPath(".");
			LinkAction.CommandPath = GetVCToolPath(
				LinkEnvironment.Config.Target.Platform,
				LinkEnvironment.Config.Target.Configuration,
				bIsBuildingLibrary ? "lib" : "link");
			LinkAction.CommandArguments = Arguments.ToString();
			LinkAction.ProducedItems.AddRange(ProducedItems);
			LinkAction.PrerequisiteItems.AddRange(PrerequisiteItems);
			LinkAction.StatusDescription = Path.GetFileName(OutputFile.AbsolutePath);

			// Tell the action that we're building an import library here and it should conditionally be
			// ignored as a prerequisite for other actions
			LinkAction.bProducesImportLibrary = bBuildImportLibraryOnly || LinkEnvironment.Config.bIsBuildingDLL;

			// Only execute linking on the local PC.
			LinkAction.bCanExecuteRemotely = false;

			Log.TraceVerbose( "     Linking: " + LinkAction.StatusDescription );
			Log.TraceVerbose( "     Command: " + LinkAction.CommandArguments );

			return OutputFile;
		}

		public override void CompileCSharpProject(CSharpEnvironment CompileEnvironment, string ProjectFileName, string DestinationFile)
		{
			var BuildProjectAction = new Action(ActionType.BuildProject);

			// Specify the source file (prerequisite) for the action
			var ProjectFileItem = FileItem.GetExistingItemByPath(ProjectFileName);
			if (ProjectFileItem == null)
			{
				throw new BuildException("Expected C# project file {0} to exist.", ProjectFileName);
			}
			
			// Add the project and the files contained to the prerequisites.
			BuildProjectAction.PrerequisiteItems.Add(ProjectFileItem);
			var ProjectFile = new VCSharpProjectFile( Utils.MakePathRelativeTo( ProjectFileName, ProjectFileGenerator.MasterProjectRelativePath ) );
			var ProjectPreReqs = ProjectFile.GetCSharpDependencies();
			var ProjectFolder = Path.GetDirectoryName(ProjectFileName);
			foreach( string ProjectPreReqRelativePath in ProjectPreReqs )
			{
				string ProjectPreReqAbsolutePath = Path.Combine( ProjectFolder, ProjectPreReqRelativePath );
				var ProjectPreReqFileItem = FileItem.GetExistingItemByPath(ProjectPreReqAbsolutePath);
				if( ProjectPreReqFileItem == null )
				{
					throw new BuildException("Expected C# dependency {0} to exist.", ProjectPreReqAbsolutePath);
				}
				BuildProjectAction.PrerequisiteItems.Add(ProjectPreReqFileItem);
			}

			// We might be able to distribute this safely, but it doesn't take any time.
			BuildProjectAction.bCanExecuteRemotely = false;

			// Setup execution via MSBuild.
			BuildProjectAction.WorkingDirectory = Path.GetFullPath(".");
			BuildProjectAction.StatusDescription = Path.GetFileName(ProjectFileName);
			BuildProjectAction.CommandPath = GetDotNetFrameworkToolPath(CompileEnvironment.EnvironmentTargetPlatform, "MSBuild");
			if (CompileEnvironment.TargetConfiguration == CSharpTargetConfiguration.Debug)
			{
				BuildProjectAction.CommandArguments = " /target:rebuild /property:Configuration=Debug";
			}
			else
			{
				BuildProjectAction.CommandArguments = " /target:rebuild /property:Configuration=Development";
			}

			// Be less verbose
			BuildProjectAction.CommandArguments += " /nologo /verbosity:minimal";

			// Add project
			BuildProjectAction.CommandArguments += String.Format(" \"{0}\"", ProjectFileItem.AbsolutePath);

			// We don't want to display all of the regular MSBuild output to the console
			BuildProjectAction.bShouldBlockStandardOutput = false;

			// Specify the output files.
			string PDBFilePath = Path.Combine( Path.GetDirectoryName(DestinationFile), Path.GetFileNameWithoutExtension(DestinationFile) + ".pdb" );
			FileItem PDBFile = FileItem.GetItemByPath( PDBFilePath );
			BuildProjectAction.ProducedItems.Add( FileItem.GetItemByPath(DestinationFile) );		
			BuildProjectAction.ProducedItems.Add( PDBFile );
		}

		/** Gets the default include paths for the given platform. */
		public static string GetVCIncludePaths(CPPTargetPlatform Platform)
		{
			string IncludePaths = "";
			if (Platform == CPPTargetPlatform.Win32 || Platform == CPPTargetPlatform.Win64)
			{
				// Make sure we've got the environment variables set up for this target
				VCToolChain.InitializeEnvironmentVariables(Platform);

				// Also add any include paths from the INCLUDE environment variable.  MSVC is not necessarily running with an environment that
				// matches what UBT extracted from the vcvars*.bat using SetEnvironmentVariablesFromBatchFile().  We'll use the variables we
				// extracted to populate the project file's list of include paths
				// @todo projectfiles: Should we only do this for VC++ platforms?
				IncludePaths = Environment.GetEnvironmentVariable("INCLUDE");
				if (!String.IsNullOrEmpty(IncludePaths) && !IncludePaths.EndsWith(";"))
				{
					IncludePaths += ";";
				}
			}
			return IncludePaths;
		}

		/** Accesses the bin directory for the VC toolchain for the specified platform. */
		static string GetVCToolPath(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration, string ToolName)
		{	
			// Initialize environment variables required for spawned tools.
			InitializeEnvironmentVariables( Platform );

			// Out variable that is going to contain fully qualified path to executable upon return.
			string VCToolPath = "";

			// rc.exe resides in the Windows SDK folder.
			if (ToolName.ToUpperInvariant() == "RC")
			{
				// 64 bit -- we can use the 32 bit version to target 64 bit on 32 bit OS.
				if (Platform == CPPTargetPlatform.Win64)
				{
					VCToolPath = Path.Combine(WindowsSDKDir, "bin/x64/rc.exe");
				}
				// 32 bit
				else
				{
					if( !WindowsPlatform.SupportWindowsXP )	// Windows XP requires use to force Windows SDK 7.1 even on the newer compiler, so we need the old path RC.exe
					{
						VCToolPath = Path.Combine( WindowsSDKDir, "bin/x86/rc.exe" );
					}
					else
					{
						VCToolPath = Path.Combine( WindowsSDKDir, "bin/rc.exe" );
					}
				}
			}
			// cl.exe and link.exe are found in the toolchain specific folders (32 vs. 64 bit)
			else
			{
				bool bIsRequestingLinkTool = ToolName.Equals( "link", StringComparison.InvariantCultureIgnoreCase );
				bool bIsRequestingLibTool = ToolName.Equals( "lib", StringComparison.InvariantCultureIgnoreCase );

				// If we were asked to use Clang, then we'll redirect the path to the compiler to the LLVM installation directory
				if( WindowsPlatform.bCompileWithClang && !bIsRequestingLinkTool && !bIsRequestingLibTool )
				{
					VCToolPath = Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.ProgramFilesX86 ), "LLVM", "msbuild-bin", ToolName + ".exe" );
					if( !File.Exists( VCToolPath ) )
					{
						throw new BuildException( "Clang was selected as the Windows compiler, but LLVM/Clang does not appear to be installed.  Could not find: " + VCToolPath );
					}
				}
				else
				{
					string BaseVSToolPath = FindBaseVSToolPath();

					// Both target and build machines are 64 bit
					bool bIs64Bit = (Platform == CPPTargetPlatform.Win64);
					// Regardless of the target, if we're linking on a 64 bit machine, we want to use the 64 bit linker (it's faster than the 32 bit linker)
					//@todo.WIN32: Using the 64-bit linker appears to be broken at the moment.
					bool bUse64BitLinker = (Platform == CPPTargetPlatform.Win64) && bIsRequestingLinkTool;

					// Use the 64 bit tools if the build machine and target are 64 bit or if we're linking a 32 bit binary on a 64 bit machine
					if (bIs64Bit || bUse64BitLinker)
					{
						// Use the native 64-bit compiler if present, otherwise use the amd64-on-x86 compiler. VS2012 Express only includes the latter.
						string PlatformToolPath = Path.Combine(BaseVSToolPath, "../../VC/bin/amd64/");
						if(!Directory.Exists(PlatformToolPath))
						{
							PlatformToolPath = Path.Combine(BaseVSToolPath, "../../VC/bin/x86_amd64/");
						}
						VCToolPath = PlatformToolPath + ToolName + ".exe";
					}
					else
					{
						// Use 32 bit for cl.exe and other tools, or for link.exe if 64 bit path doesn't exist and we're targeting 32 bit.
						VCToolPath = Path.Combine(BaseVSToolPath, "../../VC/bin/" + ToolName + ".exe");
					}
				}
			}

			return VCToolPath;
		}

		/** Accesses the directory for .NET Framework binaries such as MSBuild */
		static string GetDotNetFrameworkToolPath(CPPTargetPlatform Platform, string ToolName)
		{
			// Initialize environment variables required for spawned tools.
			InitializeEnvironmentVariables(Platform);

			string FrameworkDirectory = Environment.GetEnvironmentVariable("FrameworkDir");
			string FrameworkVersion = Environment.GetEnvironmentVariable("FrameworkVersion");
			if (FrameworkDirectory == null || FrameworkVersion == null)
			{
				throw new BuildException( "NOTE: Please ensure that 64bit Tools are installed with DevStudio - there is usually an option to install these during install" );
			}
			string DotNetFrameworkBinDir = Path.Combine(FrameworkDirectory, FrameworkVersion);
			string ToolPath = Path.Combine(DotNetFrameworkBinDir, ToolName + ".exe");
			return ToolPath;
		}

		/** Helper to only initialize environment variables once. */
		static bool bAreEnvironmentVariablesAlreadyInitialized = false;

		/** Helper to make sure environment variables have been initialized for the right platform. */
		static CPPTargetPlatform PlatformEnvironmentVariablesAreInitializedFor = CPPTargetPlatform.Win32;

		/** Installation folder of the Windows SDK, e.g. C:\Program Files\Microsoft SDKs\Windows\v6.0A\ */
		static string WindowsSDKDir = "";

		/**
		 * Initializes environment variables required by toolchain. Different for 32 and 64 bit.
		 */
		static void InitializeEnvironmentVariables( CPPTargetPlatform Platform )
		{
			if (!bAreEnvironmentVariablesAlreadyInitialized || Platform != PlatformEnvironmentVariablesAreInitializedFor)
			{
				string BaseVSToolPath = FindBaseVSToolPath();
				
				string VCVarsBatchFile = "";

				// 64 bit tool chain.
				if( Platform == CPPTargetPlatform.Win64 )
				{
					VCVarsBatchFile = Path.Combine(BaseVSToolPath, "../../VC/bin/x86_amd64/vcvarsx86_amd64.bat");
				}
				// The 32 bit vars batch file in the binary folder simply points to the one in the common tools folder.
				else
				{
					VCVarsBatchFile = Path.Combine(BaseVSToolPath, "vsvars32.bat");
				}
				Utils.SetEnvironmentVariablesFromBatchFile(VCVarsBatchFile);

				// Lib and bin folders have a x64 subfolder for 64 bit development.
				string ConfigSuffix = "";
				if( Platform == CPPTargetPlatform.Win64 )
				{
					ConfigSuffix = "\\x64";
				}

				// When targeting Windows XP on Visual Studio 2012+, we need to override the Windows SDK include and lib path set
				// by the batch file environment (http://blogs.msdn.com/b/vcblog/archive/2012/10/08/10357555.aspx)
				if( WindowsPlatform.SupportWindowsXP )
				{
					Environment.SetEnvironmentVariable("PATH", Utils.ResolveEnvironmentVariable(WindowsSDKDir + "bin" + ConfigSuffix + ";%PATH%"));
					Environment.SetEnvironmentVariable("LIB", Utils.ResolveEnvironmentVariable(WindowsSDKDir + "lib" + ConfigSuffix + ";%LIB%"));
					Environment.SetEnvironmentVariable( "INCLUDE", Utils.ResolveEnvironmentVariable(WindowsSDKDir + "include;%INCLUDE%"));
				}

				bAreEnvironmentVariablesAlreadyInitialized = true;
				PlatformEnvironmentVariablesAreInitializedFor = Platform;
			}			
		}

		/// <returns>The path to Windows SDK directory for the specified version.</returns>
		private static string FindWindowsSDKInstallationFolder( string Version )
		{
			// Based on VCVarsQueryRegistry
			string WinSDKPath = (string)Microsoft.Win32.Registry.GetValue( @"HKEY_CURRENT_USER\SOFTWARE\Microsoft\Microsoft SDKs\Windows\" + Version, "InstallationFolder", null );
			if( WinSDKPath != null )
			{
				return WinSDKPath;
			}
			WinSDKPath = (string)Microsoft.Win32.Registry.GetValue( @"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\" + Version, "InstallationFolder", null );
			if( WinSDKPath != null )
			{
				return WinSDKPath;
			}
			WinSDKPath = (string)Microsoft.Win32.Registry.GetValue( @"HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\" + Version, "InstallationFolder", null );
			if( WinSDKPath != null )
			{
				return WinSDKPath;
			}

			throw new BuildException( "Windows SDK {0} must be installed in order to build this target.", Version );
		}

		/// <summary>
		/// Figures out the path to Visual Studio's /Common7/Tools directory.  Note that if Visual Studio is not
		/// installed, the Windows SDK path will be used, which also happens to be the same directory. (It installs
		/// the toolchain into the folder where Visual Studio would have installed it to.
		/// </summary>
		/// <returns>The path to Visual Studio's /Common7/Tools directory</returns>
		private static string FindBaseVSToolPath()
		{
			string BaseVSToolPath = "";
			
			// When targeting Windows XP on Visual Studio 2012+, we need to point at the older Windows SDK 7.1A that comes
			// installed with Visual Studio 2012 Update 1. (http://blogs.msdn.com/b/vcblog/archive/2012/10/08/10357555.aspx)
			if( WindowsPlatform.SupportWindowsXP )
			{
				WindowsSDKDir = FindWindowsSDKInstallationFolder( "v7.1A" );
			}
			else
			{
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
				{
					WindowsSDKDir = FindWindowsSDKInstallationFolder( "v8.1" );
				}
				else if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2012)
				{
					WindowsSDKDir = FindWindowsSDKInstallationFolder( "v8.0" );
				}
			}

			// Grab path to Visual Studio binaries from the system environment
			BaseVSToolPath = WindowsPlatform.GetVSComnToolsPath();

			if (string.IsNullOrEmpty(BaseVSToolPath))
			{
				throw new BuildException("Visual Studio 2012 or Visual Studio 2013 must be installed in order to build this target.");
			}

			return BaseVSToolPath;
		}

        public override void AddFilesToManifest(ref FileManifest Manifest, UEBuildBinary Binary)
        {
            // ok, this is pretty awful, we want the import libraries that go with the editor, only on the PC
            if (UnrealBuildTool.BuildingRocket() &&
                Path.GetFileNameWithoutExtension(Binary.Config.OutputFilePath).StartsWith("UE4Editor-", StringComparison.InvariantCultureIgnoreCase) &&
                Path.GetExtension(Binary.Config.OutputFilePath).EndsWith("dll", StringComparison.InvariantCultureIgnoreCase) &&
                Binary.Config.Type == UEBuildBinaryType.DynamicLinkLibrary)
            {
                // ok, this is pretty awful, we want the import libraries that go with the editor, only on the PC
                Manifest.AddBinaryNames(Path.Combine(Binary.Config.IntermediateDirectory, Path.GetFileNameWithoutExtension(Binary.Config.OutputFilePath) + ".lib"), "");
            }
        }
	};
}
