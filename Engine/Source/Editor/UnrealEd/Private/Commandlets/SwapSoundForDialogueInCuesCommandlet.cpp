#include "UnrealEd.h"
#include "Commandlets/SwapSoundForDialogueInCuesCommandlet.h"
#include "Commandlets/GatherTextCommandletBase.h" // We use FGatherTextSCC since it's a nice wrapper around the default SCC
#include "AssetRegistryModule.h"
#include "PackageHelperFunctions.h"
#include "Sound/DialogueWave.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundNode.h"
#include "Sound/SoundNodeDialoguePlayer.h"
#include "Sound/SoundNodeWavePlayer.h"

DEFINE_LOG_CATEGORY_STATIC(LogSwapSoundForDialogueInCuesCommandlet, Log, All);

int32 USwapSoundForDialogueInCuesCommandlet::Main(const FString& Params)
{
	// Prepare asset registry.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Parameters;
	UCommandlet::ParseCommandLine(*Params, Tokens, Switches, Parameters);

	TSharedPtr<FGatherTextSCC> SourceControlInfo;
	const bool bEnableSourceControl = Switches.Contains(TEXT("EnableSCC"));
	if (bEnableSourceControl)
	{
		SourceControlInfo = MakeShareable(new FGatherTextSCC());

		FText SCCErrorStr;
		if (!SourceControlInfo->IsReady(SCCErrorStr))
		{
			UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Source Control error: %s"), *SCCErrorStr.ToString());
			return -1;
		}
	}

	TArray<FAssetData> AssetDataArrayForDialogueWaves;
	if (!AssetRegistry.GetAssetsByClass(UDialogueWave::StaticClass()->GetFName(), AssetDataArrayForDialogueWaves, true))
	{
		UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Unable to get dialogue wave asset data from asset registry."));
		return -1;
	}

	for (const FAssetData& AssetData : AssetDataArrayForDialogueWaves)
	{
		// Verify that the found asset is a dialogue wave.
		if (AssetData.GetClass() != UDialogueWave::StaticClass())
		{
			UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Asset registry found asset (%s), but the asset with this name is not actually a dialogue wave."), *AssetData.AssetName.ToString());
			continue;
		}

		// Get the dialogue wave.
		UDialogueWave* const DialogueWave = Cast<UDialogueWave>(AssetData.GetAsset());

		// Verify that the dialogue wave was loaded.
		if (!DialogueWave)
		{
			UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Asset registry found asset (%s), but the dialogue wave could not be accessed."), *AssetData.AssetName.ToString());
			continue;
		}

		// Iterate over each of the contexts and fix up the sound cue nodes referencing this sound wave.
		for (const FDialogueContextMapping& ContextMapping : DialogueWave->ContextMappings)
		{
			// Skip contexts without sound waves.
			const USoundWave* const SoundWave = ContextMapping.SoundWave;
			if (!SoundWave)
			{
				continue;
			}

			// Verify that the sound wave has a package.
			const UPackage* const SoundWavePackage = SoundWave->GetOutermost();
			if (!SoundWavePackage)
			{
				UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Asset registry found dialogue wave (%s) with a context referencing sound wave (%s) but no package exists for this sound wave."), *AssetData.AssetName.ToString(), *SoundWave->GetName());
				continue;
			}
			
			// Find referencers of the context's sound wave.
			TArray<FName> SoundWaveReferencerNames;
			if (!AssetRegistry.GetReferencers(SoundWavePackage->GetFName(), SoundWaveReferencerNames))
			{
				UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Asset registry found dialogue wave (%s) with a context referencing sound wave (%s) but failed to search for referencers of the sound wave."), *AssetData.AssetName.ToString(), *SoundWave->GetName());
				continue;
			}

			// Skip further searching if there are no sound wave referencers.
			if (SoundWaveReferencerNames.Num() == 0)
			{
				continue;
			}

			// Get sound cue assets that reference the context's sound wave.
			FARFilter Filter;
			Filter.ClassNames.Add(USoundCue::StaticClass()->GetFName());
			Filter.bRecursiveClasses = true;
			Filter.PackageNames = SoundWaveReferencerNames;
			TArray<FAssetData> ReferencingSoundCueAssetDataArray;
			if (!AssetRegistry.GetAssets(Filter, ReferencingSoundCueAssetDataArray))
			{
				UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Asset registry found dialogue wave (%s) with a context referencing sound wave (%s) but failed to search for sound cues referencing the sound wave."), *AssetData.AssetName.ToString(), *SoundWave->GetName());
				continue;
			}

			// Iterate through referencing sound cues, finding sound node wave players and replacing them
			for (const FAssetData& SoundCueAssetData : ReferencingSoundCueAssetDataArray)
			{
				USoundCue* const SoundCue = Cast<USoundCue>(SoundCueAssetData.GetAsset());

				// Verify that the sound cue exists.
				if (!SoundCue)
				{
					UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Asset registry found dialogue wave (%s) with a context referencing sound wave (%s) but failed to access the referencing sound cue (%s)."), *AssetData.AssetName.ToString(), *SoundWave->GetName(), *SoundCueAssetData.AssetName.ToString());
					continue;
				}

				// Iterate through sound nodes in this sound cue and find those that need replacing.
				TArray<USoundNode*> NodesToReplace;
				for (USoundNode* const SoundNode : SoundCue->AllNodes)
				{
					USoundNodeWavePlayer* const SoundNodeWavePlayer = Cast<USoundNodeWavePlayer>(SoundNode);
					
					// Skip nodes that are not sound wave players or not referencing the sound wave in question.
					if (SoundNodeWavePlayer && SoundNodeWavePlayer->GetSoundWave() == SoundWave)
					{
						NodesToReplace.Add(SoundNode);
					}
				}
				if (NodesToReplace.Num() == 0)
				{
					continue;
				}

				// Replace any sound nodes in the graph.
				TArray<USoundCueGraphNode*> GraphNodesToRemove;
				for (USoundNode* const SoundNode : NodesToReplace)
				{
					// Create the new dialogue wave player.
					USoundNodeDialoguePlayer* DialoguePlayer = SoundCue->ConstructSoundNode<USoundNodeDialoguePlayer>();
					DialoguePlayer->SetDialogueWave(DialogueWave);
					DialoguePlayer->DialogueWaveParameter.Context = ContextMapping.Context;

					// We won't need the newly created graph node as we're about to move the dialogue wave player onto the original node.
					GraphNodesToRemove.Add(DialoguePlayer->GetGraphNode());

					// Swap out the sound wave player in the graph node with the new dialogue wave player.
					USoundCueGraphNode* SoundGraphNode = SoundNode->GetGraphNode();
					SoundGraphNode->SetSoundNode(DialoguePlayer);
				}

				for (USoundCueGraphNode* const SoundGraphNode : GraphNodesToRemove)
				{
					SoundCue->GetGraph()->RemoveNode(SoundGraphNode);
				}

				// Make sure the cue is updated to match its graph.
				SoundCue->CompileSoundNodesFromGraphNodes();
				
				for (USoundNode* const SoundNode : NodesToReplace)
				{
					// Remove the old node from the list of available nodes.
					SoundCue->AllNodes.Remove(SoundNode);
				}
				SoundCue->MarkPackageDirty();

				UPackage* const SoundCuePackage = SoundCue->GetOutermost();

				// Verify sound cue package exists.
				if (!SoundCuePackage)
				{
					UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Unable to find package for sound cue (%s)."), *SoundCueAssetData.AssetName.ToString());
					continue;
				}

				// Execute save.
				const FString PackageFileName = FPackageName::LongPackageNameToFilename(SoundCueAssetData.PackageName.ToString(), FPackageName::GetAssetPackageExtension(), false);
				if (bEnableSourceControl)
				{
					FText SCCErrorStr;
					if (!SourceControlInfo->CheckOutFile(PackageFileName, SCCErrorStr))
					{
						UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Failed to check-out sound cue (%s) at (%s). %s"), *AssetData.AssetName.ToString(), *PackageFileName, *SCCErrorStr.ToString());
					}
				}
				if (!SavePackageHelper(SoundCuePackage, PackageFileName))
				{
					UE_LOG(LogSwapSoundForDialogueInCuesCommandlet, Error, TEXT("Unable to save updated package for sound cue (%s) at (%s)."), *SoundCueAssetData.AssetName.ToString(), *PackageFileName);
					continue;
				}
			}
		}
	}

	return 0;
}
