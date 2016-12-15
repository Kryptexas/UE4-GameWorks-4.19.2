// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	/// <summary>
	/// Contains information about a target required to deploy it. Written at build time, and read back in when UBT needs to run the deploy step.
	/// </summary>
	public class UEBuildDeployTarget
	{
		/// <summary>
		/// Path to the project file
		/// </summary>
		public readonly FileReference ProjectFile;

		/// <summary>
		/// Path to the .target.cs
		/// </summary>
		public readonly FileReference TargetFile;

		/// <summary>
		/// The shared app name for this target (eg. UE4Editor)
		/// </summary>
		public readonly string AppName;

		/// <summary>
		/// The name of this target
		/// </summary>
		public readonly string TargetName;

		/// <summary>
		/// Type of the target to build
		/// </summary>
		public readonly TargetRules.TargetType TargetType;

		/// <summary>
		/// The platform being built
		/// </summary>
		public readonly UnrealTargetPlatform Platform;

		/// <summary>
		/// The configuration being built
		/// </summary>
		public readonly UnrealTargetConfiguration Configuration;

		/// <summary>
		/// The output path
		/// </summary>
		public FileReference OutputPath
		{
			get
			{
				if (OutputPaths.Count != 1)
				{
					throw new BuildException("Attempted to use UEBuildDeployTarget.OutputPath property, but there are multiple (or no) OutputPaths. You need to handle multiple in the code that called this (size = {0})", OutputPaths.Count);
				}
				return OutputPaths[0];
			}
		}

		/// <summary>
		/// The full list of output paths, for platforms that support building multiple binaries simultaneously
		/// </summary>
		public readonly List<FileReference> OutputPaths;

		/// <summary>
		/// Path to the directory for engine intermediates. May be under the project directory if not being built for the shared build environment.
		/// </summary>
		public readonly DirectoryReference EngineIntermediateDirectory;

		/// <summary>
		/// The project directory, or engine directory for targets without a project file.
		/// </summary>
		public readonly DirectoryReference ProjectDirectory;

		/// <summary>
		/// Path to the generated build receipt.
		/// </summary>
		public readonly string BuildReceiptFileName;
		
		/// <summary>
		/// Whether to output build products to the Engine/Binaries folder.
		/// </summary>
		public readonly bool bOutputToEngineBinaries;

		/// <summary>
		/// Construct the deployment info from a target
		/// </summary>
		/// <param name="Target">The target being built</param>
		public UEBuildDeployTarget(UEBuildTarget Target)
		{
			this.ProjectFile = Target.ProjectFile;
			this.TargetFile = Target.TargetCsFilename;
			this.AppName = Target.AppName;
			this.TargetName = Target.TargetName;
			this.TargetType = Target.TargetType;
			this.Platform = Target.Platform;
			this.Configuration = Target.Configuration;
			this.OutputPaths = new List<FileReference>(Target.OutputPaths);
			this.EngineIntermediateDirectory = Target.EngineIntermediateDirectory;
			this.ProjectDirectory = Target.ProjectDirectory;
			this.BuildReceiptFileName = Target.BuildReceiptFileName;
			this.bOutputToEngineBinaries = Target.Rules.bOutputToEngineBinaries;
		}

		/// <summary>
		/// Read the deployment info from a file on disk
		/// </summary>
		/// <param name="Location">Path to the file to read</param>
		public UEBuildDeployTarget(FileReference Location)
		{
			using (BinaryReader Reader = new BinaryReader(File.Open(Location.FullName, FileMode.Open, FileAccess.Read, FileShare.Read)))
			{
				ProjectFile = Reader.ReadFileReference();
				TargetFile = Reader.ReadFileReference();
				AppName = Reader.ReadString();
				TargetName = Reader.ReadString();
				TargetType = (TargetRules.TargetType)Reader.ReadInt32();
				Platform = (UnrealTargetPlatform)Reader.ReadInt32();
				Configuration = (UnrealTargetConfiguration)Reader.ReadInt32();
				OutputPaths = Reader.ReadList(x => x.ReadFileReference());
				EngineIntermediateDirectory = Reader.ReadDirectoryReference();
				ProjectDirectory = Reader.ReadDirectoryReference();
				BuildReceiptFileName = Reader.ReadString();
				bOutputToEngineBinaries = Reader.ReadBoolean();
			}
		}

		/// <summary>
		/// Write the deployment info to a file on disk
		/// </summary>
		/// <param name="Location">File to write to</param>
		public void Write(FileReference Location)
		{
			Location.Directory.CreateDirectory();
			using (BinaryWriter Writer = new BinaryWriter(File.Open(Location.FullName, FileMode.Create, FileAccess.Write, FileShare.Read)))
			{
				Writer.Write(ProjectFile);
				Writer.Write(TargetFile);
				Writer.Write(AppName);
				Writer.Write(TargetName);
				Writer.Write((Int32)TargetType);
				Writer.Write((Int32)Platform);
				Writer.Write((Int32)Configuration);
				Writer.Write(OutputPaths, (x, i) => x.Write(i));
				Writer.Write(EngineIntermediateDirectory);
				Writer.Write(ProjectDirectory);
				Writer.Write(BuildReceiptFileName);
				Writer.Write(bOutputToEngineBinaries);
			}
		}
	}

	/// <summary>
	/// Base class to handle deploy of a target for a given platform
	/// </summary>
	public abstract class UEBuildDeploy
	{
		/// <summary>
		/// Prepare the target for deployment
		/// </summary>
		/// <param name="InTarget"> The target for deployment</param>
		/// <returns>bool   true if successful, false if not</returns>
		public virtual bool PrepTargetForDeployment(UEBuildDeployTarget InTarget)
		{
			return true;
		}
	}
}
