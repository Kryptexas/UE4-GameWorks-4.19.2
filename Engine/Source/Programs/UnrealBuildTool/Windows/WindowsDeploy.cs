// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Diagnostics;

namespace UnrealBuildTool
{
	/// <summary>
	///  Base class to handle deploy of a target for a given platform
	/// </summary>
	public class BaseWindowsDeploy : UEBuildDeploy
	{

        // public virtual bool PrepForUATPackageOrDeploy(FileReference ProjectFile, string ProjectName, string ProjectDirectory, string ExecutablePath, string EngineDirectory, bool bForDistribution, string CookFlavor, bool bIsDataDeploy)
        public bool PrepForUATPackageOrDeploy(FileReference ProjectFile, string ProjectName, string ProjectDirectory, string ExecutablePath, string EngineDirectory, bool bForDistribution, string CookFlavor, bool bIsDataDeploy)
        {
            string ApplicationIconPath = Path.Combine(ProjectDirectory, "Build/Windows/Application.ico");
            // Does a Project icon exist?
            if (!File.Exists(ApplicationIconPath))
            {
                // Also check for legacy location
                ApplicationIconPath = Path.Combine(ProjectDirectory, "Source", ProjectName, "Resources", "Windows", ProjectName + ".ico");
                if (!File.Exists(ApplicationIconPath))
                {
                    // point to the default UE4 icon instead
                    ApplicationIconPath = Path.Combine(EngineDirectory, "Source/Runtime/Launch/Resources/Windows/UE4.ico");
                }
            }
            // sets the icon on the original exe this will be used in the task bar when the bootstrap exe runs
            if (File.Exists(ApplicationIconPath))
            {
                GroupIconResource GroupIcon = null;
                GroupIcon = GroupIconResource.FromIco(ApplicationIconPath);

                // Update the icon on the original exe because this will be used when the game is running in the task bar
                using (ModuleResourceUpdate Update = new ModuleResourceUpdate(ExecutablePath, false))
                {
                    const int IconResourceId = 123; // As defined in Engine\Source\Runtime\Launch\Resources\Windows\resource.h
                    if (GroupIcon != null)
                    {
                        Update.SetIcons(IconResourceId, GroupIcon);
                    }
                }
            }
            return true;
        }




		public override bool PrepTargetForDeployment(UEBuildDeployTarget InTarget)
		{
			if ((InTarget.TargetType != TargetRules.TargetType.Editor && InTarget.TargetType != TargetRules.TargetType.Program) && (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Win32 || BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Win64))
			{
				string InAppName = InTarget.AppName;
				Log.TraceInformation("Prepping {0} for deployment to {1}", InAppName, InTarget.Platform.ToString());
				System.DateTime PrepDeployStartTime = DateTime.UtcNow;

                PrepForUATPackageOrDeploy(InTarget.ProjectFile, InAppName, InTarget.ProjectDirectory.FullName, InTarget.OutputPath.FullName, BuildConfiguration.RelativeEnginePath, false, "", false);
			}
			return true;
		}
	}
}
