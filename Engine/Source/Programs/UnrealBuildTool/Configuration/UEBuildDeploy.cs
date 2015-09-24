// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;

namespace UnrealBuildTool
{
	public interface IUEBuildDeploy
	{
		/// <summary>
		/// Register the platform with the UEBuildDeploy class.
		/// </summary>
		void RegisterBuildDeploy();

		/// <summary>
		/// Prepare the target for deployment.
		/// </summary>
		/// <param name="InTarget">The target for deployment.</param>
		/// <returns>bool True if successful, false if not.</returns>
		bool PrepTargetForDeployment(UEBuildTarget InTarget);

		/// <summary>
		/// Prepare the target for deployment.
		/// </summary>
		bool PrepForUATPackageOrDeploy(string ProjectName, string ProjectDirectory, string ExecutablePath, string EngineDirectory, bool bForDistribution, string CookFlavor, bool bIsDataDeploy);
	}

	/// <summary>
	///  Base class to handle deploy of a target for a given platform
	/// </summary>
	public abstract class UEBuildDeploy : IUEBuildDeploy
	{
		static Dictionary<UnrealTargetPlatform, IUEBuildDeploy> BuildDeployDictionary = new Dictionary<UnrealTargetPlatform, IUEBuildDeploy>();

		/// <summary>
		/// Register the given platforms UEBuildDeploy instance
		/// </summary>
		/// <param name="InPlatform">  The UnrealTargetPlatform to register with</param>
		/// <param name="InBuildDeploy"> The UEBuildDeploy instance to use for the InPlatform</param>
		public static void RegisterBuildDeploy(UnrealTargetPlatform InPlatform, IUEBuildDeploy InBuildDeploy)
		{
			if (BuildDeployDictionary.ContainsKey(InPlatform) == true)
			{
				Log.TraceWarning("RegisterBuildDeply Warning: Registering build deploy {0} for {1} when it is already set to {2}",
					InBuildDeploy.ToString(), InPlatform.ToString(), BuildDeployDictionary[InPlatform].ToString());
				BuildDeployDictionary[InPlatform] = InBuildDeploy;
			}
			else
			{
				BuildDeployDictionary.Add(InPlatform, InBuildDeploy);
			}
		}

		/// <summary>
		/// Retrieve the UEBuildDeploy instance for the given TargetPlatform
		/// </summary>
		/// <param name="InPlatform">  The UnrealTargetPlatform being built</param>
		/// <returns>UEBuildDeploy  The instance of the build deploy</returns>
		public static IUEBuildDeploy GetBuildDeploy(UnrealTargetPlatform InPlatform)
		{
			if (BuildDeployDictionary.ContainsKey(InPlatform) == true)
			{
				return BuildDeployDictionary[InPlatform];
			}
			// A platform does not *have* to have a deployment handler...
			return null;
		}

		/// <summary>
		/// Register the platform with the UEBuildDeploy class
		/// </summary>
		public abstract void RegisterBuildDeploy();

		/// <summary>
		/// Prepare the target for deployment
		/// </summary>
		/// <param name="InTarget"> The target for deployment</param>
		/// <returns>bool   true if successful, false if not</returns>
		public virtual bool PrepTargetForDeployment(UEBuildTarget InTarget)
		{
			return true;
		}

		/// <summary>
		/// Prepare the target for deployment
		/// </summary>
		public virtual bool PrepForUATPackageOrDeploy(string ProjectName, string ProjectDirectory, string ExecutablePath, string EngineDirectory, bool bForDistribution, string CookFlavor, bool bIsDataDeploy)
		{
			return true;
		}
	}
}
