// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using Tools.DotNETCommon;

namespace UnrealBuildTool
{
	/// <summary>
	/// Information about a target, passed along when creating a module descriptor
	/// </summary>
	public class TargetInfo
	{
		/// <summary>
		/// Name of the target
		/// </summary>
		public readonly string Name;

		/// <summary>
		/// The platform that the target is being built for
		/// </summary>
		public readonly UnrealTargetPlatform Platform;

		/// <summary>
		/// The configuration being built
		/// </summary>
		public readonly UnrealTargetConfiguration Configuration;

		/// <summary>
		/// Architecture that the target is being built for (or an empty string for the default)
		/// </summary>
		public readonly string Architecture;

		/// <summary>
		/// The project containing the target
		/// </summary>
		public readonly FileReference ProjectFile;

		/// <summary>
		/// The current build version
		/// </summary>
		public readonly ReadOnlyBuildVersion Version;

		/// <summary>
		/// The type of the target (if known)
		/// </summary>
		public readonly TargetType? Type;

		/// <summary>
		/// Whether the target is monolithic or not (if known)
		/// </summary>
		public readonly bool? bIsMonolithic;

		/// <summary>
		/// Constructs a TargetInfo for passing to the TargetRules constructor.
		/// </summary>
		/// <param name="Name">Name of the target being built</param>
		/// <param name="Platform">The platform that the target is being built for</param>
		/// <param name="Configuration">The configuration being built</param>
		/// <param name="Architecture">The architecture being built for</param>
		/// <param name="ProjectFile">Path to the project file containing the target</param>
		/// <param name="Version">The current build version</param>
		public TargetInfo(string Name, UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration, string Architecture, FileReference ProjectFile, ReadOnlyBuildVersion Version)
		{
			this.Name = Name;
			this.Platform = Platform;
			this.Configuration = Configuration;
			this.Architecture = Architecture;
			this.ProjectFile = ProjectFile;
			this.Version = Version;
		}

		/// <summary>
		/// Constructs a TargetInfo for passing to the ModuleRules constructor.
		/// </summary>
		/// <param name="Rules">The constructed target rules object. This is null when passed into a TargetRules constructor, but should be valid at all other times.</param>
		public TargetInfo(ReadOnlyTargetRules Rules)
		{
			this.Name = Rules.Name;
			this.Platform = Rules.Platform;
			this.Configuration = Rules.Configuration;
			this.Architecture = Rules.Architecture;
			this.ProjectFile = Rules.ProjectFile;
			this.Version = Rules.Version;
			this.Type = Rules.Type;
			this.bIsMonolithic = (Rules.LinkType == TargetLinkType.Monolithic);
		}

		/// <summary>
		/// True if the target type is a cooked game.
		/// </summary>
		public bool IsCooked
		{
			get
			{
				if (!Type.HasValue)
				{
					throw new BuildException("Trying to access TargetInfo.IsCooked when TargetInfo.Type is not set. Make sure IsCooked is used only in ModuleRules.");
				}
				return Type == TargetType.Client || Type == TargetType.Game || Type == TargetType.Server;
			}
		}

		/// <summary>
		/// True if the target type is a monolithic binary
		/// </summary>
		public bool IsMonolithic
		{
			get
			{
				if (!bIsMonolithic.HasValue)
				{
					throw new BuildException("Trying to access TargetInfo.IsMonolithic when bIsMonolithic is not set. Make sure IsMonolithic is used only in ModuleRules.");
				}
				return bIsMonolithic.Value;
			}
		}
	}
}
