// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CLionSourceCodeAccessor.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "DesktopPlatformModule.h"
#include "Regex.h"
#include "FileHelper.h"
#include "JsonReader.h"
#include "JsonObject.h"
#include "JsonSerializer.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "CLionSourceCodeAccessor"

DEFINE_LOG_CATEGORY_STATIC(LogCLionAccessor, Log, All);

void FCLionSourceCodeAccessor::RefreshAvailability()
{
	// Find our program
	ExecutablePath = FindExecutablePath();

	// If we have an executable path, we certainly have it installed!
	if (!ExecutablePath.IsEmpty())
	{
		bHasCLionInstalled = true;
	}
	else
	{
		bHasCLionInstalled = false;
	}
}

bool FCLionSourceCodeAccessor::AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules)
{
	// @todo.clion Manually add to folders? Or just regenerate
	return false;
}

bool FCLionSourceCodeAccessor::CanAccessSourceCode() const
{
	return bHasCLionInstalled;
}

bool FCLionSourceCodeAccessor::DoesSolutionExist() const
{
	const FString Path = FPaths::Combine(*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()), TEXT("CMakeLists.txt"));
	if (!FPaths::FileExists(Path))
	{
		return false;
	}
	return true;
}

FText FCLionSourceCodeAccessor::GetDescriptionText() const
{
	return LOCTEXT("CLionDisplayDesc", "Open source code files in CLion");
}

FName FCLionSourceCodeAccessor::GetFName() const
{
	return FName("CLionSourceCodeAccessor");
}

FText FCLionSourceCodeAccessor::GetNameText() const
{
	return LOCTEXT("CLionDisplayName", "CLion");
}

bool FCLionSourceCodeAccessor::OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	if (!bHasCLionInstalled)
	{
		return false;
	}

	const FString Path = FString::Printf(TEXT("\"%s\" --line %d \"%s\""), *FPaths::ConvertRelativePathToFull(*FPaths::ProjectDir()), LineNumber, *FullPath);

	FProcHandle Proc = FPlatformProcess::CreateProc(*ExecutablePath, *Path, true, true, false, nullptr, 0, nullptr, nullptr);
	if (!Proc.IsValid())
	{
		UE_LOG(LogCLionAccessor, Warning, TEXT("Opening file (%s) at a specific line failed."), *Path);
		FPlatformProcess::CloseProc(Proc);
		return false;
	}

	return true;
}

bool FCLionSourceCodeAccessor::OpenSolution()
{
	if (!bHasCLionInstalled)
	{
		return false;
	}

	const FString Path = FString::Printf(TEXT("\"%s\""), *FPaths::ConvertRelativePathToFull(*FPaths::ProjectDir()));

	FPlatformProcess::CreateProc(*ExecutablePath, *Path, true, true, false, nullptr, 0, nullptr, nullptr);

	return true;
}

bool FCLionSourceCodeAccessor::OpenSolutionAtPath(const FString& InSolutionPath)
{
	if (!bHasCLionInstalled)
	{
		return false;
	}

	return FPlatformProcess::CreateProc(*ExecutablePath, *InSolutionPath, true, true, false, nullptr, 0, nullptr, nullptr).IsValid();
}

bool FCLionSourceCodeAccessor::OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths)
{
	if (!bHasCLionInstalled)
	{
		return false;
	}

	FString SourceFilesList = "";

	// Build our paths based on what unreal sends to be opened
	for (const auto& SourcePath : AbsoluteSourcePaths)
	{
		SourceFilesList = FString::Printf(TEXT("%s \"%s\""), *SourceFilesList, *SourcePath);
	}

	// Trim any whitespace on our source file list
	SourceFilesList.TrimStartInline();
	SourceFilesList.TrimEndInline();

	FProcHandle Proc = FPlatformProcess::CreateProc(*ExecutablePath, *SourceFilesList, true, false, false, nullptr, 0, nullptr, nullptr);
	if (!Proc.IsValid())
	{
		UE_LOG(LogCLionAccessor, Warning, TEXT("Opening the source file (%s) failed."), *SourceFilesList);
		FPlatformProcess::CloseProc(Proc);
		return false;
	}

	return true;
}

bool FCLionSourceCodeAccessor::SaveAllOpenDocuments() const
{
	//@todo.clion This feature will be made available in 2017.3, till then we'll leave it commented out for a future PR
	// FProcHandle Proc = FPlatformProcess::CreateProc(*ExecutablePath, TEXT("save"), true, false,
	//                                                 false, nullptr, 0, nullptr, nullptr);

	// if (!Proc.IsValid())
	// {
	// 	FPlatformProcess::CloseProc(Proc);
	// 	return false;
	// }
	// return true;
	return false;
}

FString FCLionSourceCodeAccessor::FindExecutablePath()
{
#if PLATFORM_WINDOWS
	// Search from JetBrainsToolbox folder
	FString ToolboxBinPath;

	if (FWindowsPlatformMisc::QueryRegKey(HKEY_CURRENT_USER, TEXT("Software\\JetBrains s.r.o.\\JetBrainsToolbox\\"), TEXT(""), ToolboxBinPath))
	{
		FPaths::NormalizeDirectoryName(ToolboxBinPath);
		FString PatternString(TEXT("(.*)/bin"));
		FRegexPattern Pattern(PatternString);
		FRegexMatcher Matcher(Pattern, ToolboxBinPath);
		if (Matcher.FindNext())
		{
			FString ToolboxPath = Matcher.GetCaptureGroup(1);

			FString SettingJsonPath = FPaths::Combine(ToolboxPath, FString(".settings.json"));
			if (FPaths::FileExists(SettingJsonPath))
			{
				FString JsonStr;
				FFileHelper::LoadFileToString(JsonStr, *SettingJsonPath);
				TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonStr);
				TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
				if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
				{
					FString InstallLocation;
					if (JsonObject->TryGetStringField(TEXT("install_location"), InstallLocation))
					{
						if (!InstallLocation.IsEmpty())
						{
							ToolboxPath = InstallLocation;
						}
					}
				}
			}

			FString CLionHome = FPaths::Combine(ToolboxPath, FString("apps"), FString("CLion"));
			if (FPaths::DirectoryExists(CLionHome))
			{
				TArray<FString> IDEPaths;
				IFileManager::Get().FindFilesRecursive(IDEPaths, *CLionHome, TEXT("clion64.exe"), true, false);
				if (IDEPaths.Num() > 0)
				{
					return IDEPaths[0];
				}
			}
		}
	}
	
	// Search from ProgID
	FString OpenCommand;
	if (!FWindowsPlatformMisc::QueryRegKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Classes\\Applications\\clion64.exe\\shell\\open\\command\\"), TEXT(""), OpenCommand))
	{
		FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Applications\\clion64.exe\\shell\\open\\command\\"), TEXT(""), OpenCommand);
	}

	FString PatternString(TEXT("\"(.*)\" \".*\""));
	FRegexPattern Pattern(PatternString);
	FRegexMatcher Matcher(Pattern, OpenCommand);
	if (Matcher.FindNext())
	{
		FString IDEPath = Matcher.GetCaptureGroup(1);
		if (FPaths::FileExists(IDEPath))
		{
			return IDEPath;
		}
	}

#elif  PLATFORM_MAC

	// Check for EAP
	NSURL* CLionPreviewURL = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:@"com.jetbrains.CLion-EAP"];
	if (CLionPreviewURL != nullptr)
	{
		return FString([CLionPreviewURL path]);
	}

	// Standard CLion Install
	NSURL* CLionURL = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:@"com.jetbrains.CLion"];
	if (CLionURL != nullptr)
	{
		return FString([CLionURL path]);
	}

	// Failsafe
	if (FPaths::FileExists(TEXT("/Applications/CLion.app/Contents/MacOS/clion")))
	{
		return TEXT("/Applications/CLion.app/Contents/MacOS/clion");
	}

#else

	// Linux Default Install
	if(FPaths::FileExists(TEXT("/opt/clion/bin/clion.sh")))
	{
		return TEXT("/opt/clion/bin/clion.sh");
	}
#endif

	// Nothing was found, return nothing as well
	return TEXT("");
}

#undef LOCTEXT_NAMESPACE
