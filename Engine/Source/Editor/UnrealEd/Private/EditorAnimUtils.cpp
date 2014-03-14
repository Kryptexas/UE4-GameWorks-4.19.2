// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AssetData.h"
#include "EditorAnimUtils.h"
#include "BlueprintEditorUtils.h"
#include "KismetEditorUtilities.h"
#include "AnimGraphDefinitions.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#include "NotificationManager.h"
#include "Editor/Persona/Public/PersonaModule.h"

#define LOCTEXT_NAMESPACE "EditorAnimUtils"

namespace EditorAnimUtils
{
	//////////////////////////////////////////////////////////////////
	// FAnimationRetargetContext

	FAnimationRetargetContext::FAnimationRetargetContext(const TArray<FAssetData>& AssetsToRetarget, bool bRetargetReferredAssets) : SingleTargetObject(NULL)
	{
		TArray<UObject*> Objects;
		for(auto Iter = AssetsToRetarget.CreateConstIterator(); Iter; ++Iter)
		{
			Objects.Add((*Iter).GetAsset());
		}
		Initialize(Objects,bRetargetReferredAssets);
	}

	FAnimationRetargetContext::FAnimationRetargetContext(const TArray<UObject*>& AssetsToRetarget, bool bRetargetReferredAssets) : SingleTargetObject(NULL)
	{
		Initialize(AssetsToRetarget,bRetargetReferredAssets);
	}

	void FAnimationRetargetContext::Initialize(const TArray<UObject*>& AssetsToRetarget, bool bRetargetReferredAssets)
	{
		for(auto Iter = AssetsToRetarget.CreateConstIterator(); Iter; ++Iter)
		{
			UObject* Asset = (*Iter);
			if( UAnimSequence* AnimSeq = Cast<UAnimSequence>(Asset) )
			{
				AnimSequencesToRetarget.AddUnique(AnimSeq);
			}
			else if( UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(Asset) )
			{
				ComplexAnimsToRetarget.AddUnique(AnimAsset);
			}
			else if( UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(Asset) )
			{
				AnimBlueprintsToRetarget.AddUnique(AnimBlueprint);
			}
		}
		
		if(AssetsToRetarget.Num() == 1)
		{
			//Only chose one object to retarget, keep track of it
			SingleTargetObject = AssetsToRetarget[0];
		}

		if(bRetargetReferredAssets)
		{
			for(auto Iter = ComplexAnimsToRetarget.CreateConstIterator(); Iter; ++Iter)
			{
				(*Iter)->GetAllAnimationSequencesReferred(AnimSequencesToRetarget);
			}

			for(auto Iter = AnimBlueprintsToRetarget.CreateConstIterator(); Iter; ++Iter)
			{
				GetAllAnimationSequencesReferredInBlueprint( (*Iter), ComplexAnimsToRetarget, AnimSequencesToRetarget);
			}
		}
	}

	bool FAnimationRetargetContext::HasAssetsToRetarget() const
	{
		return	AnimSequencesToRetarget.Num() > 0 ||
				ComplexAnimsToRetarget.Num() > 0  ||
				AnimBlueprintsToRetarget.Num() > 0;
	}

	bool FAnimationRetargetContext::HasDuplicates() const
	{
		return	DuplicatedSequences.Num() > 0     ||
				DuplicatedComplexAssets.Num() > 0 ||
				DuplicatedBlueprints.Num() > 0;
	}

	UObject* FAnimationRetargetContext::GetSingleTargetObject() const
	{
		return SingleTargetObject;
	}

	UObject* FAnimationRetargetContext::GetDuplicate(const UObject* OriginalObject) const
	{
		if(HasDuplicates())
		{
			if(const UAnimSequence* Seq = Cast<const UAnimSequence>(OriginalObject))
			{
				if(DuplicatedSequences.Contains(Seq))
				{
					return DuplicatedSequences.FindRef(Seq);
				}
			}
			if(const UAnimationAsset* Asset = Cast<const UAnimationAsset>(OriginalObject)) 
			{
				if(DuplicatedComplexAssets.Contains(Asset))
				{
					return DuplicatedComplexAssets.FindRef(Asset);
				}
			}
			if(const UAnimBlueprint* AnimBlueprint = Cast<const UAnimBlueprint>(OriginalObject))
			{
				if(DuplicatedBlueprints.Contains(AnimBlueprint))
				{
					return DuplicatedBlueprints.FindRef(AnimBlueprint);
				}
			}
		}
		return NULL;
	}

	void FAnimationRetargetContext::DuplicateAssetsToRetarget(UPackage* DestinationPackage)
	{
		if(!HasDuplicates())
		{
			DuplicatedSequences = DuplicateAssets<UAnimSequence>(AnimSequencesToRetarget, DestinationPackage);
			DuplicatedComplexAssets = DuplicateAssets<UAnimationAsset>(ComplexAnimsToRetarget, DestinationPackage);
			DuplicatedBlueprints = DuplicateAssets<UAnimBlueprint>(AnimBlueprintsToRetarget, DestinationPackage);

			DuplicatedSequences.GenerateValueArray(AnimSequencesToRetarget);
			DuplicatedComplexAssets.GenerateValueArray(ComplexAnimsToRetarget);
			DuplicatedBlueprints.GenerateValueArray(AnimBlueprintsToRetarget);
		}
	}

	void FAnimationRetargetContext::RetargetAnimations(USkeleton* NewSkeleton)
	{
		for(auto Iter = AnimSequencesToRetarget.CreateIterator(); Iter; ++Iter)
		{
			UAnimSequence* AssetToRetarget = (*Iter);
			AssetToRetarget->ReplaceSkeleton(NewSkeleton);
		}

		for(auto Iter = ComplexAnimsToRetarget.CreateIterator(); Iter; ++Iter)
		{
			UAnimationAsset* AssetToRetarget = (*Iter);
			if(HasDuplicates())
			{
				AssetToRetarget->ReplaceReferredAnimations(DuplicatedSequences);
			}
			AssetToRetarget->ReplaceSkeleton(NewSkeleton);
		}

		// convert all Animation Blueprints and compile 
		for ( auto AnimBPIter = AnimBlueprintsToRetarget.CreateIterator(); AnimBPIter; ++AnimBPIter )
		{
			UAnimBlueprint * AnimBlueprint = (*AnimBPIter);

			AnimBlueprint->TargetSkeleton = NewSkeleton;
			if(HasDuplicates())
			{
				ReplaceReferredAnimationsInBlueprint(AnimBlueprint, DuplicatedComplexAssets, DuplicatedSequences);
			}

			bool bIsRegeneratingOnLoad = false;
			bool bSkipGarbageCollection = true;
			FBlueprintEditorUtils::RefreshAllNodes(AnimBlueprint);
			FKismetEditorUtilities::CompileBlueprint(AnimBlueprint, bIsRegeneratingOnLoad, bSkipGarbageCollection);
			AnimBlueprint->PostEditChange();
			AnimBlueprint->MarkPackageDirty();
		}
	}

	void OpenAssetFromNotify(UObject* AssetToOpen)
	{
		EToolkitMode::Type Mode = EToolkitMode::Standalone;
		FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>( "Persona" );

		if(UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(AssetToOpen))
		{
			PersonaModule.CreatePersona( Mode, TSharedPtr<IToolkitHost>(), AnimAsset->GetSkeleton(), NULL, AnimAsset, NULL );
		}
		else if(UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(AssetToOpen))
		{
			PersonaModule.CreatePersona( Mode, TSharedPtr<IToolkitHost>(), AnimBlueprint->TargetSkeleton, AnimBlueprint, NULL, NULL );
		}
	}

	//////////////////////////////////////////////////////////////////
	UObject* RetargetAnimations(USkeleton* NewSkeleton, const TArray<UObject*>& AssetsToRetarget, bool bRetargetReferredAssets, bool bDuplicateAssetsBeforeRetarget)
	{
		FAnimationRetargetContext RetargetContext(AssetsToRetarget, bRetargetReferredAssets);
		return RetargetAnimations(NewSkeleton, RetargetContext, bRetargetReferredAssets, bDuplicateAssetsBeforeRetarget);
	}

	UObject* RetargetAnimations(USkeleton* NewSkeleton, const TArray<FAssetData>& AssetsToRetarget, bool bRetargetReferredAssets, bool bDuplicateAssetsBeforeRetarget)
	{
		FAnimationRetargetContext RetargetContext(AssetsToRetarget, bRetargetReferredAssets);
		return RetargetAnimations(NewSkeleton, RetargetContext, bRetargetReferredAssets, bDuplicateAssetsBeforeRetarget);
	}

	UObject* RetargetAnimations(USkeleton* NewSkeleton, FAnimationRetargetContext& RetargetContext, bool bRetargetReferredAssets, bool bDuplicateAssetsBeforeRetarget)
	{
		check(NewSkeleton);
		UObject* OriginalObject  = RetargetContext.GetSingleTargetObject();
		UPackage* DuplicationDestPackage = NewSkeleton->GetOutermost();

		if(	RetargetContext.HasAssetsToRetarget() )
		{
			if(bDuplicateAssetsBeforeRetarget)
			{
				RetargetContext.DuplicateAssetsToRetarget(DuplicationDestPackage);
			}
			RetargetContext.RetargetAnimations(NewSkeleton);
		}

		FNotificationInfo Notification(FText::GetEmpty());
		Notification.ExpireDuration = 5.f;

		UObject* NotifyLinkObject = OriginalObject;
		if(OriginalObject && bDuplicateAssetsBeforeRetarget)
		{
			NotifyLinkObject = RetargetContext.GetDuplicate(OriginalObject);
		}

		if(!bDuplicateAssetsBeforeRetarget)
		{
			if(OriginalObject)
			{
				Notification.Text = FText::Format(LOCTEXT("SingleNonDuplicatedAsset", "'{0}' retargeted to new skeleton '{1}'"), FText::FromString(OriginalObject->GetName()), FText::FromString(NewSkeleton->GetName()));
			}
			else
			{
				Notification.Text = FText::Format(LOCTEXT("MultiNonDuplicatedAsset", "Assets retargeted to new skeleton '{0}'"), FText::FromString(NewSkeleton->GetName()));
			}
			
		}
		else
		{
			if(OriginalObject)
			{
				Notification.Text = FText::Format(LOCTEXT("SingleDuplicatedAsset", "'{0}' duplicated to '{1}' and retargeted"), FText::FromString(OriginalObject->GetName()), FText::FromString(DuplicationDestPackage->GetName()));
			}
			else
			{
				Notification.Text = FText::Format(LOCTEXT("MultiDuplicatedAsset", "Assets duplicated to '{0}' and retargeted"), FText::FromString(DuplicationDestPackage->GetName()));
			}
		}

		if(NotifyLinkObject)
		{
			Notification.Hyperlink = FSimpleDelegate::CreateStatic(&OpenAssetFromNotify, NotifyLinkObject);
			Notification.HyperlinkText = LOCTEXT("OpenAssetLink", "Open");
		}

		FSlateNotificationManager::Get().AddNotification(Notification);

		if(OriginalObject && bDuplicateAssetsBeforeRetarget)
		{
			return RetargetContext.GetDuplicate(OriginalObject);
		}
		return NULL;
	}

	TMap<UObject*, UObject*> DuplicateAssetsInternal(const TArray<UObject*>& AssetsToDuplicate, UPackage* DestinationPackage)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

		TMap<UObject*, UObject*> DuplicateMap;

		for(auto Iter = AssetsToDuplicate.CreateConstIterator(); Iter; ++Iter)
		{
			UObject* Asset = (*Iter);
			if(!DuplicateMap.Contains(Asset))
			{
				FString PathName = FPackageName::GetLongPackagePath(DestinationPackage->GetName());

				FString ObjectName;
				FString NewPackageName;
				AssetToolsModule.Get().CreateUniqueAssetName(PathName+"/"+ Asset->GetName(), TEXT("_Copy"), NewPackageName, ObjectName);

				// create one on skeleton folder
				UObject * NewAsset = AssetToolsModule.Get().DuplicateAsset(ObjectName, PathName, Asset);
				if ( NewAsset )
				{
					DuplicateMap.Add(Asset, NewAsset);
				}
			}
		}

		return DuplicateMap;
	}

	void GetAllAnimationSequencesReferredInBlueprint(UAnimBlueprint* AnimBlueprint, TArray<UAnimationAsset*>& ComplexAnims, TArray<UAnimSequence*>& AnimSequences)
	{
		TArray<UEdGraph*> Graphs;
		AnimBlueprint->GetAllGraphs(Graphs);
		for(auto GraphIter = Graphs.CreateConstIterator(); GraphIter; ++GraphIter)
		{
			const UEdGraph* Graph = *GraphIter;
			for(auto NodeIter = Graph->Nodes.CreateConstIterator(); NodeIter; ++NodeIter)
			{
				if(const UAnimGraphNode_Base* AnimNode = Cast<UAnimGraphNode_Base>(*NodeIter))
				{
					AnimNode->GetAllAnimationSequencesReferred(ComplexAnims, AnimSequences);
				}
			}
		}
	}

	void ReplaceReferredAnimationsInBlueprint(UAnimBlueprint* AnimBlueprint, const TMap<UAnimationAsset*, UAnimationAsset*>& ComplexAnimMap, const TMap<UAnimSequence*, UAnimSequence*>& AnimSequenceMap)
	{
		TArray<UEdGraph*> Graphs;
		AnimBlueprint->GetAllGraphs(Graphs);
		for(auto GraphIter = Graphs.CreateIterator(); GraphIter; ++GraphIter)
		{
			UEdGraph* Graph = *GraphIter;
			for(auto NodeIter = Graph->Nodes.CreateIterator(); NodeIter; ++NodeIter)
			{
				if(UAnimGraphNode_Base* AnimNode = Cast<UAnimGraphNode_Base>(*NodeIter))
				{
					AnimNode->ReplaceReferredAnimations(ComplexAnimMap, AnimSequenceMap);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE