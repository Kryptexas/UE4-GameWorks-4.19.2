// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AnimGraphNode_Base.h"
#include "AssetData.h"
#include "EditorAnimUtils.h"
#include "BlueprintEditorUtils.h"
#include "KismetEditorUtilities.h"
#include "AnimGraphDefinitions.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#include "NotificationManager.h"
#include "Editor/Persona/Public/PersonaModule.h"
#include "ObjectEditorUtils.h"
#include "SNotificationList.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "EditorAnimUtils"

namespace EditorAnimUtils
{
	//////////////////////////////////////////////////////////////////
	// FAnimationRetargetContext
	FAnimationRetargetContext::FAnimationRetargetContext(const TArray<FAssetData>& AssetsToRetarget, bool bRetargetReferredAssets, bool bInConvertAnimationDataInComponentSpaces, const FNameDuplicationRule& NameRule) 
		: SingleTargetObject(NULL)
		, bConvertAnimationDataInComponentSpaces(bInConvertAnimationDataInComponentSpaces)
	{
		TArray<UObject*> Objects;
		for(auto Iter = AssetsToRetarget.CreateConstIterator(); Iter; ++Iter)
		{
			Objects.Add((*Iter).GetAsset());
		}
		auto WeakObjectList = FObjectEditorUtils::GetTypedWeakObjectPtrs<UObject>(Objects);
		Initialize(WeakObjectList,bRetargetReferredAssets);
	}

	FAnimationRetargetContext::FAnimationRetargetContext(TArray<TWeakObjectPtr<UObject>> AssetsToRetarget, bool bRetargetReferredAssets, bool bInConvertAnimationDataInComponentSpaces, const FNameDuplicationRule& NameRule) 
		: SingleTargetObject(NULL)
		, bConvertAnimationDataInComponentSpaces(bInConvertAnimationDataInComponentSpaces)
	{
		Initialize(AssetsToRetarget,bRetargetReferredAssets);
	}

	void FAnimationRetargetContext::Initialize(TArray<TWeakObjectPtr<UObject>> AssetsToRetarget, bool bRetargetReferredAssets)
	{
		for(auto Iter = AssetsToRetarget.CreateConstIterator(); Iter; ++Iter)
		{
			UObject* Asset = (*Iter).Get();
			if( UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(Asset) )
			{
				AnimationAssetsToRetarget.AddUnique(AnimAsset);
			}
			else if( UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(Asset) )
			{
				AnimBlueprintsToRetarget.AddUnique(AnimBlueprint);
			}
		}
		
		if(AssetsToRetarget.Num() == 1)
		{
			//Only chose one object to retarget, keep track of it
			SingleTargetObject = AssetsToRetarget[0].Get();
		}

		if(bRetargetReferredAssets)
		{
			// Grab assets from the blueprint. Do this first as it can add complex assets to the retarget array
			// which will need to be processed next.
			for(auto Iter = AnimBlueprintsToRetarget.CreateConstIterator(); Iter; ++Iter)
			{
				GetAllAnimationSequencesReferredInBlueprint( (*Iter), AnimationAssetsToRetarget);
			}

			int32 AssetIndex = 0;
			while (AssetIndex < AnimationAssetsToRetarget.Num())
			{
				UAnimationAsset* AnimAsset = AnimationAssetsToRetarget[AssetIndex++];
				AnimAsset->HandleAnimReferenceCollection(AnimationAssetsToRetarget);
			}
		}
	}

	bool FAnimationRetargetContext::HasAssetsToRetarget() const
	{
		return	AnimationAssetsToRetarget.Num() > 0 ||
				AnimBlueprintsToRetarget.Num() > 0;
	}

	bool FAnimationRetargetContext::HasDuplicates() const
	{
		return	DuplicatedAnimAssets.Num() > 0     ||
				DuplicatedBlueprints.Num() > 0;
	}

	TArray<UObject*> FAnimationRetargetContext::GetAllDuplicates() const
	{
		TArray<UObject*> Duplicates;

		if (AnimationAssetsToRetarget.Num() > 0)
		{
			Duplicates.Append(AnimationAssetsToRetarget);
		}

		if(AnimBlueprintsToRetarget.Num() > 0)
		{
			Duplicates.Append(AnimBlueprintsToRetarget);
		}
		return Duplicates;
	}

	UObject* FAnimationRetargetContext::GetSingleTargetObject() const
	{
		return SingleTargetObject;
	}

	UObject* FAnimationRetargetContext::GetDuplicate(const UObject* OriginalObject) const
	{
		if(HasDuplicates())
		{
			if(const UAnimationAsset* Asset = Cast<const UAnimationAsset>(OriginalObject)) 
			{
				if(DuplicatedAnimAssets.Contains(Asset))
				{
					return DuplicatedAnimAssets.FindRef(Asset);
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

	void FAnimationRetargetContext::DuplicateAssetsToRetarget(UPackage* DestinationPackage, const FNameDuplicationRule* NameRule)
	{
		if(!HasDuplicates())
		{
			TArray<UAnimationAsset*> AnimationAssetsToDuplicate = AnimationAssetsToRetarget;
			TArray<UAnimBlueprint*> AnimBlueprintsToDuplicate = AnimBlueprintsToRetarget;

			// We only want to duplicate unmapped assets, so we remove mapped assets from the list we're duplicating
			for(TPair<UAnimationAsset*, UAnimationAsset*>& Pair : RemappedAnimAssets)
			{
				AnimationAssetsToDuplicate.Remove(Pair.Key);
			}

			DuplicatedAnimAssets = DuplicateAssets<UAnimationAsset>(AnimationAssetsToDuplicate, DestinationPackage, NameRule);
			DuplicatedBlueprints = DuplicateAssets<UAnimBlueprint>(AnimBlueprintsToDuplicate, DestinationPackage, NameRule);

			DuplicatedAnimAssets.GenerateValueArray(AnimationAssetsToRetarget);
			DuplicatedBlueprints.GenerateValueArray(AnimBlueprintsToRetarget);
		}
	}

	void FAnimationRetargetContext::RetargetAnimations(USkeleton* OldSkeleton, USkeleton* NewSkeleton)
	{
		check (!bConvertAnimationDataInComponentSpaces || OldSkeleton);
		check (NewSkeleton);

		if (bConvertAnimationDataInComponentSpaces)
		{
			// we need to update reference pose before retargeting. 
			// this is to ensure the skeleton has the latest pose you're looking at. 
			USkeletalMesh * PreviewMesh = NULL;
			if (OldSkeleton != NULL)
			{
				PreviewMesh = OldSkeleton->GetPreviewMesh(true);
				if (PreviewMesh)
				{
					OldSkeleton->UpdateReferencePoseFromMesh(PreviewMesh);
				}
			}
			
			PreviewMesh = NewSkeleton->GetPreviewMesh(true);
			if (PreviewMesh)
			{
				NewSkeleton->UpdateReferencePoseFromMesh(PreviewMesh);
			}
		}

		// anim sequences will be retargeted first becauseReplaceSkeleton forces it to change skeleton
		// @todo: please note that I think we can merge two loops 
		//(without separating two loops - one for AnimSequence and one for everybody else)
		// but if you have animation asssets that does replace skeleton, it will try fix up internal asset also
		// so I think you might be doing twice - look at AnimationAsset:ReplaceSkeleton
		// for safety, I'm doing Sequence first and then everything else
		// however this can be re-investigated and fixed better in the future
		for(auto Iter = AnimationAssetsToRetarget.CreateIterator(); Iter; ++Iter)
		{
			UAnimSequence* AnimSequenceToRetarget = Cast<UAnimSequence>(*Iter);
			if (AnimSequenceToRetarget)
			{
				// Copy curve data from source asset, preserving data in the target if present.
				if (OldSkeleton)
				{
					EditorAnimUtils::CopyAnimCurves(OldSkeleton, NewSkeleton, AnimSequenceToRetarget, USkeleton::AnimCurveMappingName, FRawCurveTracks::FloatType);

					// clear transform curves since those curves won't work in new skeleton
					// since we're deleting curves, mark this rebake flag off
					AnimSequenceToRetarget->RawCurveData.TransformCurves.Empty();
					AnimSequenceToRetarget->bNeedsRebake = false;
					// I can't copy transform curves yet because transform curves need retargeting. 
					//EditorAnimUtils::CopyAnimCurves(OldSkeleton, NewSkeleton, AssetToRetarget, USkeleton::AnimTrackCurveMappingName, FRawCurveTracks::TransformType);
				}
			}

			UAnimationAsset* AssetToRetarget = (*Iter);
			if (HasDuplicates())
			{
				AssetToRetarget->ReplaceReferredAnimations(DuplicatedAnimAssets);
			}
			AssetToRetarget->ReplaceSkeleton(NewSkeleton, bConvertAnimationDataInComponentSpaces);
			AssetToRetarget->MarkPackageDirty();
		}

		// Put duplicated and remapped assets in one list
		RemappedAnimAssets.Append(DuplicatedAnimAssets);

		// convert all Animation Blueprints and compile 
		for ( auto AnimBPIter = AnimBlueprintsToRetarget.CreateIterator(); AnimBPIter; ++AnimBPIter )
		{
			UAnimBlueprint * AnimBlueprint = (*AnimBPIter);

			AnimBlueprint->TargetSkeleton = NewSkeleton;

			if(RemappedAnimAssets.Num() > 0)
			{
				ReplaceReferredAnimationsInBlueprint(AnimBlueprint, RemappedAnimAssets);
			}

			bool bIsRegeneratingOnLoad = false;
			bool bSkipGarbageCollection = true;
			FBlueprintEditorUtils::RefreshAllNodes(AnimBlueprint);
			FKismetEditorUtilities::CompileBlueprint(AnimBlueprint, bIsRegeneratingOnLoad, bSkipGarbageCollection);
			AnimBlueprint->PostEditChange();
			AnimBlueprint->MarkPackageDirty();
		}
	}

	void FAnimationRetargetContext::AddRemappedAsset(UAnimationAsset* OriginalAsset, UAnimationAsset* NewAsset)
	{
		if(OriginalAsset->IsA(UAnimationAsset::StaticClass()) && NewAsset->IsA(UAnimationAsset::StaticClass()))
		{
			RemappedAnimAssets.Add(Cast<UAnimationAsset>(OriginalAsset), Cast<UAnimationAsset>(NewAsset));
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
	UObject* RetargetAnimations(USkeleton* OldSkeleton, USkeleton* NewSkeleton, TArray<TWeakObjectPtr<UObject>> AssetsToRetarget, bool bRetargetReferredAssets, const FNameDuplicationRule* NameRule, bool bConvertSpace)
	{
		FAnimationRetargetContext RetargetContext(AssetsToRetarget, bRetargetReferredAssets, bConvertSpace);
		return RetargetAnimations(OldSkeleton, NewSkeleton, RetargetContext, bRetargetReferredAssets, NameRule);
	}

	UObject* RetargetAnimations(USkeleton* OldSkeleton, USkeleton* NewSkeleton, const TArray<FAssetData>& AssetsToRetarget, bool bRetargetReferredAssets, const FNameDuplicationRule* NameRule, bool bConvertSpace)
	{
		FAnimationRetargetContext RetargetContext(AssetsToRetarget, bRetargetReferredAssets, bConvertSpace);
		return RetargetAnimations(OldSkeleton, NewSkeleton, RetargetContext, bRetargetReferredAssets, NameRule);
	}

	UObject* RetargetAnimations(USkeleton* OldSkeleton, USkeleton* NewSkeleton, FAnimationRetargetContext& RetargetContext, bool bRetargetReferredAssets, const FNameDuplicationRule* NameRule)
	{
		check(NewSkeleton);
		UObject* OriginalObject  = RetargetContext.GetSingleTargetObject();
		UPackage* DuplicationDestPackage = NewSkeleton->GetOutermost();

		if(	RetargetContext.HasAssetsToRetarget() )
		{
			if(NameRule)
			{
				RetargetContext.DuplicateAssetsToRetarget(DuplicationDestPackage, NameRule);
			}
			RetargetContext.RetargetAnimations(OldSkeleton, NewSkeleton);
		}

		FNotificationInfo Notification(FText::GetEmpty());
		Notification.ExpireDuration = 5.f;

		UObject* NotifyLinkObject = OriginalObject;
		if(OriginalObject && NameRule)
		{
			NotifyLinkObject = RetargetContext.GetDuplicate(OriginalObject);
		}

		if(!NameRule)
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

		// sync newly created objects on CB
		if (NotifyLinkObject)
		{
			TArray<UObject*> NewObjects = RetargetContext.GetAllDuplicates();
			TArray<FAssetData> CurrentSelection;
			for(auto& NewObject : NewObjects)
			{
				CurrentSelection.Add(FAssetData(NewObject));
			}

			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().SyncBrowserToAssets(CurrentSelection);
		}
		if(OriginalObject && NameRule)
		{
			return RetargetContext.GetDuplicate(OriginalObject);
		}
		return NULL;
	}

	FString CreateDesiredName(UObject* Asset, const FNameDuplicationRule* NameRule)
	{
		check(Asset);

		FString NewName = Asset->GetName();

		if(NameRule)
		{
			NewName = NameRule->Rename(Asset);
		}

		return NewName;
	}

	TMap<UObject*, UObject*> DuplicateAssetsInternal(const TArray<UObject*>& AssetsToDuplicate, UPackage* DestinationPackage, const FNameDuplicationRule* NameRule)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

		TMap<UObject*, UObject*> DuplicateMap;

		for(auto Iter = AssetsToDuplicate.CreateConstIterator(); Iter; ++Iter)
		{
			UObject* Asset = (*Iter);
			if(!DuplicateMap.Contains(Asset))
			{
				FString PathName = (NameRule)? NameRule->FolderPath : FPackageName::GetLongPackagePath(DestinationPackage->GetName());

				FString ObjectName;
				FString NewPackageName;
				AssetToolsModule.Get().CreateUniqueAssetName(PathName+"/"+ CreateDesiredName(Asset, NameRule), TEXT(""), NewPackageName, ObjectName);

				// create one on skeleton folder
				UObject* NewAsset = AssetToolsModule.Get().DuplicateAsset(ObjectName, PathName, Asset);
				if ( NewAsset )
				{
					DuplicateMap.Add(Asset, NewAsset);
				}
			}
		}

		return DuplicateMap;
	}

	void GetAllAnimationSequencesReferredInBlueprint(UAnimBlueprint* AnimBlueprint, TArray<UAnimationAsset*>& AnimationAssets)
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
					AnimNode->GetAllAnimationSequencesReferred(AnimationAssets);
				}
			}
		}

		// Pull all the assets we reference from variables
		TArray<UProperty*> SimpleAnimVariableProperties;
		TArray<UProperty*> ComplexAnimVariableProperties;
		TArray<UAnimSequence*> VariableReferencedSimpleAnims;
		TArray<UAnimationAsset*> VariableReferencedComplexAnims;

		UObject* DefaultObject = AnimBlueprint->GetAnimBlueprintGeneratedClass()->GetDefaultObject();
		GetBlueprintAssetVariableProperties(AnimBlueprint, SimpleAnimVariableProperties, ComplexAnimVariableProperties);
		GetAssetsFromProperties(SimpleAnimVariableProperties, DefaultObject, VariableReferencedSimpleAnims);
		GetAssetsFromProperties(ComplexAnimVariableProperties, DefaultObject, VariableReferencedComplexAnims);

		for(UAnimSequence* Sequence : VariableReferencedSimpleAnims)
		{
			AnimationAssets.AddUnique(Sequence);
		}

		for(UAnimationAsset* Asset : VariableReferencedComplexAnims)
		{
			AnimationAssets.AddUnique(Asset);
		}
	}

	void ReplaceReferredAnimationsInBlueprint(UAnimBlueprint* AnimBlueprint, const TMap<UAnimationAsset*, UAnimationAsset*>& AnimAssetReplacementMap)
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
					AnimNode->ReplaceReferredAnimations(AnimAssetReplacementMap);
				}
			}
		}

		// Push all the assets we reference from variables
		TArray<UProperty*> SimpleAnimVariableProperties;
		TArray<UProperty*> ComplexAnimVariableProperties;

		UObject* DefaultObject = AnimBlueprint->GetAnimBlueprintGeneratedClass()->GetDefaultObject();
		GetBlueprintAssetVariableProperties(AnimBlueprint, SimpleAnimVariableProperties, ComplexAnimVariableProperties);

		for(UProperty* SimpleProp : SimpleAnimVariableProperties)
		{
			if(UObject** ResolvedObject = SimpleProp->ContainerPtrToValuePtr<UObject*>(DefaultObject))
			{
				if(UAnimSequence* Sequence = Cast<UAnimSequence>(*ResolvedObject))
				{
					// Found an anim sequence variable that's set to a valid sequence
					if(UAnimationAsset* const* NewSequence = AnimAssetReplacementMap.Find(Sequence))
					{
						*ResolvedObject = *NewSequence;
					}
				}
			}
		}

		for(UProperty* ComplexProp : ComplexAnimVariableProperties)
		{
			if(UObject** ResolvedObject = ComplexProp->ContainerPtrToValuePtr<UObject*>(DefaultObject))
			{
				if(UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(*ResolvedObject))
				{
					// Found an anim sequence variable that's set to a valid sequence
					if(UAnimationAsset* const* NewAsset = AnimAssetReplacementMap.Find(AnimAsset))
					{
						*ResolvedObject = *NewAsset;
					}
				}
			}
		}
	}

	void GetBlueprintAssetVariableProperties(UAnimBlueprint* InBlueprint, TArray<UProperty*>& OutSimpleProperties, TArray<UProperty*>& OutComplexProperties)
	{
		OutSimpleProperties.Empty();
		OutComplexProperties.Empty();

		UAnimBlueprintGeneratedClass* GeneratedClass = InBlueprint->GetAnimBlueprintGeneratedClass();

		for(FBPVariableDescription& VarDesc : InBlueprint->NewVariables)
		{
			// Grab any variables directly referencing and anim sequence object.
			if(VarDesc.VarType.PinCategory == FString("object"))
			{
				if(UObject* VarSubObject = VarDesc.VarType.PinSubCategoryObject.Get())
				{
					// Get the object class
					if(UClass* SubObjectAsClass = Cast<UClass>(VarSubObject))
					{
						// Anything that IS an anim sequence is a simple anim
						// Anything that ISN'T an anim sequence but is an animation asset is a complex anim
						if(SubObjectAsClass == UAnimSequence::StaticClass())
						{
							if(UProperty* Prop = GeneratedClass->FindPropertyByName(VarDesc.VarName))
							{
								OutSimpleProperties.Add(Prop);
							}
						}
						else if(SubObjectAsClass->IsChildOf(UAnimationAsset::StaticClass()))
						{
							if(UProperty* Prop = GeneratedClass->FindPropertyByName(VarDesc.VarName))
							{
								OutComplexProperties.Add(Prop);
							}
						}
					}
				}
			}
		}
	}

	void CopyAnimCurves(USkeleton* OldSkeleton, USkeleton* NewSkeleton, UAnimSequenceBase *SequenceBase, const FName ContainerName, FRawCurveTracks::ESupportedCurveType CurveType )
	{
		// Copy curve data from source asset, preserving data in the target if present.
		const FSmartNameMapping* OldNameMapping = OldSkeleton->GetSmartNameContainer(ContainerName);
		SequenceBase->RawCurveData.RefreshName(OldNameMapping, CurveType);

		switch (CurveType)
		{
		case FRawCurveTracks::FloatType:
			{
				for(FFloatCurve& Curve : SequenceBase->RawCurveData.FloatCurves)
				{
					NewSkeleton->AddSmartNameAndModify(ContainerName, Curve.Name.DisplayName, Curve.Name);
				}
				break;
			}
		case FRawCurveTracks::VectorType:
			{
				for(FVectorCurve& Curve : SequenceBase->RawCurveData.VectorCurves)
				{
					NewSkeleton->AddSmartNameAndModify(ContainerName, Curve.Name.DisplayName, Curve.Name);
				}
				break;
			}
		case FRawCurveTracks::TransformType:
			{
				for(FTransformCurve& Curve : SequenceBase->RawCurveData.TransformCurves)
				{
					NewSkeleton->AddSmartNameAndModify(ContainerName, Curve.Name.DisplayName, Curve.Name);
				}
				break;
			}
		}
	}

	FString FNameDuplicationRule::Rename(const UObject* Asset) const
	{
		check(Asset);

		FString NewName = Asset->GetName();

		NewName = NewName.Replace(*ReplaceFrom, *ReplaceTo);
		return FString::Printf(TEXT("%s%s%s"), *Prefix, *NewName, *Suffix);
	}
}

#undef LOCTEXT_NAMESPACE
