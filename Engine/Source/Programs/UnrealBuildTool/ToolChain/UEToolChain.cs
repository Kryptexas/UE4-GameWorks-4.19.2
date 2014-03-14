// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace UnrealBuildTool
{
	public abstract class UEToolChain
	{
		static Dictionary<CPPTargetPlatform, UEToolChain> CPPToolChainDictionary = new Dictionary<CPPTargetPlatform, UEToolChain>();

		public static void RegisterPlatformToolChain(CPPTargetPlatform InPlatform, UEToolChain InToolChain)
		{
			if (CPPToolChainDictionary.ContainsKey(InPlatform) == true)
			{
				Log.TraceInformation("RegisterPlatformToolChain Warning: Registering tool chain {0} for {1} when it is already set to {2}",
					InToolChain.ToString(), InPlatform.ToString(), CPPToolChainDictionary[InPlatform].ToString());
				CPPToolChainDictionary[InPlatform] = InToolChain;
			}
			else
			{
				CPPToolChainDictionary.Add(InPlatform, InToolChain);
			}
		}

		public static UEToolChain GetPlatformToolChain(CPPTargetPlatform InPlatform)
		{
			if (CPPToolChainDictionary.ContainsKey(InPlatform) == true)
			{
				return CPPToolChainDictionary[InPlatform];
			}
			throw new BuildException("GetPlatformToolChain: No tool chain found for {0}", InPlatform.ToString());
		}

		public abstract void RegisterToolChain();

		public abstract CPPOutput CompileCPPFiles(CPPEnvironment CompileEnvironment, List<FileItem> SourceFiles, string ModuleName);

		public virtual CPPOutput CompileRCFiles(CPPEnvironment Environment, List<FileItem> RCFiles)
		{
			CPPOutput Result = new CPPOutput();
			return Result;
		}

		public abstract FileItem LinkFiles(LinkEnvironment LinkEnvironment, bool bBuildImportLibraryOnly);

		public virtual void CompileCSharpProject(CSharpEnvironment CompileEnvironment, string ProjectFileName, string DestinationFile)
		{
		}

		/// <summary>
		/// Get the name of the response file for the current linker environment and output file
		/// </summary>
		/// <param name="LinkEnvironment"></param>
		/// <param name="OutputFile"></param>
		/// <returns></returns>
		public static string GetResponseFileName( LinkEnvironment LinkEnvironment, FileItem OutputFile )
		{
			// Construct a relative path for the intermediate response file
			string ResponseFileName = Path.Combine( LinkEnvironment.Config.IntermediateDirectory, Path.GetFileName( OutputFile.AbsolutePath ) + ".response" );
			if (UnrealBuildTool.HasUProjectFile())
			{
				// If this is the uproject being built, redirect the intermediate
				if (Utils.IsFileUnderDirectory( OutputFile.AbsolutePath, UnrealBuildTool.GetUProjectPath() ))
				{
					ResponseFileName = Path.Combine(
						UnrealBuildTool.GetUProjectPath(), 
						BuildConfiguration.PlatformIntermediateFolder,
						Path.GetFileNameWithoutExtension(UnrealBuildTool.GetUProjectFile()),
						LinkEnvironment.Config.TargetConfiguration.ToString(),
						Path.GetFileName(OutputFile.AbsolutePath) + ".response");
				}
			}
			// Convert the relative path to an absolute path
			ResponseFileName = Path.GetFullPath( ResponseFileName );

			return ResponseFileName;
		}

		/** Converts the passed in path from UBT host to compiler native format. */
		public virtual String ConvertPath(String OriginalPath)
		{
			return OriginalPath;
		}


		/// <summary>
		/// Called immediately after UnrealHeaderTool is executed to generated code for all UObjects modules.  Only is called if UnrealHeaderTool was actually run in this session.
		/// </summary>
		/// <param name="UObjectModules">List of UObject modules we generated code for.</param>
		public virtual void PostCodeGeneration(UEBuildTarget Target,  UHTManifest Manifest)
		{
		}

		public virtual void PreBuildSync()
		{
		}

		public virtual void PostBuildSync(UEBuildTarget Target)
		{
		}

		public virtual void SetUpGlobalEnvironment()
		{
		}


		protected void RunUnrealHeaderToolIfNeeded()
		{

		}
	};
}