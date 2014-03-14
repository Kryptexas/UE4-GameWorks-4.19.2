// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Reflection;
using AutomationTool;
using UnrealBuildTool;

/// <summary>
/// Helper command used for cooking.
/// </summary>
/// <remarks>
/// Command line parameters used by this command:
/// -clean
/// </remarks>
public partial class Project : CommandUtils
{
	#region Cook Command

	public static void Cook(ProjectParams Params)
	{
		if (!Params.Cook || Params.SkipCook)
		{
			return;
		}
		Params.ValidateAndLog();

		string UE4EditorExe = HostPlatform.Current.GetUE4ExePath(Params.UE4Exe);
		if (!FileExists(UE4EditorExe))
		{
			throw new AutomationException("Missing " + Params.UE4Exe + " executable. Needs to be built first.");
		}

		var PlatformsToCook = new List<string>();

		if (!Params.NoClient)
		{
			foreach ( var ClientPlatformInstance in Params.ClientTargetPlatformInstances )
			{
				PlatformsToCook.Add(ClientPlatformInstance.GetCookPlatform(false, Params.HasDedicatedServerAndClient, Params.CookFlavor));
			}
		}
		if (Params.DedicatedServer)
		{
			foreach ( var ServerPlatformInstance in Params.ServerTargetPlatformInstances )
			{
				PlatformsToCook.Add(ServerPlatformInstance.GetCookPlatform(true, false, Params.CookFlavor));
			}
		}

		if (Params.Clean.HasValue && Params.Clean.Value && !Params.IterativeCooking)
		{
			Log("Cleaning cooked data.");
			CleanupCookedData(PlatformsToCook, Params);
		}

		// cook the set of maps, or the run map, or nothing
		string[] Maps = null;
		if (Params.HasMapsToCook)
		{
			Maps = Params.MapsToCook.ToArray();
		}

		string[] Dirs = null;
		if (Params.HasDirectoriesToCook)
		{
			Dirs = Params.DirectoriesToCook.ToArray();
		}

		try
		{
			var CommandletParams = "-unattended -buildmachine -forcelogflush -AllowStdOutTraceEventType -Unversioned";
			if (Params.Compressed)
			{
				CommandletParams += " -Compressed";
			}
            if (Params.UseDebugParamForEditorExe)
            {
                CommandletParams += " -debug";
            }
			if (Params.Manifests)
			{
				CommandletParams += " -manifests";
			}
			if (Params.IterativeCooking)
			{
				CommandletParams += " -iterate";
			}
			CookCommandlet(Params.RawProjectPath, Params.UE4Exe, Maps, Dirs, CombineCommandletParams(PlatformsToCook.ToArray()), CommandletParams);
		}
		catch (Exception Ex)
		{
			// Delete cooked data (if any) as it may be incomplete / corrupted.
			Log("Cook failed. Deleting cooked data.");
			CleanupCookedData(PlatformsToCook, Params);
			throw Ex;
		}
	}

	private static void CleanupCookedData(List<string> PlatformsToCook, ProjectParams Params)
	{		
		var ProjectPath = Path.GetFullPath(Params.RawProjectPath);
		var CookedSandboxesPath = CombinePaths(GetDirectoryName(ProjectPath), "Saved", "Sandboxes", "Cooked-");
		var CleanDirs = new string[PlatformsToCook.Count];
		for (int DirIndex = 0; DirIndex < CleanDirs.Length; ++DirIndex)
		{
			CleanDirs[DirIndex] = CookedSandboxesPath + PlatformsToCook[DirIndex];
		}

		const bool bQuiet = true;
		DeleteDirectory(bQuiet, CleanDirs);
	}

	#endregion
}
