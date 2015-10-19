// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;
using System.Text;

namespace UnrealBuildTool
{
	class VCEnvironment
	{
		public readonly CPPTargetPlatform Platform;             // The platform the envvars have been initialized for
		public readonly string            BaseVSToolPath;       // The path to Visual Studio's /Common7/Tools directory.
		public readonly string            PlatformVSToolPath;   // The path to the platform tool binaries.
		public readonly string            WindowsSDKDir;        // Installation folder of the Windows SDK, e.g. C:\Program Files\Microsoft SDKs\Windows\v6.0A\
		public readonly string            WindowsSDKExtensionDir;  // Installation folder of the Windows SDK Extensions, e.g. C:\Program Files (x86)\Windows SDKs\10
		public readonly string            NetFxSDKExtensionDir;    // Installation folder of the NetFx SDK, since that is split out from platform SDKs >= v10
		public readonly Version           WindowsSDKExtensionHeaderLibVersion;  // 10.0.9910.0 for instance...
		public readonly string            CompilerPath;         // The path to the linker for linking executables
		public readonly Version           CLExeVersion;         // The version of cl.exe we're running
		public readonly string            LinkerPath;           // The path to the linker for linking executables
		public readonly string            LibraryLinkerPath;    // The path to the linker for linking libraries
		public readonly string            ResourceCompilerPath; // The path to the resource compiler

		private string _MSBuildPath = null;
		public string MSBuildPath // The path to MSBuild
		{
			get
			{
				if (_MSBuildPath == null)
				{
					_MSBuildPath = GetMSBuildToolPath();
				}
				return _MSBuildPath;
			}
		}

		/**
		 * Initializes environment variables required by toolchain. Different for 32 and 64 bit.
		 */
		public static VCEnvironment SetEnvironment(CPPTargetPlatform Platform)
		{
			if (EnvVars != null && EnvVars.Platform == Platform)
			{
				return EnvVars;
			}

			EnvVars = new VCEnvironment(Platform);
			return EnvVars;
		}

		private VCEnvironment(CPPTargetPlatform InPlatform)
		{
			Platform = InPlatform;

			// If Visual Studio is not installed, the Windows SDK path will be used, which also happens to be the same
			// directory. (It installs the toolchain into the folder where Visual Studio would have installed it to).
			BaseVSToolPath = WindowsPlatform.GetVSComnToolsPath();
			if (string.IsNullOrEmpty(BaseVSToolPath))
			{
				throw new BuildException("Visual Studio 2012, 2013 or 2015 must be installed in order to build this target.");
			}

			WindowsSDKDir        = FindWindowsSDKInstallationFolder(Platform);
			WindowsSDKExtensionDir = FindWindowsSDKExtensionInstallationFolder();
			NetFxSDKExtensionDir = FindNetFxSDKExtensionInstallationFolder();
			WindowsSDKExtensionHeaderLibVersion = FindWindowsSDKExtensionLatestVersion(WindowsSDKExtensionDir);
			PlatformVSToolPath = GetPlatformVSToolPath      (Platform, BaseVSToolPath);
			CompilerPath         = GetCompilerToolPath        (PlatformVSToolPath);
			CLExeVersion         = FindCLExeVersion           (CompilerPath);
			LinkerPath           = GetLinkerToolPath          (PlatformVSToolPath);
			LibraryLinkerPath    = GetLibraryLinkerToolPath   (PlatformVSToolPath);
			ResourceCompilerPath = GetResourceCompilerToolPath(Platform);

			// We ensure an extra trailing slash because of a user getting an odd error where the paths seemed to get concatenated wrongly:
			//
			// C:\Programme\Microsoft Visual Studio 12.0\Common7\Tools../../VC/bin/x86_amd64/vcvarsx86_amd64.bat
			//
			// https://answers.unrealengine.com/questions/233640/unable-to-create-project-files-for-48-preview-3.html
			//
			bool   bUse64BitCompiler             = Platform == CPPTargetPlatform.Win64 || Platform == CPPTargetPlatform.UWP;
			string BaseToolPathWithTrailingSlash = BaseVSToolPath.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
			string VCVarsBatchFile               = Path.Combine(BaseToolPathWithTrailingSlash, bUse64BitCompiler ? @"..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" : "vsvars32.bat");
			if (Platform == CPPTargetPlatform.UWP && UWPPlatform.bBuildForStore)
			{
				Utils.SetEnvironmentVariablesFromBatchFile(VCVarsBatchFile, "store");
			}
			else
			{
				Utils.SetEnvironmentVariablesFromBatchFile(VCVarsBatchFile);
			}

			// When targeting Windows XP on Visual Studio 2012+, we need to override the Windows SDK include and lib path set
			// by the batch file environment (http://blogs.msdn.com/b/vcblog/archive/2012/10/08/10357555.aspx)
			if (WindowsPlatform.IsWindowsXPSupported())
			{
				// Lib and bin folders have a x64 subfolder for 64 bit development.
				var ConfigSuffix = (Platform == CPPTargetPlatform.Win64) ? "\\x64" : "";

				Environment.SetEnvironmentVariable("PATH",    Utils.ResolveEnvironmentVariable(WindowsSDKDir + "bin" + ConfigSuffix + ";%PATH%"));
				Environment.SetEnvironmentVariable("LIB",     Utils.ResolveEnvironmentVariable(WindowsSDKDir + "lib" + ConfigSuffix + ";%LIB%"));
				Environment.SetEnvironmentVariable("INCLUDE", Utils.ResolveEnvironmentVariable(WindowsSDKDir + "include;%INCLUDE%"));
			}
		}

		/// <returns>The path to Windows SDK directory for the specified version.</returns>
		private static string FindWindowsSDKInstallationFolder(CPPTargetPlatform InPlatform)
		{
			// When targeting Windows XP on Visual Studio 2012+, we need to point at the older Windows SDK 7.1A that comes
			// installed with Visual Studio 2012 Update 1. (http://blogs.msdn.com/b/vcblog/archive/2012/10/08/10357555.aspx)
			string Version;
			if (WindowsPlatform.IsWindowsXPSupported())
			{
				Version = "v7.1A";
			}
			else switch (WindowsPlatform.Compiler)
			{
				case WindowsCompiler.VisualStudio2015:
					if( WindowsPlatform.bUseWindowsSDK10 )
					{ 
						Version = "v10.0";
					}
					else
					{
						Version = "v8.1";
					}
					break;

				case WindowsCompiler.VisualStudio2013:
					Version = "v8.1";
					break;

				case WindowsCompiler.VisualStudio2012:
					Version = "v8.0";
					break;

				default:
					throw new BuildException("Unexpected compiler setting when trying to determine Windows SDK folder");
			}

			// Based on VCVarsQueryRegistry
			string FinalResult = null;
			foreach (string IndividualVersion in Version.Split('|'))
			{
				var Result = Microsoft.Win32.Registry.GetValue(@"HKEY_CURRENT_USER\SOFTWARE\Microsoft\Microsoft SDKs\Windows\" + IndividualVersion, "InstallationFolder", null)
					?? Microsoft.Win32.Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\" + IndividualVersion, "InstallationFolder", null)
					?? Microsoft.Win32.Registry.GetValue(@"HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\" + IndividualVersion, "InstallationFolder", null);

				if (Result != null)
				{
					FinalResult = (string)Result;
					break;
				}
			}
			if (FinalResult == null)
			{
				throw new BuildException("Windows SDK {0} must be installed in order to build this target.", Version);
			}

			return FinalResult;
		}

		
		private static string FindNetFxSDKExtensionInstallationFolder()
		{
			string Version;
			switch (WindowsPlatform.Compiler)
			{
				case WindowsCompiler.VisualStudio2015:
					if( WindowsPlatform.bUseWindowsSDK10 )
					{ 
						Version = "4.6";
					}
					else
					{ 
						return string.Empty;
					}
					break;

				default:
					return string.Empty;
			}
			string FinalResult = string.Empty;
			var Result = Microsoft.Win32.Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\NETFXSDK\" + Version, "KitsInstallationFolder", null)
					  ?? Microsoft.Win32.Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\NETFXSDK\" + Version, "KitsInstallationFolder", null);

			if (Result != null)
			{
				FinalResult = ((string)Result).TrimEnd('\\');
			}
			return FinalResult;
		}

		private static string FindWindowsSDKExtensionInstallationFolder()
		{
			string Version;
			switch (WindowsPlatform.Compiler)
			{
				case WindowsCompiler.VisualStudio2015:
					if( WindowsPlatform.bUseWindowsSDK10 )
					{ 
						Version = "v10.0";
					}
					else
					{
						return string.Empty;
					}
					break;

				default:
					return string.Empty;
			}

			// Based on VCVarsQueryRegistry
			string FinalResult = null;
			{
				var Result = Microsoft.Win32.Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows SDKs\" + Version, "InstallationFolder", null)
						  ?? Microsoft.Win32.Registry.GetValue(@"HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows SDKs\" + Version, "InstallationFolder", null);
				if (Result == null)
				{
					 Result = Microsoft.Win32.Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\" + Version, "InstallationFolder", null)
						   ?? Microsoft.Win32.Registry.GetValue(@"HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\" + Version, "InstallationFolder", null);
				}
				if (Result != null)
				{
					FinalResult = ((string)Result).TrimEnd('\\');
				}
				
			}
			if (FinalResult == null)
			{
				FinalResult = string.Empty;
			}

			return FinalResult;
		}

		static Version FindWindowsSDKExtensionLatestVersion(string WindowsSDKExtensionDir)
		{
			Version LatestVersion = new Version(0, 0, 0, 0);

			if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2015 &&
				WindowsPlatform.bUseWindowsSDK10 &&
				!string.IsNullOrEmpty(WindowsSDKExtensionDir)  &&
				Directory.Exists(WindowsSDKExtensionDir))
			{
				string IncludesBaseDirectory = Path.Combine(WindowsSDKExtensionDir, "include");
				if (Directory.Exists(IncludesBaseDirectory))
				{
					string[] IncludeVersions = Directory.GetDirectories(IncludesBaseDirectory);
					foreach (string IncludeVersion in IncludeVersions)
					{
						string VersionString = Path.GetFileName(IncludeVersion);
						Version FoundVersion;
						if (Version.TryParse(VersionString, out FoundVersion) && FoundVersion > LatestVersion)
						{
							LatestVersion = FoundVersion;
						}
					}
				}
			}
			return LatestVersion;
		}

		/** Gets the path to the tool binaries for the specified platform. */
		static string GetPlatformVSToolPath(CPPTargetPlatform Platform, string BaseVSToolPath)
		{
			// Regardless of the target, if we're linking on a 64 bit machine, we want to use the 64 bit linker (it's faster than the 32 bit linker)
			//@todo.WIN32: Using the 64-bit linker appears to be broken at the moment.
			if (Platform == CPPTargetPlatform.Win64 || Platform == CPPTargetPlatform.UWP)
			{
				// Use the native 64-bit compiler if present, otherwise use the amd64-on-x86 compiler. VS2012 Express only includes the latter.
				var Result = Path.Combine(BaseVSToolPath, "../../VC/bin/amd64");
				if (Directory.Exists(Result))
				{
					return Result;
				}

				return Path.Combine(BaseVSToolPath, "../../VC/bin/x86_amd64");
			}

			return Path.Combine(BaseVSToolPath, "../../VC/bin");
		}

		/** Gets the path to the compiler. */
		static string GetCompilerToolPath(string PlatformVSToolPath)
		{
			// If we were asked to use Clang, then we'll redirect the path to the compiler to the LLVM installation directory
			if (WindowsPlatform.bCompileWithClang)
			{
				string Result;
				if( WindowsPlatform.bUseVCCompilerArgs )
				{ 
					// Use regular Clang compiler on Windows
					Result = Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.ProgramFilesX86 ), "LLVM", "msbuild-bin", "cl.exe" );
				}
				else
				{
					// Use 'clang-cl', a wrapper around Clang that supports Visual C++ compiler command-line arguments
					Result = Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.ProgramFilesX86 ), "LLVM", "bin", "clang.exe" );
				}
				if (!File.Exists(Result))
				{
					throw new BuildException( "Clang was selected as the Windows compiler, but LLVM/Clang does not appear to be installed.  Could not find: " + Result );
				}

				return Result;
			}

			return Path.Combine(PlatformVSToolPath, "cl.exe");
		}

		/// <returns>The version of the compiler.</returns>
		private static Version FindCLExeVersion(string CompilerExe)
		{
			// Check that the cl.exe exists (GetVersionInfo doesn't handle this well).
			if (!File.Exists(CompilerExe))
			{
				// By default VS2015 doesn't install the C++ toolchain. Help developers out with a special message.
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2015)
				{
					throw new BuildException("Failed to find cl.exe in the default toolchain directory " + CompilerExe + ". Please verify that \"Common Tools for Visual C++ 2015\" was selected when installing Visual Studio 2015.");
				}
				else
				{
					throw new BuildException("Failed to find cl.exe in the default toolchain directory " + CompilerExe + ". Please check that Visual Studio is correctly installed.");
				}
			}

			var ExeVersionInfo = FileVersionInfo.GetVersionInfo(CompilerExe);
			if (ExeVersionInfo == null)
			{
				throw new BuildException("Failed to read the version number of: " + CompilerExe);
			}

			return new Version(ExeVersionInfo.FileMajorPart, ExeVersionInfo.FileMinorPart, ExeVersionInfo.FileBuildPart, ExeVersionInfo.FilePrivatePart);
		}

		/** Gets the path to the linker. */
		static string GetLinkerToolPath(string PlatformVSToolPath)
		{
			// If we were asked to use Clang, then we'll redirect the path to the compiler to the LLVM installation directory
			if( WindowsPlatform.bCompileWithClang && WindowsPlatform.bAllowClangLinker )
			{
				var Result = Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.ProgramFilesX86 ), "LLVM", "bin", "lld.exe" );
				if( !File.Exists( Result ) )
				{
					throw new BuildException( "Clang was selected as the Windows compiler, but LLVM/Clang does not appear to be installed.  Could not find: " + Result );
				}

				return Result;
			}

			return Path.Combine(PlatformVSToolPath, "link.exe");
		}

		/** Gets the path to the library linker. */
		static string GetLibraryLinkerToolPath(string PlatformVSToolPath)
		{
			// Regardless of the target, if we're linking on a 64 bit machine, we want to use the 64 bit linker (it's faster than the 32 bit linker)
			//@todo.WIN32: Using the 64-bit linker appears to be broken at the moment.
			return Path.Combine(PlatformVSToolPath, "lib.exe");
		}

		/** Gets the path to the resource compiler's rc.exe for the specified platform. */
		string GetResourceCompilerToolPath(CPPTargetPlatform Platform)
		{
			// 64 bit -- we can use the 32 bit version to target 64 bit on 32 bit OS.
			if (Platform == CPPTargetPlatform.Win64 || Platform == CPPTargetPlatform.UWP)
			{
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2015 && WindowsPlatform.bUseWindowsSDK10)
				{
					return Path.Combine(WindowsSDKExtensionDir, "bin/x64/rc.exe");
				}
				else
				{
					return Path.Combine(WindowsSDKDir, "bin/x64/rc.exe");
				}
			}

			// @todo UWP: Verify that Windows XP will compile using VS 2015 (it should be supported)
			if (!WindowsPlatform.IsWindowsXPSupported())	// Windows XP requires use to force Windows SDK 7.1 even on the newer compiler, so we need the old path RC.exe
			{
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2015 && WindowsPlatform.bUseWindowsSDK10)
				{
					return Path.Combine(WindowsSDKExtensionDir, "bin/x86/rc.exe");
				}
				else
				{
					return Path.Combine(WindowsSDKDir, "bin/x86/rc.exe");
				}
			}
			return Path.Combine(WindowsSDKDir, "bin/rc.exe");
		}

		/** Gets the path to MSBuild. */
		static string GetMSBuildToolPath()
		{
			string FrameworkDirectory = Environment.GetEnvironmentVariable("FrameworkDir");
			string FrameworkVersion   = Environment.GetEnvironmentVariable("FrameworkVersion");
			if (FrameworkDirectory == null || FrameworkVersion == null)
			{
				throw new BuildException( "NOTE: Please ensure that 64bit Tools are installed with DevStudio - there is usually an option to install these during install" );
			}

			return Path.Combine(FrameworkDirectory, FrameworkVersion, "MSBuild.exe");
		}

		static VCEnvironment EnvVars = null;
	}
}
