// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AudioRefactorCommandlets.cpp: Commandlets required to convert from UE3
	                              to UE4 audio architecture
=============================================================================*/

#include "UnrealEd.h"
#include "ISourceControlModule.h"
#include "SoundDefinitions.h"
#include "AssetRegistryModule.h"
#include "PackageHelperFunctions.h"

DEFINE_LOG_CATEGORY_STATIC(LogUE4AudioRefactorMigrationCommandlet, Log, All);

namespace
{

enum EMigrationResult
{
	MIGRATION_OK,
	MIGRATION_ABORT
};

void PeriodicallyCollectGarbage()
{
	static int32 CallCount = 0;

	if (++CallCount > 100)
	{
		CollectGarbage(RF_Native);
		CallCount = 0;
	}
}

void SavePackage(UPackage* Package)
{
	FString PackageName = Package->GetFName().ToString();
	FString PackageFilename;
	if ( FPackageName::DoesPackageExist( *PackageName, NULL, &PackageFilename ) == false )
	{
		UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not save package '%s' because filename could not be determined"), *PackageName);
	}
	else if ( IFileManager::Get().IsReadOnly( *PackageFilename) )
	{
		UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Can not save package '%s' because file is read-only"), *PackageName);
	}
	else if ( SavePackageHelper(Package, PackageFilename) == false )
	{
		UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Failed to save package '%s'"), *PackageName);
	}
}

bool CheckoutPackage(UPackage* Package)
{
	FString PackageName = Package->GetFName().ToString();

	Package->FullyLoad();
	if(ISourceControlModule::Get().IsEnabled())
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		FString PackageFilename;
		FPackageName::DoesPackageExist( PackageName, NULL, &PackageFilename );
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(PackageFilename, EStateCacheUsage::ForceUpdate);
		if (SourceControlState.IsValid() && !SourceControlState->IsCheckedOut() && !SourceControlState->IsAdded())
		{
			if(!SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), Package))
			{
				UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not check out package '%s'."), *PackageName);
				return false;
			}
		}
	}

	// TODO: Handle read-only files without SCC?

	return true;
}

void DeletePackage(const FString& Filename)
{
	// get file SCC status
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::ForceUpdate);
	if(!SourceControlState.IsValid() || !SourceControlState->IsSourceControlled() || !SourceControlState->IsDeleted())
	{
		if (!IFileManager::Get().Delete(*Filename, false, true))
		{
			SET_WARN_COLOR(COLOR_RED);
			UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("  ... failed to delete from disk."), *Filename);
			CLEAR_WARN_COLOR();
		}
	}
	else if(SourceControlState->IsCheckedOut() || SourceControlState->IsAdded())
	{
		// First revert...
		SourceControlProvider.Execute(ISourceControlOperation::Create<FRevert>(), Filename);

		// ... then delete from source control
		SourceControlProvider.Execute(ISourceControlOperation::Create<FDelete>(), Filename);
	}
	else if(SourceControlState->CanEdit() || !SourceControlState->IsCurrent())
	{
		// delete from source control
		SourceControlProvider.Execute(ISourceControlOperation::Create<FDelete>(), Filename);
	}
	else if(SourceControlState->IsCheckedOutOther())
	{
		// someone else has it checked out, so we can't do anything about it
		SET_WARN_COLOR(COLOR_RED);
		UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Couldn't delete '%s' from source control, someone has it checked out, skipping..."), *Filename);
		CLEAR_WARN_COLOR();
	}
}

bool HasDeprecatedSoundNode(USoundCue* SoundCue)
{
	if (SoundCue)
	{
		for (int32 NodeIndex = 0; NodeIndex < SoundCue->AllNodes.Num(); ++NodeIndex)
		{
			USoundNode* SoundNode = SoundCue->AllNodes[NodeIndex];

			if ( SoundNode && SoundNode->IsA(UDEPRECATED_SoundNodeDeprecated::StaticClass()) )
			{
				return true;
			}
		}
	}

	return false;
}

void DeleteSoundCuesWithUnsupportedNodes()
{
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> SoundCueList;
	AssetRegistryModule.Get().GetAssetsByClass(USoundCue::StaticClass()->GetFName(), SoundCueList);

	TArray<FString> PackagesToDelete;
	for( int32 SoundCueIndex = 0; SoundCueIndex < SoundCueList.Num(); ++SoundCueIndex )
	{
		{
			if (HasDeprecatedSoundNode(CastChecked<USoundCue>(SoundCueList[SoundCueIndex].GetAsset())))
			{
				FString PackageFilename;
				if( FPackageName::DoesPackageExist( SoundCueList[SoundCueIndex].PackageName.ToString(), NULL, &PackageFilename ) )
				{
					PackagesToDelete.Add(PackageFilename);
				}
				else
				{
					UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not delete package '%s' because filename could not be determined"), *(SoundCueList[SoundCueIndex].PackageName.ToString()));
				}
			}
		}

		PeriodicallyCollectGarbage();
	}

	for( int32 PackageIndex = 0; PackageIndex < PackagesToDelete.Num(); ++PackageIndex)
	{
		DeletePackage(PackagesToDelete[PackageIndex]);
	}
}

// Iterates through all sound cues and replaces the SoundNodeWave nodes with SoundNodeWavePlayer nodes
// Returns a map of what SoundWave each player will need to reference

struct FWavePlayerSoundPair
{
	FWavePlayerSoundPair(const FString& InWavePlayerName, const FString& InSoundName)
		: WavePlayerName(InWavePlayerName)
		, SoundName(InSoundName)
	{
	}
	FString WavePlayerName;
	FString SoundName;
};

void ReplaceSoundNodeWavesInSoundCues(const FAssetRegistryModule& AssetRegistryModule, TMap<FName, TArray<FWavePlayerSoundPair>>& WavePlayerSoundMap)
{
	TArray<FAssetData> SoundCues;
	AssetRegistryModule.Get().GetAssetsByClass(USoundCue::StaticClass()->GetFName(), SoundCues);

	for (int32 SoundCueIndex = 0; SoundCueIndex < SoundCues.Num(); ++SoundCueIndex)
	{
		{
			USoundCue *SoundCue = CastChecked<USoundCue>(SoundCues[SoundCueIndex].GetAsset());

			UPackage* Package = SoundCue->GetOutermost();

			bool bPackageDirty = false;
			bool bFailedToCheckout = false;

			TMap<UDEPRECATED_SoundNodeWave*, USoundNodeWavePlayer*> WaveToPlayerMap;

			// Iterate across the editor data to find all the nodes and update them
			// First pass we're going to create a mapping from USoundNodeWave to USoundNodeWavePlayer
			for (int32 NodeIndex = 0; NodeIndex < SoundCue->AllNodes.Num(); ++NodeIndex)
			{
				UDEPRECATED_SoundNodeWave* SoundNodeWave = Cast<UDEPRECATED_SoundNodeWave>(SoundCue->AllNodes[NodeIndex]);
				if (SoundNodeWave)
				{
					if (!bPackageDirty && !CheckoutPackage(Package))
					{
						bFailedToCheckout = true;
						break;
					}
					bPackageDirty = true;

					USoundNodeWavePlayer* WavePlayer = SoundCue->ConstructSoundNode<USoundNodeWavePlayer>();
					WavePlayerSoundMap.FindOrAdd(Package->GetFName()).Add(FWavePlayerSoundPair(WavePlayer->GetPathName(), SoundNodeWave->GetPathName()));
					WaveToPlayerMap.Add(SoundNodeWave, WavePlayer);
				}
			}
			if (bFailedToCheckout)
			{
				continue;
			}

			// If there were no SoundNodeWave nodes found, no reason to do the rest of the sound cue processing
			if (bPackageDirty)
			{
				// Second pass we update all the references to the USoundNodeWave to reference the USoundNodeWavePlayer
				for (int32 NodeIndex = 0; NodeIndex < SoundCue->AllNodes.Num(); ++NodeIndex)
				{
					USoundNode* SoundNode = SoundCue->AllNodes[NodeIndex];
					if (SoundNode)
					{
						for (int32 ChildIndex=0; ChildIndex < SoundNode->ChildNodes.Num(); ++ChildIndex)
						{
							UDEPRECATED_SoundNodeWave* SoundNodeWave = Cast<UDEPRECATED_SoundNodeWave>(SoundNode->ChildNodes[ChildIndex]);
							if (SoundNodeWave)
							{
								SoundNode->ChildNodes[ChildIndex] = WaveToPlayerMap.FindChecked(SoundNodeWave);
							}
						}
					}
				}

				// Next we remove the USoundNodeWave from the cue, put the USoundNodeWavePlayer in, and set the position of the player
				for (auto WaveToPlayerIt = WaveToPlayerMap.CreateConstIterator(); WaveToPlayerIt; ++WaveToPlayerIt)
				{
					SoundCue->AllNodes.Remove(WaveToPlayerIt.Key());

					WaveToPlayerIt.Value()->GraphNode->NodePosX = WaveToPlayerIt.Key()->GraphNode->NodePosX;
					WaveToPlayerIt.Value()->GraphNode->NodePosY = WaveToPlayerIt.Key()->GraphNode->NodePosY;
				}

				// Finally we check to see if the sound cue's first node was a USoundNodeWave and update it if necessary
				UDEPRECATED_SoundNodeWave* SoundNodeWave = Cast<UDEPRECATED_SoundNodeWave>(SoundCue->FirstNode);
				if (SoundNodeWave)
				{
					SoundCue->FirstNode = WaveToPlayerMap.FindChecked(SoundNodeWave);
				}
			}

			if (bPackageDirty)
			{
				SavePackage(Package);
			}

			SoundCue->LinkGraphNodesFromSoundNodes();
		}

		PeriodicallyCollectGarbage();
	}
}

void UpdateSoundWavesInPackage(UPackage* Package, const TMap<FString, TMap<int32, FString>>& PackageWaveMap)
{
	bool bPackageDirty = false;

	for (auto ObjectIt = PackageWaveMap.CreateConstIterator(); ObjectIt; ++ObjectIt)
	{
		UObject* Object = FindObject<UObject>(NULL, *(ObjectIt.Key()));
		UAudioComponent* AudioComponent = Cast<UAudioComponent>(Object);
		UDEPRECATED_SoundNodeAmbient* SoundNodeAmbient = Cast<UDEPRECATED_SoundNodeAmbient>(Object);

		if (AudioComponent == NULL && SoundNodeAmbient == NULL)
		{
			UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not find object '%s' in package '%s'."), *(ObjectIt.Key()), *Package->GetName());
			continue;
		}
		for (auto ParamIt = ObjectIt.Value().CreateConstIterator(); ParamIt; ++ParamIt)
		{
			if (AudioComponent)
			{
				if (ParamIt.Key() >= AudioComponent->InstanceParameters.Num())
				{
					UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Invalid param index '%d' in audio component '%s' in package '%s'."), ParamIt.Key(), *(ObjectIt.Key()), *Package->GetName());
					continue;
				}
			}
			else
			{
				TArray<FAmbientSoundSlot>& Slots = SoundNodeAmbient->SoundSlots;
				if (ParamIt.Key() >= Slots.Num())
				{
					UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Invalid param index '%d' in ambient sound node '%s' in package '%s'."), ParamIt.Key(), *(ObjectIt.Key()), *Package->GetName());
					continue;
				}
			}
			if (!bPackageDirty && !CheckoutPackage(Package))
			{
				return;
			}
			bPackageDirty = true;

			if (AudioComponent)
			{
				AudioComponent->InstanceParameters[ParamIt.Key()].SoundWaveParam = LoadObject<USoundWave>(NULL, *ParamIt.Value());
			}
			else
			{
				TArray<FAmbientSoundSlot>& Slots = SoundNodeAmbient->SoundSlots;
				Slots[ParamIt.Key()].SoundWave = LoadObject<USoundWave>(NULL, *ParamIt.Value());
			}
		}
		// Once the sound node has had the slots array converted we can now run the migration
		if (SoundNodeAmbient)
		{
			AAmbientSound* AmbientSound = CastChecked<AAmbientSound>(SoundNodeAmbient->GetOuter());
			AmbientSound->MigrateSoundNodeInstance();
		}
	}
	if (bPackageDirty)
	{
		SavePackage(Package);
	}
}

// Iterate through all packages that had an audio component referencing a SoundNodeWave
// and hook it up to the SoundWave
void UpdateSoundWavesInPackages(const TMap<FName, TMap<FString, TMap<int32, FString>>>& WaveMap)
{
	for (auto PackageIt = WaveMap.CreateConstIterator(); PackageIt; ++PackageIt)
	{
		FString PackageFilename;
		if ( FPackageName::DoesPackageExist( PackageIt.Key().ToString(), NULL, &PackageFilename ) == false )
		{
			UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not find package '%s'."), *(PackageIt.Key().ToString()));
			continue;
		}

		UPackage* Package = LoadPackage(NULL, *PackageFilename, LOAD_None);
		if (Package == NULL)
		{
			UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not load package '%s'."), *PackageFilename);
			continue;
		}
		UpdateSoundWavesInPackage(Package, PackageIt.Value());

		if (Package->GetLinker()->ContainsMap())
		{
			CollectGarbage(RF_Native);
		}
		else
		{
			PeriodicallyCollectGarbage();
		}
	}
}

// Iterates through all the AudioComponents in a Package and finds references to SoundNodeWave,
// stores off the information necessary to hook up the SoundWave once it is created, and then
// severs the references to the SoundNodeWave
void ClearSoundNodeWavesInPackage(UPackage* Package, TMap<FName, TMap<FString, TMap<int32, FString>>>& WaveMap)
{
	bool bPackageDirty = false;

	for (TObjectIterator<UAudioComponent> It; It; ++It)
	{
		UAudioComponent* AudioComponent = *It;
		if (AudioComponent->IsIn(Package))
		{
			for (int32 ParamIndex = 0; ParamIndex < AudioComponent->InstanceParameters.Num(); ++ParamIndex)
			{
				FAudioComponentParam& Param = AudioComponent->InstanceParameters[ParamIndex];
				if (Param.WaveParam_DEPRECATED)
				{
					if (!bPackageDirty)
					{
						CheckoutPackage(Package);

						bPackageDirty = true;
					}
					TMap<int32, FString>& AudioComponentWaveMap = WaveMap.FindOrAdd(Package->GetFName()).FindOrAdd(AudioComponent->GetPathName());
					AudioComponentWaveMap.Add(ParamIndex, Param.WaveParam_DEPRECATED->GetPathName());
					Param.WaveParam_DEPRECATED = NULL;
				}
			}
		}
	}
	for (TObjectIterator<USoundNode> It; It; ++It)
	{
		USoundNode* SoundNode = *It;
		if (SoundNode->IsIn(Package))
		{
			UDEPRECATED_SoundNodeAmbient* SoundNodeAmbient = Cast<UDEPRECATED_SoundNodeAmbient>(SoundNode);
			if (SoundNodeAmbient)
			{
				TArray<FAmbientSoundSlot>& Slots = SoundNodeAmbient->SoundSlots;

				for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
				{
					FAmbientSoundSlot& Slot = Slots[SlotIndex];
					if (Slot.Wave_DEPRECATED)
					{
						if (!bPackageDirty)
						{
							CheckoutPackage(Package);

							bPackageDirty = true;
						}
						TMap<int32, FString>& AudioComponentWaveMap = WaveMap.FindOrAdd(Package->GetFName()).FindOrAdd(SoundNode->GetPathName());
						AudioComponentWaveMap.Add(SlotIndex, Slot.Wave_DEPRECATED->GetPathName());
						Slot.Wave_DEPRECATED = NULL;
					}
				}
			}
		}
	}
	if (bPackageDirty)
	{
		SavePackage(Package);
	}
}

void ClearSoundNodeWavesInBlueprints(const FAssetRegistryModule& AssetRegistryModule, TMap<FName, TMap<FString, TMap<int32, FString>>>& BlueprintWaveMap)
{
	TArray<FAssetData> BlueprintList;
	AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), BlueprintList);

	for (int32 BlueprintIndex = 0; BlueprintIndex < BlueprintList.Num(); ++BlueprintIndex)
	{
		{
			UBlueprint* Blueprint = CastChecked<UBlueprint>(BlueprintList[BlueprintIndex].GetAsset());
			ClearSoundNodeWavesInPackage(Blueprint->GetOutermost(), BlueprintWaveMap);
		}

		PeriodicallyCollectGarbage();
	}
}

void ClearSoundNodeWavesInLevels(TMap<FName, TMap<FString, TMap<int32, FString>>>& LevelWaveMap)
{
	TArray<FString> FilesInPath;
	FEditorFileUtils::FindAllPackageFiles(FilesInPath);

	for (int32 FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++)
	{
		const FString& Filename = FilesInPath[FileIndex];

		// if we don't want to load maps for this
		if (FPaths::GetExtension(Filename, true) != FPackageName::GetMapPackageExtension())
		{
			continue;
		}

		{
			UPackage* Package = LoadPackage( NULL, *Filename, LOAD_None );
			if (Package == NULL)
			{
				UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not load package '%s'. Any SoundNodeWave references will not be updated to SoundWave."), *Filename);
				continue;
			}
			ClearSoundNodeWavesInPackage(Package, LevelWaveMap);
		}

		// We don't do periodic here since levels are big, so clean up after each one
		CollectGarbage(RF_Native);
	}
}

// Go through and copy all the SoundNodeWave data in to the new SoundWave and delete the SoundNodeWave
void ReplaceSoundNodeWaveWithSoundWave(const FAssetRegistryModule& AssetRegistryModule)
{
	TArray<FAssetData> SoundNodeWaveList;
	AssetRegistryModule.Get().GetAssetsByClass(UDEPRECATED_SoundNodeWave::StaticClass()->GetFName(), SoundNodeWaveList);

	for (int32 SoundNodeIndex = 0; SoundNodeIndex < SoundNodeWaveList.Num(); ++SoundNodeIndex)
	{
		{
			UDEPRECATED_SoundNodeWave* SoundNodeWave = CastChecked<UDEPRECATED_SoundNodeWave>(SoundNodeWaveList[SoundNodeIndex].GetAsset());

			UPackage* Package = SoundNodeWave->GetOutermost();

			if (!CheckoutPackage(Package))
			{
				continue;
			}

			const FName SoundName = SoundNodeWave->GetFName();

			int32 DeprecatedCount = 0;
			while(FindObject<UObject>(GetTransientPackage(),*(SoundNodeWave->GetName() + TEXT("_DEPRECATED") + FString::FromInt(DeprecatedCount))))
			{
				++DeprecatedCount;
			}
			SoundNodeWave->Rename(*(SoundNodeWave->GetName() + TEXT("_DEPRECATED") + FString::FromInt(DeprecatedCount)),GetTransientPackage(),REN_DontCreateRedirectors);
			SoundNodeWave->ClearFlags(RF_Public | RF_Standalone);

			USoundWave* SoundWave = ConstructObject<USoundWave>(USoundWave::StaticClass(), Package, SoundName, RF_Public | RF_Standalone);

			SoundWave->CompressionQuality = SoundNodeWave->CompressionQuality;
			SoundWave->bLoopableSound = SoundNodeWave->bLoopingSound;
			SoundWave->SpokenText = SoundNodeWave->SpokenText;
			SoundWave->Volume = SoundNodeWave->Volume;
			SoundWave->Pitch = SoundNodeWave->Pitch;
			SoundWave->Duration = SoundNodeWave->Duration;
			SoundWave->NumChannels = SoundNodeWave->NumChannels;
			SoundWave->SampleRate = SoundNodeWave->SampleRate;
			SoundWave->ChannelOffsets = SoundNodeWave->ChannelOffsets;
			SoundWave->ChannelSizes = SoundNodeWave->ChannelSizes;
			SoundWave->RawPCMDataSize = SoundNodeWave->RawPCMDataSize;
			SoundWave->Subtitles = SoundNodeWave->Subtitles;
			SoundWave->bMature = SoundNodeWave->bMature;
			SoundWave->Comment = SoundNodeWave->Comment;
			SoundWave->bManualWordWrap = SoundNodeWave->bManualWordWrap;
			SoundWave->bSingleLine = SoundNodeWave->bSingleLine;
			SoundWave->LocalizedSubtitles = SoundNodeWave->LocalizedSubtitles;
			SoundWave->SourceFilePath = SoundNodeWave->SourceFilePath;
			SoundWave->SourceFileTimestamp = SoundNodeWave->SourceFileTimestamp;
			SoundWave->RawData = SoundNodeWave->RawData;

			SavePackage(Package);
		}

		PeriodicallyCollectGarbage();
	}
}

// Goes through all the wave player to sound name mappings and hooks up the SoundNodeWavePlayer to the SoundWave
void UpdateWavePlayersInSoundCues(FAssetRegistryModule& AssetRegistryModule, const TMap<FName, TArray<FWavePlayerSoundPair>>& WavePlayerSoundMap)
{
	for (auto PackageIt = WavePlayerSoundMap.CreateConstIterator(); PackageIt; ++PackageIt)
	{
		FString PackageFilename;
		if ( FPackageName::DoesPackageExist( PackageIt.Key().ToString(), NULL, &PackageFilename ) == false )
		{
			UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not find package '%s'."), *(PackageIt.Key().ToString()));
			continue;
		}

		{
			UPackage* Package = LoadPackage(NULL, *PackageFilename, LOAD_None);
			if (Package == NULL)
			{
				UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Could not load package '%s'."), *PackageFilename);
				continue;
			}

			const TArray<FWavePlayerSoundPair>& PackageWavePlayerSoundList = PackageIt.Value();
			for (int32 Index = 0; Index < PackageWavePlayerSoundList.Num(); ++Index)
			{
				USoundNodeWavePlayer* WavePlayer = FindObject<USoundNodeWavePlayer>(NULL, *PackageWavePlayerSoundList[Index].WavePlayerName);

				if (WavePlayer)
				{
					WavePlayer->SoundWave = LoadObject<USoundWave>(NULL, *PackageWavePlayerSoundList[Index].SoundName);
				}
				else
				{
					UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Did not find expected SoundNodeWavePlayer '%s' in package '%s'."), *PackageWavePlayerSoundList[Index].WavePlayerName, *PackageFilename);
					continue;
				}
			}

			SavePackage(Package);
		}

		PeriodicallyCollectGarbage();
	}
}

void SplitSoundNodeWaves()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// For phase 1 of the migration we need SoundNodeWave class to not be deprecated so that any packages containing both a 
	// Sound Cue or Blueprint and a SoundNodeWave doesn't wipe the SoundNodeWave out when the package is saved
	UClass *SoundNodeWaveClass = FindObject<UClass>(NULL, TEXT("Engine.SoundNodeWave"), true);
	SoundNodeWaveClass->ClassFlags &= ~(CLASS_Deprecated);

	// SoundNodeAmbient, SoundNodeAmbientNonLoop, and SoundNodeAmbientNonLoopToggle need to not be deprecated while we are splitting
	// the sound node waves otherwise we will be unable to convert the AmbientSound sound nodes into a SoundCue
	UClass *SoundNodeAmbientClass = FindObject<UClass>(NULL, TEXT("Engine.SoundNodeAmbient"), true);
	UClass *SoundNodeAmbientNonLoopClass = FindObject<UClass>(NULL, TEXT("Engine.SoundNodeAmbientNonLoop"), true);
	UClass *SoundNodeAmbientNonLoopToggleClass = FindObject<UClass>(NULL, TEXT("Engine.SoundNodeAmbientNonLoopToggle"), true);
	SoundNodeAmbientClass->ClassFlags &= ~(CLASS_Deprecated);
	SoundNodeAmbientNonLoopClass->ClassFlags &= ~(CLASS_Deprecated);
	SoundNodeAmbientNonLoopToggleClass->ClassFlags &= ~(CLASS_Deprecated);

	// AmbientSound class needs to be flagged not to migrate settings until the data is ready
	AAmbientSound::bUE4AudioRefactorMigrationUnderway = true;

	// Go through all the sound cues and replace the SoundNodeWave nodes with SoundNodeWavePlayer nodes
	// keeping track of what SoundWave each player will need to reference
	TMap<FName, TArray<FWavePlayerSoundPair>> WavePlayerSoundMap;
	ReplaceSoundNodeWavesInSoundCues(AssetRegistryModule, WavePlayerSoundMap);

	// We must also go through all blueprints and levels to find AudioComponent objects that might reference a SoundNodeWave
	TMap<FName, TMap<FString, TMap<int32, FString>>> SoundWaveMap;
	ClearSoundNodeWavesInBlueprints(AssetRegistryModule, SoundWaveMap);
	ClearSoundNodeWavesInLevels(SoundWaveMap);

	SoundNodeWaveClass->ClassFlags |= CLASS_Deprecated;
	SoundNodeAmbientClass->ClassFlags |= CLASS_Deprecated;
	SoundNodeAmbientNonLoopClass->ClassFlags |= CLASS_Deprecated;
	SoundNodeAmbientNonLoopToggleClass->ClassFlags |= CLASS_Deprecated;
	AAmbientSound::bUE4AudioRefactorMigrationUnderway = false;

	// Go through and copy all the SoundNodeWave data in to the new SoundWave and delete the SoundNodeWave
	ReplaceSoundNodeWaveWithSoundWave(AssetRegistryModule);

	// Go back through all the SoundCues and hookup the SoundNodeWavePlayer to the newly created SoundWave
	UpdateWavePlayersInSoundCues(AssetRegistryModule, WavePlayerSoundMap);

	// Go back through all the AudioComponents and establish reference to newly created SoundWave
	UpdateSoundWavesInPackages(SoundWaveMap);
}

// Returns if there was an error that should abort processing
EMigrationResult MigrateAttenuationSettings(USoundCue* SoundCue, bool& bPackageDirty)
{
	if (SoundCue)
	{
		int32 NodeCount = 0;
		for (int32 NodeIndex = 0; NodeIndex < SoundCue->AllNodes.Num(); ++ NodeIndex)
		{
			USoundNodeAttenuation *AttenuationNode = Cast<USoundNodeAttenuation>(SoundCue->AllNodes[NodeIndex]);
			if (AttenuationNode)
			{
				++NodeCount;
			}
		}
		if (NodeCount == 1)
		{ 
			USoundNodeAttenuation *AttenuationNode = Cast<USoundNodeAttenuation>(SoundCue->FirstNode);
			if (AttenuationNode)
			{
				UPackage* Package = SoundCue->GetOutermost();
				if (!bPackageDirty && !CheckoutPackage(Package))
				{
					return MIGRATION_ABORT;
				}
				bPackageDirty = true;

				SoundCue->AttenuationSettings = AttenuationNode->AttenuationSettings;
				SoundCue->bOverrideAttenuation = AttenuationNode->bOverrideAttenuation;
				SoundCue->AttenuationOverrides = AttenuationNode->AttenuationOverrides;

				if (AttenuationNode->ChildNodes.Num() > 0)
				{
					SoundCue->FirstNode = AttenuationNode->ChildNodes[0];
					SoundCue->AllNodes.Remove(AttenuationNode);
					SoundCue->LinkGraphNodesFromSoundNodes();
				}
			}
			else
			{
				UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("%s has 1 sound attenuation node but is not FirstNode. Not migrating attenuation settings."), *SoundCue->GetPathName());
			}
		}
		else if (NodeCount > 1)
		{
			UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("%s has %d attenuation nodes. Not migrating attenuation settings."), *SoundCue->GetPathName(), NodeCount);
		}
	}

	return MIGRATION_OK;
}

// Only looping nodes that are connected directly to a SoundNodeWavePlayer are removed and the looping flag on the WavePlayer is used
EMigrationResult MigrateLooping(USoundCue* SoundCue, bool& bPackageDirty)
{
	if (SoundCue)
	{
		int32 NodeCount = 0;
		TArray<USoundNode*> NodesToRemove;
		for (int32 NodeIndex = 0; NodeIndex < SoundCue->AllNodes.Num(); ++NodeIndex)
		{
			USoundNode* SoundNode = SoundCue->AllNodes[NodeIndex];
			if (SoundNode)
			{
				for(int32 ChildIndex = 0; ChildIndex < SoundNode->ChildNodes.Num(); ++ChildIndex)
				{
					USoundNodeLooping* LoopingNode = Cast<USoundNodeLooping>(SoundNode->ChildNodes[ChildIndex]);
					if (LoopingNode && LoopingNode->ChildNodes.Num() > 0)
					{
						check(LoopingNode->ChildNodes.Num() == 1);

						USoundNodeWavePlayer* PlayerNode = Cast<USoundNodeWavePlayer>(LoopingNode->ChildNodes[0]);
						if (PlayerNode)
						{
							if (!bPackageDirty && !CheckoutPackage(SoundCue->GetOutermost()))
							{
								return MIGRATION_ABORT;
							}
							bPackageDirty = true;

							PlayerNode->bLooping = true;
							SoundNode->ChildNodes[ChildIndex] = LoopingNode->ChildNodes[0];
							NodesToRemove.Add(LoopingNode);
						}
					}
				}
			}
			else
			{
				// Take the opportunity to get this NULL key out
				if (!bPackageDirty && !CheckoutPackage(SoundCue->GetOutermost()))
				{
					return MIGRATION_ABORT;
				}
				bPackageDirty = true;

				NodesToRemove.Add(SoundNode);
			}
		}

		USoundNodeLooping* LoopingNode = Cast<USoundNodeLooping>(SoundCue->FirstNode);
		if (LoopingNode && LoopingNode->ChildNodes.Num() > 0)
		{
			check(LoopingNode->ChildNodes.Num() == 1);

			USoundNodeWavePlayer* PlayerNode = Cast<USoundNodeWavePlayer>(LoopingNode->ChildNodes[0]);
			if (PlayerNode)
			{
				if (!bPackageDirty && !CheckoutPackage(SoundCue->GetOutermost()))
				{
					return MIGRATION_ABORT;
				}
				bPackageDirty = true;

				PlayerNode->bLooping = true;
				SoundCue->FirstNode = LoopingNode->ChildNodes[0];
				NodesToRemove.Add(LoopingNode);
			}
		}

		for (int32 NodeIndex = 0; NodeIndex < NodesToRemove.Num(); ++NodeIndex)
		{
			SoundCue->AllNodes.Remove(NodesToRemove[NodeIndex]);
		}

		SoundCue->LinkGraphNodesFromSoundNodes();
	}

	return MIGRATION_OK;
}

void ProcessSoundCues()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> SoundCueList;
	AssetRegistryModule.Get().GetAssetsByClass(USoundCue::StaticClass()->GetFName(), SoundCueList);

	TArray<FString> PackagesToDelete;
	for( int32 SoundCueIndex = 0; SoundCueIndex < SoundCueList.Num(); ++SoundCueIndex )
	{
		{
			bool bPackageDirty = false;
			USoundCue* SoundCue = CastChecked<USoundCue>(SoundCueList[SoundCueIndex].GetAsset());
			if (MigrateAttenuationSettings(SoundCue, bPackageDirty) == MIGRATION_ABORT) continue;
			if (MigrateLooping(SoundCue, bPackageDirty) == MIGRATION_ABORT) continue;

			if (bPackageDirty)
			{
				UPackage* Package = SoundCue->GetOutermost();
				SavePackage(Package);
			}
		}

		PeriodicallyCollectGarbage();
	}
}

void FixupSoundClasses()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> SoundClassList;
	AssetRegistryModule.Get().GetAssetsByClass(USoundClass::StaticClass()->GetFName(), SoundClassList);

	struct FSoundClassInfo
	{
		USoundClass* SoundClass;
		bool bAmbiguous;

		FSoundClassInfo()
			: SoundClass(NULL)
			, bAmbiguous(false)
		{
		}
	};
	TMap<FName, FSoundClassInfo> SoundClassInfoMap;

	// First gather all the sound classes and identify ambiguous conflicts
	for (int32 SoundClassIndex = 0; SoundClassIndex < SoundClassList.Num(); ++SoundClassIndex)
	{
		USoundClass* SoundClass = CastChecked<USoundClass>(SoundClassList[SoundClassIndex].GetAsset());
		FSoundClassInfo* SoundClassInfo = SoundClassInfoMap.Find(SoundClass->GetFName());
		if (SoundClassInfo)
		{
			UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Ambiguous sound class %s.  %s used over %s."), 
				*SoundClass->GetName(), *SoundClassInfo->SoundClass->GetPathName(), *SoundClass->GetPathName());
			SoundClassInfo->bAmbiguous = true;
		}
		else
		{
			FSoundClassInfo NewInfo;
			NewInfo.SoundClass = SoundClass;
			SoundClassInfoMap.Add(SoundClass->GetFName(), NewInfo);
		}
	}

	// Go through sound classes and convert the class names to objects
	for (int32 SoundClassIndex = 0; SoundClassIndex < SoundClassList.Num(); ++SoundClassIndex)
	{
		USoundClass* SoundClass = CastChecked<USoundClass>(SoundClassList[SoundClassIndex].GetAsset());
		const int32 ChildClassCount = SoundClass->ChildClassNames_DEPRECATED.Num();
		if (ChildClassCount > 0)
		{
			UPackage* Package = SoundClass->GetOutermost();
			if (!CheckoutPackage(Package))
			{
				continue;
			}

			SoundClass->ChildClasses.Init(ChildClassCount);
			for (int32 ChildClassIndex = 0; ChildClassIndex < ChildClassCount; ++ChildClassIndex)
			{
				const FName ChildClassName = SoundClass->ChildClassNames_DEPRECATED[ChildClassIndex];
				FSoundClassInfo* SoundClassInfo = SoundClassInfoMap.Find(ChildClassName);
				if (SoundClassInfo)
				{
					SoundClass->ChildClasses[ChildClassIndex] = SoundClassInfo->SoundClass;
					SoundClass->ChildClasses[ChildClassIndex]->SetParentClass(SoundClass);
					if (SoundClassInfo->bAmbiguous)
					{
						UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("%s references ambiguous sound class %s."), 
							*SoundClass->GetName(), *ChildClassName.ToString());
					}
				}
				else
				{
					UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("Unknown sound class %s referenced by %s."), 
						*ChildClassName.ToString(), *SoundClass->GetName());
				}
			}

			SavePackage(Package);
		}
	}

	// Go through sound cues and waves and update their class reference
	TArray<FAssetData> SoundList;
	AssetRegistryModule.Get().GetAssetsByClass(USoundBase::StaticClass()->GetFName(), SoundList, true);

	for (int32 SoundIndex = 0; SoundIndex < SoundList.Num(); ++SoundIndex)
	{
		USoundBase* Sound = CastChecked<USoundBase>(SoundList[SoundIndex].GetAsset());

		if (Sound->SoundClass_DEPRECATED != NAME_None)
		{
			FSoundClassInfo* SoundClassInfo = SoundClassInfoMap.Find(Sound->SoundClass_DEPRECATED);
			if (SoundClassInfo)
			{
				UPackage* Package = Sound->GetOutermost();
				if (!CheckoutPackage(Package))
				{
					continue;
				}

				Sound->SoundClassObject = SoundClassInfo->SoundClass;
				if (SoundClassInfo->bAmbiguous)
				{
					UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("%s references ambiguous sound class %s."), 
						*Sound->GetName(), *Sound->SoundClass_DEPRECATED.ToString());
				}

				SavePackage(Package);
			}
		}
	}

	// Go through sound mixes and update their class reference
	TArray<FAssetData> SoundMixList;
	AssetRegistryModule.Get().GetAssetsByClass(USoundMix::StaticClass()->GetFName(), SoundMixList);

	for (int32 SoundMixIndex = 0; SoundMixIndex < SoundMixList.Num(); ++SoundMixIndex)
	{
		USoundMix* SoundMix = CastChecked<USoundMix>(SoundMixList[SoundMixIndex].GetAsset());

		if (SoundMix->SoundClassEffects.Num() > 0)
		{
			UPackage* Package = SoundMix->GetOutermost();
			if (!CheckoutPackage(Package))
			{
				continue;
			}

			for (int32 AdjusterIndex = 0; AdjusterIndex < SoundMix->SoundClassEffects.Num(); ++AdjusterIndex)
			{
				FSoundClassAdjuster& Adjuster = SoundMix->SoundClassEffects[AdjusterIndex];
				if (Adjuster.SoundClass_DEPRECATED != NAME_None)
				{
					FSoundClassInfo* SoundClassInfo = SoundClassInfoMap.Find(Adjuster.SoundClass_DEPRECATED);
					if (SoundClassInfo)
					{
						Adjuster.SoundClassObject = SoundClassInfo->SoundClass;
						if (SoundClassInfo->bAmbiguous)
						{
							UE_LOG(LogUE4AudioRefactorMigrationCommandlet, Warning, TEXT("%s references ambiguous sound class %s."), 
								*SoundMix->GetName(), *Adjuster.SoundClass_DEPRECATED.ToString());
						}
					}
				}
			}

			SavePackage(Package);
		}
	}
}
}

/*-----------------------------------------------------------------------------
	UE4 Audio Refactor Migration commandlet.
-----------------------------------------------------------------------------*/
UUE4AudioRefactorMigrationCommandlet::UUE4AudioRefactorMigrationCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	LogToConsole = true;
}	

int32 UUE4AudioRefactorMigrationCommandlet::Main( const FString& Params )
{
	TArray<FString> Tokens, Switches;
	ParseCommandLine(*Params, Tokens, Switches);

	ISourceControlModule::Get().GetProvider().Init();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().SearchAllAssets(true);

	const bool bAll = (Switches.Num() == 0);

	// Examines sound cues and deletes any that contain an unsupported sound node
	if (bAll || Switches.Contains(TEXT("DELETEUNSUPPORTEDSOUNDCUES")))
	{
		DeleteSoundCuesWithUnsupportedNodes();
	}
	// Splits SoundNodeWaves in to runtime (SoundNodeWavePlayer) and asset (SoundWave)
	if (bAll || Switches.Contains(TEXT("SPLITSOUNDNODEWAVES")))
	{
		SplitSoundNodeWaves();
	}
	// Migrates looping and attenuation settings out of nodes in the sound cue and in to properties on the cue itself
	if (bAll || Switches.Contains(TEXT("MIGRATESOUNDCUE")))
	{
		ProcessSoundCues();
	}
	// Converts SoundClasses from name based referencing to object based
	if (bAll || Switches.Contains(TEXT("FIXUPSOUNDCLASSES")))
	{
		FixupSoundClasses();
	}

	ISourceControlModule::Get().GetProvider().Close(); // clean up our allocated SCC

	return 0;
}
