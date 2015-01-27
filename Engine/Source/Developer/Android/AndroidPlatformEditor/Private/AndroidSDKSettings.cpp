// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AndroidPlatformEditorPrivatePCH.h"

//#include "EngineTypes.h"
#include "AndroidSDKSettings.h"

DEFINE_LOG_CATEGORY_STATIC(AndroidSDKSettings, Log, All);

UAndroidSDKSettings::UAndroidSDKSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

#if WITH_EDITOR
void UAndroidSDKSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateTargetModulePaths(true);	
}

void UAndroidSDKSettings::SetTargetModule(ITargetPlatformManagerModule * TargetManagerModule)
{
	this->TargetManagerModule = TargetManagerModule;
}

void UAndroidSDKSettings::SetDeviceDetection(IAndroidDeviceDetection * AndroidDeviceDetection)
{
	this->AndroidDeviceDetection = AndroidDeviceDetection;
}

void UAndroidSDKSettings::SetupInitialTargetPaths()
{
	// Check each path, if we don't have it then try to look up the values from a known location instead
	if (SDKPath.Path.IsEmpty())
	{
		TCHAR AndroidSDKPath[256];
		FPlatformMisc::GetEnvironmentVariable(TEXT("ANDROID_HOME"), AndroidSDKPath, ARRAY_COUNT(AndroidSDKPath));
		if (AndroidSDKPath[0])
		{
			SDKPath.Path = AndroidSDKPath;
		}
	}

	if (NDKPath.Path.IsEmpty())
	{
		TCHAR AndroidNDKPath[256];
		FPlatformMisc::GetEnvironmentVariable(TEXT("NDKROOT"), AndroidNDKPath, ARRAY_COUNT(AndroidNDKPath));
		if (AndroidNDKPath[0])
		{
			NDKPath.Path = AndroidNDKPath;
		}
	}

	if (ANTPath.Path.IsEmpty())
	{
		TCHAR AndroidANTPath[256];
		FPlatformMisc::GetEnvironmentVariable(TEXT("ANT_HOME"), AndroidANTPath, ARRAY_COUNT(AndroidANTPath));
		if (AndroidANTPath[0])
		{
			ANTPath.Path = AndroidANTPath;
		}
	}

#if PLATFORM_MAC == 0
	if (JavaPath.Path.IsEmpty())
	{
		TCHAR AndroidJavaPath[256];
		FPlatformMisc::GetEnvironmentVariable(TEXT("JAVA_HOME"), AndroidJavaPath, ARRAY_COUNT(AndroidJavaPath));
		if (AndroidJavaPath[0])
		{
			JavaPath.Path = AndroidJavaPath;
		}
	}
#endif

#if PLATFORM_MAC
	// On a Mac, if we still don't have the values then check the .bash_profile file for them
	if (SDKPath.Path.IsEmpty() || NDKPath.Path.IsEmpty() || ANTPath.Path.IsEmpty())
	{
		FArchive* FileReader = IFileManager::Get().CreateFileReader(*FString([@"~/.bash_profile" stringByExpandingTildeInPath]));
		if (FileReader)
		{
			const int64 FileSize = FileReader->TotalSize();
			ANSICHAR* AnsiContents = (ANSICHAR*)FMemory::Malloc(FileSize);
			FileReader->Serialize(AnsiContents, FileSize);
			FileReader->Close();
			delete FileReader;

			TArray<FString> Lines;
			FString(ANSI_TO_TCHAR(AnsiContents)).ParseIntoArrayLines(&Lines);
			FMemory::Free(AnsiContents);

			for (int32 Index = 0; Index < Lines.Num(); Index++)
			{
				if (SDKPath.Path.IsEmpty() && Lines[Index].StartsWith(TEXT("export ANDROID_HOME=")))
				{
					FString Directory;
					Lines[Index].Split(TEXT("="), NULL, &Directory);
					SDKPath.Path = Directory.Replace(TEXT("\""), TEXT(""));
				}
				else if (ANTPath.Path.IsEmpty() && Lines[Index].StartsWith(TEXT("export ANT_HOME=")))
				{
					FString Directory;
					Lines[Index].Split(TEXT("="), NULL, &Directory);
					ANTPath.Path = Directory.Replace(TEXT("\""), TEXT(""));
				}
				else if (NDKPath.Path.IsEmpty() && Lines[Index].StartsWith(TEXT("export NDKROOT=")))
				{
					FString Directory;
					Lines[Index].Split(TEXT("="), NULL, &Directory);
					NDKPath.Path = Directory.Replace(TEXT("\""), TEXT(""));
				}
			}
		}
	}

#endif
	UpdateTargetModulePaths(false);
}
void UAndroidSDKSettings::UpdateTargetModulePaths(bool bForceUpdate)
{
	TArray<FString> Keys;
	TArray<FString> Values;
	
	if (bForceUpdate || !SDKPath.Path.IsEmpty())
	{
		FPaths::NormalizeFilename(SDKPath.Path);
		Keys.Add(TEXT("ANDROID_HOME"));
		Values.Add(SDKPath.Path);
	}
	
	if (bForceUpdate || !NDKPath.Path.IsEmpty())
	{
		FPaths::NormalizeFilename(NDKPath.Path);
		Keys.Add(TEXT("NDKROOT"));
		Values.Add(NDKPath.Path);
	}
	
	if (bForceUpdate || !ANTPath.Path.IsEmpty())
	{
		FPaths::NormalizeFilename(ANTPath.Path);
		Keys.Add(TEXT("ANT_HOME"));
		Values.Add(ANTPath.Path);
	}

#if PLATFORM_MAC == 0
	if (bForceUpdate || !JavaPath.Path.IsEmpty())
	{
		FPaths::NormalizeFilename(JavaPath.Path);
		Keys.Add(TEXT("JAVA_HOME"));
		Values.Add(JavaPath.Path);
	}
#endif

	SaveConfig();
	
	if (Keys.Num() != 0)
	{
		TargetManagerModule->UpdatePlatformEnvironment(TEXT("Android"), Keys, Values);
		AndroidDeviceDetection->UpdateADBPath();
	}
}

#endif
