// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextAssetCommandlet.cpp: Commandlet for batch conversion and testing of
	text asset formats
=============================================================================*/

#include "Commandlets/TextAssetCommandlet.h"
#include "PackageHelperFunctions.h"
#include "Engine/Texture.h"
#include "Logging/LogMacros.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UObjectIterator.h"
#include "Stats/StatsMisc.h"

DEFINE_LOG_CATEGORY(LogTextAsset);

UTextAssetCommandlet::UTextAssetCommandlet( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
}

int32 UTextAssetCommandlet::Main(const FString& CmdLineParams)
{
	TArray<FString> Blacklist;

	FString ModeString, FilenameFilterString, OutputPathString, IterationsString;
	FParse::Value(*CmdLineParams, TEXT("mode="), ModeString);
	FParse::Value(*CmdLineParams, TEXT("filter="), FilenameFilterString);
	FParse::Value(*CmdLineParams, TEXT("outputpath="), OutputPathString);

	enum class EMode
	{
		ResaveAsText,
	};

	TMap<FString, EMode> Modes;
	Modes.Add(TEXT("resaveastext"), EMode::ResaveAsText);

	check(Modes.Contains(ModeString));
	EMode Mode = Modes[ModeString];
	
	TArray<UPackage*> Packages;
	TArray<UObject*> Objects;	

	int32 NumSaveIterations = 1;
	FParse::Value(*CmdLineParams, TEXT("iterations="), NumSaveIterations);

	TArray<FString> Files;

	FString BasePath = *FPaths::ProjectContentDir();
	IFileManager::Get().FindFilesRecursive(Files, *BasePath, TEXT("*.uasset"), true, false, true);
	IFileManager::Get().FindFilesRecursive(Files, *BasePath, TEXT("*.umap"), true, false, false);

	TArray<TTuple<FString, FString>> FilesToProcess;

	for (const FString& SrcFilename : Files)
	{
		bool bIgnore = false;

		FString Filename = SrcFilename;
		if (FilenameFilterString.Len() > 0 && !Filename.Contains(FilenameFilterString))
		{
			bIgnore = true;
		}

		bIgnore = bIgnore || (Filename.Contains(TEXT("_BuiltData")));

		for (const FString& BlacklistItem : Blacklist)
		{
			if (Filename.Contains(BlacklistItem))
			{
				bIgnore = true;
				break;
			}
		}

		if (bIgnore)
		{
			continue;
		}

		bool bShouldProcess = true;

		//FString Ext = FPackageName::GetTextAssetPackageExtension();
		FString Ext = FPackageName::GetAssetPackageExtension();
		FString DestinationFilename = FPaths::ChangeExtension(Filename, Ext);

		switch (Mode)
		{
		case EMode::ResaveAsText:
		{
			IFileManager::Get().Delete(*DestinationFilename, true, true, true);
			break;
		}
		default:
		{
			break;
		}
		}

		if (bShouldProcess)
		{
			FilesToProcess.Add(TTuple<FString, FString>(Filename, DestinationFilename));
		}
	}

	float TotalPackageLoadTime = 0.0;
	float TotalPackageSaveTime = 0.0;

	for (int32 Iteration = 0; Iteration < NumSaveIterations; ++Iteration)
	{
		double MaxTime = FLT_MIN;
		double MinTime = FLT_MAX;
		double TotalTime = 0;
		int64 NumFiles = 0;
		FString MaxTimePackage;
		FString MinTimePackage;
		float IterationPackageLoadTime = 0.0;
		float IterationPackageSaveTime = 0.0;

		for (const TTuple<FString, FString>& FileToProcess : FilesToProcess)
		{
			FString SourceFilename = FPackageName::FilenameToLongPackageName(FileToProcess.Get<0>());
			FString DestinationFilename = FileToProcess.Get<1>();

			double StartTime = FPlatformTime::Seconds();

			switch (Mode)
			{
			case EMode::ResaveAsText:
			{
				UE_LOG(LogTextAsset, Display, TEXT("Resaving as text asset: '%s'"), *DestinationFilename);
				UPackage* Package = nullptr;

				double Timer = 0.0;
				{
					SCOPE_SECONDS_COUNTER(Timer);
					Package = LoadPackage(nullptr, *SourceFilename, 0);
				}
				IterationPackageLoadTime += Timer;
				TotalPackageLoadTime += Timer;

				if (Package)
				{
					{
						SCOPE_SECONDS_COUNTER(Timer);
						SavePackageHelper(Package, *DestinationFilename, RF_Standalone, GWarn, nullptr, SAVE_KeepGUID);
					}
					TotalPackageSaveTime += Timer;
					IterationPackageSaveTime += Timer;
				}

				if (OutputPathString.Len() > 0)
				{
					FString CopyFilename = DestinationFilename;
					FPaths::MakePathRelativeTo(CopyFilename, *FPaths::RootDir());
					CopyFilename = OutputPathString / CopyFilename;
					IFileManager::Get().MakeDirectory(*FPaths::GetPath(CopyFilename));
					IFileManager::Get().Move(*CopyFilename, *DestinationFilename);
				}

				break;
			}
			}

			double EndTime = FPlatformTime::Seconds();
			double Time = EndTime - StartTime;

			if (Time > MaxTime)
			{
				MaxTime = Time;
				MaxTimePackage = SourceFilename;
			}

			if (Time < MinTime)
			{
				MinTime = Time;
				MinTimePackage = SourceFilename;
			}

			TotalTime += Time;
			NumFiles++;
		}

		UE_LOG(LogTextAsset, Log, TEXT("Iteration %i Completed"), Iteration + 1);
		UE_LOG(LogTextAsset, Log, TEXT("\tTotal Time:\t%.2fs"), TotalTime);
		UE_LOG(LogTextAsset, Log, TEXT("\tAvg Time:  \t%.2fms"), (TotalTime * 1000.0) / (double)NumFiles);
		UE_LOG(LogTextAsset, Log, TEXT("\tMin Time:  \t%.2fms (%s)"), MinTime * 1000.0, *MinTimePackage);
		UE_LOG(LogTextAsset, Log, TEXT("\tMax Time:  \t%.2fms (%s)"), MaxTime * 1000.0, *MaxTimePackage);
		UE_LOG(LogTextAsset, Log, TEXT("\tPackage Load Time:  \t%.2fs"), IterationPackageLoadTime);
		UE_LOG(LogTextAsset, Log, TEXT("\tPackage Save Time:  \t%.2fs"), IterationPackageSaveTime);

		Packages.Empty();
		CollectGarbage(RF_NoFlags, true);
	}

	UE_LOG(LogTextAsset, Log, TEXT("Text Asset Commandlet Completed!"));
	UE_LOG(LogTextAsset, Log, TEXT("\tAvg Iteration Package Load Time:  \t%.2fs"), TotalPackageLoadTime / (float)NumSaveIterations);
	UE_LOG(LogTextAsset, Log, TEXT("\tAvg Iteration Save Time:  \t%.2fs"), TotalPackageSaveTime / (float)NumSaveIterations);

	
	return 0;
}