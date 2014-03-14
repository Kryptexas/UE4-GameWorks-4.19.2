// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;


public abstract class BaseWinPlatform : Platform
{
	public BaseWinPlatform(UnrealTargetPlatform P)
		: base(P)
	{ 
	}

    private int StageExecutable(string Ext, DeploymentContext SC, string InPath, string Wildcard = "*", bool bRecursive = true, string[] ExcludeWildcard = null, string NewPath = null, bool bAllowNone = false, StagedFileType InStageFileType = StagedFileType.NonUFS, string NewName = null)
	{
        int Result = SC.StageFiles(InStageFileType, InPath, Wildcard + Ext, bRecursive, ExcludeWildcard, NewPath, bAllowNone, true, (NewName == null)? null : (NewName + Ext));
		if (Result > 0)
		{
			SC.StageFiles(StagedFileType.DebugNonUFS, InPath, Wildcard + "pdb", bRecursive, ExcludeWildcard, NewPath, true, true, (NewName == null) ? null : (NewName + "pdb"));
			SC.StageFiles(StagedFileType.DebugNonUFS, InPath, Wildcard + "map", bRecursive, ExcludeWildcard, NewPath, true, true, (NewName == null) ? null : (NewName + "map"));
		}
		return Result;
	}

	public override void GetFilesToDeployOrStage(ProjectParams Params, DeploymentContext SC)
	{
		// Engine non-ufs (binaries)

        if (SC.bStageCrashReporter) 
        {
            StageExecutable("exe", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "AutoReporter.");
            StageExecutable("config", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "AutoReporter.exe.");
            StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "AutoReporter.XmlSerializers.");            
            StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "CrashReportCommon.");
            StageExecutable("exe", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "CrashReportInput.");
            StageExecutable("config", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "CrashReportInput.exe.");
            StageExecutable("exe", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "CrashReportUploader.");
            StageExecutable("config", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "CrashReportUploader.exe.");
            StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "DotNETUtilities.");
            StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/DotNET"), "Ionic.Zip.Reduced.");
            StageExecutable("exe", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir), "CrashReportClient.");
        }

		//todo we need to support shipping and test executables
		//todo this should all be partially based on UBT manifests and not hard coded
		//monolithic assumption
		StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Ogg", SC.PlatformDir), "*.", true);
		StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Vorbis", SC.PlatformDir), "*.", true);
		string PhysXVer = "VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();
        string ApexVer = "VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();
		if(SC.StageTargetConfigurations.Contains(UnrealTargetConfiguration.Debug) && !Params.Rocket)
		{
            StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/PhysX/APEX-1.3", SC.PlatformDir, ApexVer), "*DEBUG*.", true);
			StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/PhysX/PhysX-3.3", SC.PlatformDir, PhysXVer), "*DEBUG*.", true);
			StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/PhysX/PhysX-3.3", SC.PlatformDir, PhysXVer), "nvToolsExt*.", true);
		}
		if (SC.StageTargetConfigurations.Any(x => x != UnrealTargetConfiguration.Debug) || Params.Rocket)
		{
            StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/PhysX/APEX-1.3", SC.PlatformDir, ApexVer), "*.", true, new string[] { "*DEBUG*.*", "*CHECKED*.*" });
			StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/PhysX/PhysX-3.3", SC.PlatformDir, PhysXVer), "*.", true, new string[] { "*DEBUG*.*", "*CHECKED*.*" });
		}

        if (Params.bUsesSteam)
        {
			string SteamVersion = "Steamv128";

			// Check that the TPS directory exists. We don't distribute binaries for Steam in Rocket.
			if (Directory.Exists(CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion)))
			{
				if (SC.StageTargetPlatform.PlatformType == UnrealTargetPlatform.Win32)
				{
					StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion, SC.PlatformDir), "steam_api.");
					if (SC.DedicatedServer)
					{
						StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion, SC.PlatformDir), "steamclient.");
						StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion, SC.PlatformDir), "tier0_s.");
						StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion, SC.PlatformDir), "vstdlib_s.");
					}
				}
				else
				{
					StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion, SC.PlatformDir), "steam_api64.");
					if (SC.DedicatedServer)
					{
						StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion, SC.PlatformDir), "steamclient64.");
						StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion, SC.PlatformDir), "tier0_s64.");
						StageExecutable("dll", SC, CommandUtils.CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/Steamworks/" + SteamVersion, SC.PlatformDir), "vstdlib_s64.");
					}
				}
			}
        }

        // Copy the splash screen, windows specific
        SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Content/Splash"), "Splash.bmp", false, null, null, true);

		List<string> Exes = GetExecutableNames(SC);

        // the first exe is the "main" one, the rest are marked as debug files
        StagedFileType WorkingFileType = StagedFileType.NonUFS;

        foreach (var Exe in Exes)
        {

            if (Exe.StartsWith(CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir)))
            {
                // remap the project root. For Rocket executables, rename the executable to the game name.
				if (Params.Rocket && Exe == Exes[0])
				{
					StageExecutable("exe", SC, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir), Path.GetFileNameWithoutExtension(Exe) + ".", true, null, CommandUtils.CombinePaths(SC.RelativeProjectRootForStage, "Binaries", SC.PlatformDir), false, WorkingFileType, SC.ShortProjectName + ".");
				}
				else
				{
					StageExecutable("exe", SC, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir), Path.GetFileNameWithoutExtension(Exe) + ".", true, null, CommandUtils.CombinePaths(SC.RelativeProjectRootForStage, "Binaries", SC.PlatformDir), false, WorkingFileType);
				}
            }
            else if (Exe.StartsWith(CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir)))
            {
				// Move the executable for non-code rocket projects into the game directory, using the game name, so it can figure out the UProject to look for and is consitent with code projects.
				if (Params.Rocket && Exe == Exes[0])
				{
					StageExecutable("exe", SC, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir), Path.GetFileNameWithoutExtension(Exe) + ".", true, null, CommandUtils.CombinePaths(SC.RelativeProjectRootForStage, "Binaries", SC.PlatformDir), false, WorkingFileType, SC.ShortProjectName + ".");
				}
				else
				{
					StageExecutable("exe", SC, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir), Path.GetFileNameWithoutExtension(Exe) + ".", true, null, null, false, WorkingFileType);
				}
            }
            else
            {
                throw new AutomationException("Can't stage the exe {0} because it doesn't start with {1} or {2}", Exe, CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir), CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir));
            }
            // the first exe is the "main" one, the rest are marked as debug files
            WorkingFileType = StagedFileType.DebugNonUFS;
        }

	}

	public override string GetCookPlatform(bool bDedicatedServer, bool bIsClientOnly, string CookFlavor)
	{
		const string NoEditorCookPlatform = "WindowsNoEditor";
		const string ServerCookPlatform = "WindowsServer";
		const string ClientCookPlatform = "WindowsClient";
		
		if( bDedicatedServer )
		{
			return ServerCookPlatform;
		}
		else if( bIsClientOnly )
		{
			return ClientCookPlatform;
		}
		else
		{
			return NoEditorCookPlatform;
		}
	}

	public override void Package(ProjectParams Params, DeploymentContext SC, int WorkingCL)
	{
		// package up the program, potentially with an installer for Windows
		PrintRunTime();
	}

	public override List<string> GetExecutableNames(DeploymentContext SC, bool bIsRun = false)
	{
		var ExecutableNames = new List<String>();
		string Ext = AutomationTool.Platform.GetExeExtension(TargetPlatformType);
		if (!String.IsNullOrEmpty(SC.CookPlatform))
		{
			if (SC.StageExecutables.Count() > 0)
			{
				foreach (var StageExecutable in SC.StageExecutables)
				{
					string ExeName = SC.StageTargetPlatform.GetPlatformExecutableName(StageExecutable);
					if (!SC.IsCodeBasedProject && (!GlobalCommandLine.Rocket || !bIsRun))
					{
						ExecutableNames.Add(CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir, ExeName + Ext));
					}
					else
					{
						if (ExeName.Contains("UE4Game") && GlobalCommandLine.Rocket && bIsRun)
						{
							ExeName = ExeName.Replace("UE4Game", SC.ShortProjectName);
						}
						ExecutableNames.Add(CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir, ExeName + Ext));
					}
				}
			}
			//@todo, probably the rest of this can go away once everything passes it through
			else if (SC.DedicatedServer)
			{
				if (!SC.IsCodeBasedProject)
				{
					string ExeName = SC.StageTargetPlatform.GetPlatformExecutableName("UE4Server");
					ExecutableNames.Add(CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir, ExeName + Ext));
				}
				else
				{
					string ExeName = SC.StageTargetPlatform.GetPlatformExecutableName(SC.ShortProjectName + "Server");
					string ClientApp = CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir, ExeName + Ext);
					var TestApp = CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir, SC.ShortProjectName + "Server" + Ext);
					string Game = "Game";
					//@todo, this is sketchy, someone might ask what the exe is before it is compiled
					if (!FileExists_NoExceptions(ClientApp) && !FileExists_NoExceptions(TestApp) && SC.ShortProjectName.EndsWith(Game, StringComparison.InvariantCultureIgnoreCase))
					{
						ExeName = SC.StageTargetPlatform.GetPlatformExecutableName(SC.ShortProjectName.Substring(0, SC.ShortProjectName.Length - Game.Length) + "Server");
						ClientApp = CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir, ExeName + Ext);
					}
					ExecutableNames.Add(ClientApp);
				}
			}
			else
			{
				if (!SC.IsCodeBasedProject && (!GlobalCommandLine.Rocket || !bIsRun))
				{
					string ExeName = SC.StageTargetPlatform.GetPlatformExecutableName("UE4Game");
					ExecutableNames.Add(CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir, ExeName + Ext));
				}
				else
				{
					string ExeName = SC.StageTargetPlatform.GetPlatformExecutableName(SC.ShortProjectName);
					ExecutableNames.Add(CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir, ExeName + Ext));
				}
			}
		}
		else
		{
			string ExeName = SC.StageTargetPlatform.GetPlatformExecutableName("UE4Editor");
			ExecutableNames.Add(CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir, ExeName + Ext));
		}
		return ExecutableNames;
	}
}

public class Win64Platform : BaseWinPlatform
{
	public Win64Platform()
		: base(UnrealTargetPlatform.Win64)
	{
	}

	public override bool IsSupported { get { return true; } }
}

public class Win32Platform : BaseWinPlatform
{
	public Win32Platform()
		: base(UnrealTargetPlatform.Win32)
	{
	}

	public override bool IsSupported { get { return true; } }
}