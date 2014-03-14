// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;
using System.Xml;

namespace UnrealBuildTool
{
	class AndroidPlatform : UEBuildPlatform
	{
		// The current architecture - affects everything about how UBT operates on Android
		private static string ActiveArchitecture = Utils.GetStringEnvironmentVariable("ue.AndroidArchitecture", "-armv7");
		public override string GetActiveArchitecture()
		{
			// by default, use an empty architecture (which is really just a modifer to the platform for some paths/names)
			return ActiveArchitecture;
		}

		public override bool HasRequiredSDKsInstalled()
		{
			string NDKPath = Environment.GetEnvironmentVariable("NDKROOT");

			// we don't have an NDKROOT specified
			if (String.IsNullOrEmpty(NDKPath))
			{
				return false;
			}

			NDKPath = NDKPath.Replace("\"", "");

			// can't find llvm-3.3 or llvm-3.1 in the toolchains
			if (!Directory.Exists(Path.Combine(NDKPath, @"toolchains\llvm-3.3")) && !Directory.Exists(Path.Combine(NDKPath, @"toolchains\llvm-3.1")))
			{
				return false;
			}

			return true;
		}

		public override void RegisterBuildPlatform()
		{
			if ((ProjectFileGenerator.bGenerateProjectFiles == true) || (HasRequiredSDKsInstalled() == true))
			{
				bool bRegisterBuildPlatform = true;

				string EngineSourcePath = Path.Combine(ProjectFileGenerator.EngineRelativePath, "Source");
				string AndroidTargetPlatformFile = Path.Combine(EngineSourcePath, "Developer", "Android", "AndroidTargetPlatform", "AndroidTargetPlatform.Build.cs");

				if (File.Exists(AndroidTargetPlatformFile) == false)
				{
					bRegisterBuildPlatform = false;
				}

				if (bRegisterBuildPlatform == true)
				{
					// Register this build platform
					Log.TraceVerbose("        Registering for {0}", UnrealTargetPlatform.Android.ToString());
					UEBuildPlatform.RegisterBuildPlatform(UnrealTargetPlatform.Android, this);
					UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.Android, UnrealPlatformGroup.Android);
				}
			}
		}

		public override CPPTargetPlatform GetCPPTargetPlatform(UnrealTargetPlatform InUnrealTargetPlatform)
		{
			switch (InUnrealTargetPlatform)
			{
				case UnrealTargetPlatform.Android:
					return CPPTargetPlatform.Android;
			}
			throw new BuildException("AndroidPlatform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
		}

		public override string GetBinaryExtension(UEBuildBinaryType InBinaryType)
		{
			switch (InBinaryType)
			{
				case UEBuildBinaryType.DynamicLinkLibrary:
					return ".so";
				case UEBuildBinaryType.Executable:
					return ".so";
				case UEBuildBinaryType.StaticLibrary:
					return ".a";
			}
			return base.GetBinaryExtension(InBinaryType);
		}

		public override string GetDebugInfoExtension( UEBuildBinaryType InBinaryType )
		{
			return "";
		}

		public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			ValidateUEBuildConfiguration();
			//BuildConfiguration.bDeployAfterCompile = true;
		}

		public override void ValidateUEBuildConfiguration()
		{
			BuildConfiguration.bUseUnityBuild = true;

			UEBuildConfiguration.bCompileLeanAndMeanUE = true;
			UEBuildConfiguration.bCompilePhysX = true;
			UEBuildConfiguration.bCompileAPEX = false;

			UEBuildConfiguration.bBuildEditor = false;
			UEBuildConfiguration.bBuildDeveloperTools = false;
			UEBuildConfiguration.bCompileSimplygon = false;
			UEBuildConfiguration.bCompileNetworkProfiler = false;

			UEBuildConfiguration.bCompileRecast = true;

			// Don't stop compilation at first error...
			BuildConfiguration.bStopXGECompilationAfterErrors = true;

			BuildConfiguration.bUseSharedPCHs = false;
		}

		public override bool ShouldCompileMonolithicBinary(UnrealTargetPlatform InPlatform)
		{
			// This platform currently always compiles monolithic
			return true;
		}

		public override bool ShouldUsePDBFiles(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration, bool bCreateDebugInfo)
		{
			return true;
		}

		public override bool ShouldNotBuildEditor(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return true;
		}

		public override bool BuildRequiresCookedData(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return true;
		}

		public override bool RequiresDeployPrepAfterCompile()
		{
			return true;
		}

		public override void GetExtraModules(TargetInfo Target, UEBuildTarget BuildTarget, ref List<string> PlatformExtraModules)
		{
		}

		public override void ModifyNewlyLoadedModule(UEBuildModule InModule, TargetInfo Target)
		{
			if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
			{
				bool bBuildShaderFormats = UEBuildConfiguration.bForceBuildShaderFormats;
				if (!UEBuildConfiguration.bBuildRequiresCookedData)
				{
					if (InModule.ToString() == "Engine")
					{
						if (UEBuildConfiguration.bBuildDeveloperTools)
						{
                            InModule.AddPlatformSpecificDynamicallyLoadedModule("AndroidTargetPlatform");
							InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_PVRTCTargetPlatform");
							InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_ATCTargetPlatform");
							InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_DXTTargetPlatform");
                            InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_ETC1TargetPlatform");
                            InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_ETC2TargetPlatform");
                        }
					}
					else if (InModule.ToString() == "UnrealEd")
					{
						InModule.AddPlatformSpecificDynamicallyLoadedModule("AndroidPlatformEditor");
					}
					else if (InModule.ToString() == "TargetPlatform")
					{
						bBuildShaderFormats = true;
                        InModule.AddDynamicallyLoadedModule("TextureFormatPVR");
						InModule.AddDynamicallyLoadedModule("TextureFormatDXT");
                        InModule.AddPlatformSpecificDynamicallyLoadedModule("TextureFormatAndroid");    // ATITC, ETC1 and ETC2
                        if (UEBuildConfiguration.bBuildDeveloperTools)
						{
							//InModule.AddDynamicallyLoadedModule("AudioFormatADPCM");	//@todo android: android audio
						}
					}
				}

				// allow standalone tools to use targetplatform modules, without needing Engine
				if (UEBuildConfiguration.bForceBuildTargetPlatforms)
				{
                    InModule.AddPlatformSpecificDynamicallyLoadedModule("AndroidTargetPlatform");
					InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_PVRTCTargetPlatform");
					InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_ATCTargetPlatform");
					InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_DXTTargetPlatform");
                    InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_ETC1TargetPlatform");
                    InModule.AddPlatformSpecificDynamicallyLoadedModule("Android_ETC2TargetPlatform");
                }

				if (bBuildShaderFormats)
				{
					//InModule.AddDynamicallyLoadedModule("ShaderFormatAndroid");		//@todo android: ShaderFormatAndroid
				}
			}
		}

		public override void SetUpEnvironment(UEBuildTarget InBuildTarget)
		{
			// we want gcc toolchain 4.8, but fall back to 4.6 for now if it doesn't exist
			string NDKPath = Environment.GetEnvironmentVariable("NDKROOT");
			NDKPath = NDKPath.Replace("\"", "");

			string GccVersion = "4.8";
			if (!Directory.Exists(Path.Combine(NDKPath, @"sources\cxx-stl\gnu-libstdc++\4.8")))
			{
				GccVersion = "4.6";
			}


			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_DESKTOP=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_64BITS=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_CAN_SUPPORT_EDITORONLY_DATA=0");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_OGGVORBIS=1");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("UNICODE");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_UNICODE");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_ANDROID=1");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("ANDROID=1");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_DATABASE_SUPPORT=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_EDITOR=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("USE_NULL_RHI=0");

			if (InBuildTarget.GlobalCompileEnvironment.Config.TargetArchitecture == "-armv7")
			{
				InBuildTarget.GlobalCompileEnvironment.Config.SystemIncludePaths.Add("$(NDKROOT)/sources/cxx-stl/gnu-libstdc++/" + GccVersion + "/include");
				InBuildTarget.GlobalCompileEnvironment.Config.SystemIncludePaths.Add("$(NDKROOT)/sources/cxx-stl/gnu-libstdc++/" + GccVersion + "/libs/armeabi-v7a/include");
			}
			else
			{
				InBuildTarget.GlobalCompileEnvironment.Config.SystemIncludePaths.Add("$(NDKROOT)/sources/cxx-stl/gnu-libstdc++/" + GccVersion + "/include");
				InBuildTarget.GlobalCompileEnvironment.Config.SystemIncludePaths.Add("$(NDKROOT)/sources/cxx-stl/gnu-libstdc++/" + GccVersion + "/libs/x86/include");
			}

			InBuildTarget.GlobalCompileEnvironment.Config.SystemIncludePaths.Add("$(NDKROOT)/sources/android/native_app_glue");
			InBuildTarget.GlobalCompileEnvironment.Config.SystemIncludePaths.Add("$(NDKROOT)/sources/android/cpufeatures");

			// link with Android libraries.
			InBuildTarget.GlobalLinkEnvironment.Config.LibraryPaths.Add("$(NDKROOT)/sources/cxx-stl/stlport/libs/armeabi-v7a");

			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("stdc++");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("gcc");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("z");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("c");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("m");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("log");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("dl");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("GLESv2");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("EGL");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("OpenSLES");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("android");

			UEBuildConfiguration.bCompileSimplygon = false;
			BuildConfiguration.bDeployAfterCompile = true;
		}

		public override bool ShouldCreateDebugInfo(UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration)
		{
			switch (Configuration)
			{
				case UnrealTargetConfiguration.Development:
				case UnrealTargetConfiguration.Shipping:
				case UnrealTargetConfiguration.Test:
				case UnrealTargetConfiguration.Debug:
				default:
					return true;
			};
		}

		public override void SetupBinaries(UEBuildTarget InBuildTarget)
		{
			// dangerously fast mode doesn't generate stub files
			if (!IOSToolChain.bUseDangerouslyFastMode)
			{
				List<UEBuildBinary> NewBinaries = new List<UEBuildBinary>();
				// add the .apk to the binaries
				foreach (var Binary in InBuildTarget.AppBinaries)
				{
					// make a binary that just points to the .stub of this executable
					UEBuildBinaryConfiguration NewConfig = new UEBuildBinaryConfiguration(
																	InType: Binary.Config.Type,
																	InOutputFilePath: Path.ChangeExtension(Binary.Config.OutputFilePath, ".apk"),
																	InIntermediateDirectory: Binary.Config.IntermediateDirectory,
																	bInCreateImportLibrarySeparately: Binary.Config.bCreateImportLibrarySeparately,
																	bInAllowExports: Binary.Config.bAllowExports,
																	InModuleNames: Binary.Config.ModuleNames);

					NewBinaries.Add(new UEStubDummyBinary(InBuildTarget, NewConfig));
				}

				InBuildTarget.AppBinaries.AddRange(NewBinaries);
			}
		}
	}

	/// <summary>
	/// A .stub that has the executable and metadata included
	/// Note that this doesn't actually build anything. It just makes the .apk get checked in
	/// THis could be shared with IOS, etc
	/// </summary>
	class UEApkDummyBinary : UEBuildBinary
	{
		/// <summary>
		/// Create an instance initialized to the given configuration
		/// </summary>
		/// <param name="InConfig">The build binary configuration to initialize the instance to</param>
		public UEApkDummyBinary(UEBuildTarget InTarget, UEBuildBinaryConfiguration InConfig)
			: base(InTarget, InConfig)
		{
		}

		/// <summary>
		/// Builds the binary.
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <returns></returns>
		public override IEnumerable<FileItem> Build(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment)
		{
			// generate the .apk!
			return new FileItem[] { FileItem.GetItemByPath(Config.OutputFilePath) };
		}

		/// <summary>
		/// Writes an XML summary of the build environment for this binary
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <returns></returns>
		public override void WriteBuildEnvironment(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment, XmlWriter Writer)
		{
		}
	}
}
