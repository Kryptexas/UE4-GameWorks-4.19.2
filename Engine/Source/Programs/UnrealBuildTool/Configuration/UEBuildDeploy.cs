// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;

namespace UnrealBuildTool
{
	/// <summary>
	///  Base class to handle deploy of a target for a given platform
	/// </summary>
	public abstract class UEBuildDeploy
	{
		static Dictionary<UnrealTargetPlatform, UEBuildDeploy> BuildDeployDictionary = new Dictionary<UnrealTargetPlatform, UEBuildDeploy>();

		/**
		 *	Register the given platforms UEBuildDeploy instance
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform to register with
		 *	@param	InBuildDeploy		The UEBuildDeploy instance to use for the InPlatform
		 */
		public static void RegisterBuildDeploy(UnrealTargetPlatform InPlatform, UEBuildDeploy InBuildDeploy)
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

		/**
		 *	Retrieve the UEBuildDeploy instance for the given TargetPlatform
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	
		 *	@return	UEBuildDeploy		The instance of the build deploy
		 */
		public static UEBuildDeploy GetBuildDeploy(UnrealTargetPlatform InPlatform)
		{
			if (BuildDeployDictionary.ContainsKey(InPlatform) == true)
			{
				return BuildDeployDictionary[InPlatform];
			}
			// A platform does not *have* to have a deployment handler...
			return null;
		}

		/**
		 *	Register the platform with the UEBuildDeploy class
		 */
		public abstract void RegisterBuildDeploy();

		/**
		 *	Prepare the target for deployment
		 *	
		 *	@param	InTarget		The target for deployment
		 *	
		 *	@return	bool			true if successful, false if not
		 */
		public virtual bool PrepTargetForDeployment(UEBuildTarget InTarget)
		{
			return true;
		}

		/**
		 * Prepare the target for deployment
		 */
		public virtual bool PrepForUATPackageOrDeploy(string ProjectName, string ProjectDirectory, string ExecutablePath, string EngineDirectory, bool bForDistribution)
		{
			return true;
		}
	}
}
