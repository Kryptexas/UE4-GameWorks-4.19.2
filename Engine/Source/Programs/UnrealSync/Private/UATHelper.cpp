// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealSync.h"

#include "UATHelper.h"

/**
 * Triggers progress event.
 *
 * @param Input Input string to pass.
 * @param OnUATMadeProgress Progress delegate to pass input.
 *
 * @returns Passes down the result of OnUATMadeProgress which tells if process should continue.
 */
bool TriggerProgress(const FString& Input, FOnUATMadeProgress OnUATMadeProgress)
{
	if (OnUATMadeProgress.IsBound())
	{
		return OnUATMadeProgress.Execute(Input);
	}

	return true;
}

bool RunUATLog(const FString& CommandLine, FString& Output)
{
	class FOnProgressAppender
	{
	public:
		bool Execute(const FString& Input)
		{
			Output += Input;

			return true;
		}

		FString Output;
	};

	FOnProgressAppender OnProgressAppender;

	auto bSuccess = RunUATProgress(CommandLine, FOnUATMadeProgress::CreateRaw(&OnProgressAppender, &FOnProgressAppender::Execute));

	Output = OnProgressAppender.Output;

	return bSuccess;
}

/**
 * For a given binary/project pair checks if binary exists and if it doesn't it tries to build it.
 *
 * @param BinaryPath Path to binary file.
 * @param ProjectPath Path to project file that outputs the binary file.
 *
 * @returns False if build failed. True otherwise.
 */
bool BuildIfDoesntExist(FString BinaryPath, FString ProjectPath)
{
	if (FPaths::FileExists(BinaryPath))
	{
		return true;
	}

	// First determine the appropriate vcvars batch file to launch
	FString VCVarsBat;

	if (!FPlatformMisc::GetVSComnTools(12, VCVarsBat))
	{
		if (!FPlatformMisc::GetVSComnTools(11, VCVarsBat))
		{
			return false;
		}
	}

	VCVarsBat = FPaths::Combine(*VCVarsBat, L"../../VC/bin/x86_amd64/vcvarsx86_amd64.bat");

	FString CommandLine = FString::Printf(TEXT("/C \"call \"%s\" & msbuild.exe /nologo /verbosity:quiet \"%s\" /property:Configuration=Development /property:Platform=AnyCPU\""), *VCVarsBat, *ProjectPath);

	void* ReadPipe;
	void* WritePipe;

	FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

	auto BuildingProcHandle = FPlatformProcess::CreateProc(TEXT("cmd.exe"), *CommandLine, false, true, false, NULL, 0, NULL, WritePipe);

	FPlatformProcess::WaitForProc(BuildingProcHandle);

	int32 RetCode;
	FPlatformProcess::GetProcReturnCode(BuildingProcHandle, &RetCode);

	FString Output = FPlatformProcess::ReadPipe(ReadPipe);

	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);

	return RetCode == 0 && FPaths::FileExists(BinaryPath);
}

/**
 * Ensures UAT exists. If it doesn't it tries to build it.
 *
 * @returns False if UAT doesn't exist and the build failed. True otherwise.
 */
bool EnsureUATExists()
{
	FString DotNETBinariesPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries"), TEXT("DotNET")));

	FString AutomationToolPath = DotNETBinariesPath / TEXT("AutomationTool.exe");
	FString AutomationToolLauncherPath = DotNETBinariesPath / TEXT("AutomationToolLauncher.exe");

	FString SourcePath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(*FPaths::EngineDir(), TEXT("Source"), TEXT("Programs")));

	FString AutomationToolProject = SourcePath / TEXT("AutomationTool") / TEXT("AutomationTool.csproj");
	FString AutomationToolLauncherProject = SourcePath / TEXT("AutomationToolLauncher") / TEXT("AutomationToolLauncher.csproj");

	return BuildIfDoesntExist(AutomationToolPath, AutomationToolProject) &&	BuildIfDoesntExist(AutomationToolLauncherPath, AutomationToolLauncherProject);
}

bool RunUATProgress(const FString& CommandLine, FOnUATMadeProgress OnUATMadeProgress)
{
	if (!EnsureUATExists())
	{
		return false;
	}

	void* ReadPipe;
	void* WritePipe;

	FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

	FString Executable = TEXT("AutomationToolLauncher.exe");
	FString ExecutablePath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries"), TEXT("DotNET")));

	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*(ExecutablePath / Executable), *CommandLine, false, true, true, NULL, 0, *ExecutablePath, WritePipe);

	while (FPlatformProcess::IsProcRunning(ProcessHandle))
	{
		if (!TriggerProgress(FPlatformProcess::ReadPipe(ReadPipe), OnUATMadeProgress))
		{
			FPlatformProcess::TerminateProc(ProcessHandle);
			FPlatformProcess::WaitForProc(ProcessHandle);
			return false;
		}
		FPlatformProcess::Sleep(0.25);
	}

	FPlatformProcess::WaitForProc(ProcessHandle);
	TriggerProgress(FPlatformProcess::ReadPipe(ReadPipe), OnUATMadeProgress);

	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);

	int32 ReturnCode;
	if (!FPlatformProcess::GetProcReturnCode(ProcessHandle, &ReturnCode))
	{
		return false;
	}

	return true;
}