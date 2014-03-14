// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AudioRefactorCommandlets.cpp: Commandlets required to convert from UE3
	                              to UE4 audio architecture
=============================================================================*/

#include "UnrealEd.h"
#include "SoundDefinitions.h"
#include "AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogCountSoundNodesCommandlet, Log, All);

/*-----------------------------------------------------------------------------
	UCountSoundNodesCommandlet commandlet.
-----------------------------------------------------------------------------*/
UCountSoundNodesCommandlet::UCountSoundNodesCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	LogToConsole = true;
}

int32 UCountSoundNodesCommandlet::Main( const FString& Params )
{
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> SoundCueList;
	AssetRegistryModule.Get().GetAssetsByClass(USoundCue::StaticClass()->GetFName(), SoundCueList);


	for( int32 SoundCueIndex = 0; SoundCueIndex < SoundCueList.Num(); ++SoundCueIndex )
	{
		{
			USoundCue* SoundCue = CastChecked<USoundCue>(SoundCueList[SoundCueIndex].GetAsset());
			if (SoundCue)
			{
				// determine if there is a deprecated node in the cue
				CountSoundNodes(SoundCue->FirstNode);
			}
		}

		CollectGarbage(RF_Native);
	}

	struct FCompareSoundCount
	{
		FORCEINLINE bool operator()( const uint32 A, const uint32 B ) const
		{
			return B < A;
		}
	};
	SoundNodeCounts.ValueSort(FCompareSoundCount());

	for (TMap<UClass*,uint32>::TConstIterator It(SoundNodeCounts); It; ++It )
	{
		UE_LOG(LogCountSoundNodesCommandlet, Log, TEXT("%s - %d"), *(It.Key()->GetDescription()), It.Value());
	}

	return 0;
}

void UCountSoundNodesCommandlet::CountSoundNodes(USoundNode* node)
{
	if (node)
	{
		++(SoundNodeCounts.FindOrAdd(node->GetClass()));

		for( int32 i = 0; i < node->ChildNodes.Num(); ++i )
		{
			CountSoundNodes(node->ChildNodes[i]);
		}
	}
}
