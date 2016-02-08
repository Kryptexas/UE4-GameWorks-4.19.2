// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneSection.h"
#include "MovieSceneCinematicShotSection.h"
#include "LevelSequence.h"
#include "AssetRegistryModule.h"


/* MovieSceneToolHelpers
 *****************************************************************************/

void MovieSceneToolHelpers::TrimSection(const TSet<TWeakObjectPtr<UMovieSceneSection>>& Sections, float Time, bool bTrimLeft)
{
	for (auto Section : Sections)
	{
		if (Section.IsValid())
		{
			Section->TrimSection(Time, bTrimLeft);
		}
	}
}


void MovieSceneToolHelpers::SplitSection(const TSet<TWeakObjectPtr<UMovieSceneSection>>& Sections, float Time)
{
	for (auto Section : Sections)
	{
		if (Section.IsValid())
		{
			Section->SplitSection(Time);
		}
	}
}

bool MovieSceneToolHelpers::ParseShotName(const FString& ShotName, FString& ShotPrefix, uint32& ShotNumber, uint32& TakeNumber)
{
	// Parse a shot name
	// 
	// sht010:
	//  ShotPrefix = sht
	//  ShotNumber = 10
	//  TakeNumber = 1 (default)
	// 
	// sp020_002
	//  ShotPrefix = sp
	//  ShotNumber = 20
	//  TakeNumber = 2
	//
	const UMovieSceneToolsProjectSettings* ProjectSettings = GetDefault<UMovieSceneToolsProjectSettings>();

	uint32 FirstShotNumberIndex = INDEX_NONE;
	uint32 LastShotNumberIndex = INDEX_NONE;
	bool bInShotNumber = false;

	uint32 FirstTakeNumberIndex = INDEX_NONE;
	uint32 LastTakeNumberIndex = INDEX_NONE;
	bool bInTakeNumber = false;

	bool bFoundTakeSeparator = false;
	TakeNumber = ProjectSettings->FirstTakeNumber;

	for (int32 CharIndex = 0; CharIndex < ShotName.Len(); ++CharIndex)
	{
		if (FChar::IsDigit(ShotName[CharIndex]))
		{
			// Find shot number indices
			if (FirstShotNumberIndex == INDEX_NONE)
			{
				bInShotNumber = true;
				FirstShotNumberIndex = CharIndex;
			}
			if (bInShotNumber)
			{
				LastShotNumberIndex = CharIndex;
			}

			if (FirstShotNumberIndex != INDEX_NONE && LastShotNumberIndex != INDEX_NONE)
			{
				if (bFoundTakeSeparator)
				{
					// Find take number indices
					if (FirstTakeNumberIndex == INDEX_NONE)
					{
						bInTakeNumber = true;
						FirstTakeNumberIndex = CharIndex;
					}
					if (bInTakeNumber)
					{
						LastTakeNumberIndex = CharIndex;
					}
				}
			}
		}

		if (FirstShotNumberIndex != INDEX_NONE && LastShotNumberIndex != INDEX_NONE)
		{
			if (ShotName[CharIndex] == ProjectSettings->TakeSeparator[0])
			{
				bFoundTakeSeparator = true;
			}
		}
	}

	if (FirstShotNumberIndex != INDEX_NONE)
	{
		ShotPrefix = ShotName.Left(FirstShotNumberIndex);
		ShotNumber = FCString::Atoi(*ShotName.Mid(FirstShotNumberIndex, LastShotNumberIndex-FirstShotNumberIndex+1));
	}

	if (FirstTakeNumberIndex != INDEX_NONE)
	{
		TakeNumber = FCString::Atoi(*ShotName.Mid(FirstTakeNumberIndex, LastTakeNumberIndex-FirstTakeNumberIndex+1));
	}

	return FirstShotNumberIndex != INDEX_NONE;
}


FString MovieSceneToolHelpers::ComposeShotName(const FString& ShotPrefix, uint32 ShotNumber, uint32 TakeNumber)
{
	const UMovieSceneToolsProjectSettings* ProjectSettings = GetDefault<UMovieSceneToolsProjectSettings>();

	FString ShotName = ShotPrefix;

	FString ShotFormat = TEXT("%0") + FString::Printf(TEXT("%d"), ProjectSettings->ShotNumDigits) + TEXT("d");
	FString TakeFormat = TEXT("%0") + FString::Printf(TEXT("%d"), ProjectSettings->TakeNumDigits) + TEXT("d");

	ShotName += FString::Printf(*ShotFormat, ShotNumber);
	if (TakeNumber != INDEX_NONE)
	{
		ShotName += ProjectSettings->TakeSeparator;
		ShotName += FString::Printf(*TakeFormat, TakeNumber);
	}
	return ShotName;
}


FString MovieSceneToolHelpers::GenerateNewShotName(const TArray<UMovieSceneSection*>& AllSections, float Time)
{
	const UMovieSceneToolsProjectSettings* ProjectSettings = GetDefault<UMovieSceneToolsProjectSettings>();

	UMovieSceneCinematicShotSection* BeforeShot = nullptr;
	UMovieSceneCinematicShotSection* NextShot = nullptr;

	float MinEndDiff = FLT_MAX;
	float MinStartDiff = FLT_MAX;
	for (auto Section : AllSections)
	{
		if (Section->GetEndTime() >= Time)
		{
			float EndDiff = Section->GetEndTime() - Time;
			if (MinEndDiff > EndDiff)
			{
				MinEndDiff = EndDiff;
				BeforeShot = Cast<UMovieSceneCinematicShotSection>(Section);
			}
		}
		if (Section->GetStartTime() <= Time)
		{
			float StartDiff = Time - Section->GetStartTime();
			if (MinStartDiff > StartDiff)
			{
				MinStartDiff = StartDiff;
				NextShot = Cast<UMovieSceneCinematicShotSection>(Section);
			}
		}
	}
	
	// There aren't any shots, let's create the first shot name
	if (BeforeShot == nullptr || NextShot == nullptr)
	{
		// Default case
	}
	// This is the last shot
	else if (BeforeShot == NextShot)
	{
		FString NextShotPrefix = ProjectSettings->ShotPrefix;
		uint32 NextShotNumber = ProjectSettings->FirstShotNumber;
		uint32 NextTakeNumber = ProjectSettings->FirstTakeNumber;

		if (ParseShotName(NextShot->GetShotDisplayName().ToString(), NextShotPrefix, NextShotNumber, NextTakeNumber))
		{
			uint32 NewShotNumber = NextShotNumber + ProjectSettings->ShotIncrement;
			return ComposeShotName(NextShotPrefix, NewShotNumber, ProjectSettings->FirstTakeNumber);
		}
	}
	// This is in between two shots
	else 
	{
		FString BeforeShotPrefix = ProjectSettings->ShotPrefix;
		uint32 BeforeShotNumber = ProjectSettings->FirstShotNumber;
		uint32 BeforeTakeNumber = ProjectSettings->FirstTakeNumber;

		FString NextShotPrefix = ProjectSettings->ShotPrefix;
		uint32 NextShotNumber = ProjectSettings->FirstShotNumber;
		uint32 NextTakeNumber = ProjectSettings->FirstTakeNumber;

		if (ParseShotName(BeforeShot->GetShotDisplayName().ToString(), BeforeShotPrefix, BeforeShotNumber, BeforeTakeNumber) &&
			ParseShotName(NextShot->GetShotDisplayName().ToString(), NextShotPrefix, NextShotNumber, NextTakeNumber))
		{
			uint32 NewShotNumber = BeforeShotNumber + ( (NextShotNumber - BeforeShotNumber) / 2); // what if we can't find one? or conflicts with another?
			return ComposeShotName(BeforeShotPrefix, NewShotNumber, ProjectSettings->FirstTakeNumber);
		}
	}

	// Default case
	return ComposeShotName(ProjectSettings->ShotPrefix, ProjectSettings->FirstShotNumber, ProjectSettings->FirstTakeNumber);
}

void MovieSceneToolHelpers::GatherTakes(const UMovieSceneSection* Section, TArray<uint32>& TakeNumbers)
{
	const UMovieSceneCinematicShotSection* Shot = Cast<const UMovieSceneCinematicShotSection>(Section);
	
	if (Shot->GetSequence() == nullptr)
	{
		return;
	}

	FAssetData ShotData(Shot->GetSequence()->GetOuter());

	FString ShotPackagePath = ShotData.PackagePath.ToString();
	int32 ShotLastSlashPos = INDEX_NONE;
	ShotPackagePath.FindLastChar(TCHAR('/'), ShotLastSlashPos);
	ShotPackagePath = ShotPackagePath.Left(ShotLastSlashPos);

	FString ShotPrefix;
	uint32 ShotNumber = INDEX_NONE;
	uint32 TakeNumber = INDEX_NONE;

	if (ParseShotName(Shot->GetShotDisplayName().ToString(), ShotPrefix, ShotNumber, TakeNumber))
	{
		// Gather up all level sequence assets
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> ObjectList;
		AssetRegistryModule.Get().GetAssetsByClass(ULevelSequence::StaticClass()->GetFName(), ObjectList);

		for (auto AssetObject : ObjectList)
		{
			FString AssetPackagePath = AssetObject.PackagePath.ToString();
			int32 AssetLastSlashPos = INDEX_NONE;
			AssetPackagePath.FindLastChar(TCHAR('/'), AssetLastSlashPos);
			AssetPackagePath = AssetPackagePath.Left(AssetLastSlashPos);

			if (AssetPackagePath == ShotPackagePath)
			{
				FString AssetShotPrefix;
				uint32 AssetShotNumber = INDEX_NONE;
				uint32 AssetTakeNumber = INDEX_NONE;

				if (ParseShotName(AssetObject.AssetName.ToString(), AssetShotPrefix, AssetShotNumber, AssetTakeNumber))
				{
					if (AssetShotPrefix == ShotPrefix && AssetShotNumber == ShotNumber)
					{
						TakeNumbers.Add(AssetTakeNumber);
					}
				}
			}
		}
	}

	TakeNumbers.Sort();
}

UObject* MovieSceneToolHelpers::GetTake(const UMovieSceneSection* Section, uint32 TakeNumber)
{
	const UMovieSceneCinematicShotSection* Shot = Cast<const UMovieSceneCinematicShotSection>(Section);

	FAssetData ShotData(Shot->GetSequence()->GetOuter());

	FString ShotPackagePath = ShotData.PackagePath.ToString();
	int32 ShotLastSlashPos = INDEX_NONE;
	ShotPackagePath.FindLastChar(TCHAR('/'), ShotLastSlashPos);
	ShotPackagePath = ShotPackagePath.Left(ShotLastSlashPos);

	FString ShotPrefix;
	uint32 ShotNumber = INDEX_NONE;
	uint32 TakeNumberDummy = INDEX_NONE;

	if (ParseShotName(Shot->GetShotDisplayName().ToString(), ShotPrefix, ShotNumber, TakeNumberDummy))
	{
		// Gather up all level sequence assets
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> ObjectList;
		AssetRegistryModule.Get().GetAssetsByClass(ULevelSequence::StaticClass()->GetFName(), ObjectList);

		for (auto AssetObject : ObjectList)
		{
			FString AssetPackagePath = AssetObject.PackagePath.ToString();
			int32 AssetLastSlashPos = INDEX_NONE;
			AssetPackagePath.FindLastChar(TCHAR('/'), AssetLastSlashPos);
			AssetPackagePath = AssetPackagePath.Left(AssetLastSlashPos);

			if (AssetPackagePath == ShotPackagePath)
			{
				FString AssetShotPrefix;
				uint32 AssetShotNumber = INDEX_NONE;
				uint32 AssetTakeNumber = INDEX_NONE;

				if (ParseShotName(AssetObject.AssetName.ToString(), AssetShotPrefix, AssetShotNumber, AssetTakeNumber))
				{
					if (AssetShotPrefix == ShotPrefix && AssetShotNumber == ShotNumber && TakeNumber == AssetTakeNumber)
					{
						return AssetObject.GetAsset();
					}
				}
			}
		}
	}

	return nullptr;
}


