// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "UMGSequencerObjectBindingManager.h"
#include "WidgetBlueprintEditor.h"
#include "ISequencer.h"
#include "MovieScene.h"
#include "WidgetBlueprintEditor.h"
#include "Components/PanelSlot.h"
#include "Components/Visual.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "WidgetBlueprint.h"

#define LOCTEXT_NAMESPACE "UMGSequencerObjectBindingManager"

FUMGSequencerObjectBindingManager::FUMGSequencerObjectBindingManager( FWidgetBlueprintEditor& InWidgetBlueprintEditor, UWidgetAnimation& InAnimation )
	: WidgetAnimation( &InAnimation )
	, WidgetBlueprintEditor( InWidgetBlueprintEditor )
{
	WidgetBlueprintEditor.GetOnWidgetPreviewUpdated().AddRaw( this, &FUMGSequencerObjectBindingManager::OnWidgetPreviewUpdated );

}

FUMGSequencerObjectBindingManager::~FUMGSequencerObjectBindingManager()
{
	WidgetBlueprintEditor.GetOnWidgetPreviewUpdated().RemoveAll( this );
}

void FUMGSequencerObjectBindingManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(WidgetAnimation);
}

FGuid FUMGSequencerObjectBindingManager::FindGuidForObject( const UMovieScene& MovieScene, UObject& Object ) const
{
	return PreviewObjectToGuidMap.FindRef( &Object );
}

bool FUMGSequencerObjectBindingManager::CanPossessObject( UObject& Object ) const
{
	UWidgetBlueprint* WidgetBlueprint = WidgetBlueprintEditor.GetWidgetBlueprintObj();

	// Only preview widgets in this blueprint can be possessed
	UUserWidget* PreviewWidget = WidgetBlueprintEditor.GetPreview();

	return Object.IsA<UVisual>() && Object.IsIn( PreviewWidget );
}

void FUMGSequencerObjectBindingManager::BindPossessableObject( const FGuid& PossessableGuid, UObject& PossessedObject )
{
	PreviewObjectToGuidMap.Add( &PossessedObject, PossessableGuid );
	GuidToPreviewObjectsMap.Add(PossessableGuid, &PossessedObject);

	FWidgetAnimationBinding NewBinding;
	NewBinding.AnimationGuid = PossessableGuid;

	UPanelSlot* Slot = Cast<UPanelSlot>( &PossessedObject );
	if( Slot && Slot->Content )
	{
		// Save the name of the widget containing the slots.  This is the object to look up that contains the slot itself(the thing we are animating)
		NewBinding.SlotWidgetName = Slot->GetFName();
		NewBinding.WidgetName = Slot->Content->GetFName();


		WidgetAnimation->AnimationBindings.Add(NewBinding);
	}
	else if( !Slot )
	{
		NewBinding.WidgetName = PossessedObject.GetFName();

		WidgetAnimation->AnimationBindings.Add(NewBinding);
	}

}

void FUMGSequencerObjectBindingManager::UnbindPossessableObjects( const FGuid& PossessableGuid )
{
	TArray<TWeakObjectPtr<UObject>> PreviewObjects;
	GuidToPreviewObjectsMap.MultiFind(PossessableGuid, PreviewObjects);
	for (TWeakObjectPtr<UObject>& PreviewObject : PreviewObjects)
	{
		PreviewObjectToGuidMap.Remove(PreviewObject);
	}
	GuidToPreviewObjectsMap.Remove(PossessableGuid);

	UWidgetBlueprint* WidgetBlueprint = WidgetBlueprintEditor.GetWidgetBlueprintObj();

	WidgetAnimation->AnimationBindings.RemoveAll( [&]( const FWidgetAnimationBinding& Binding ) { return Binding.AnimationGuid == PossessableGuid; } );

}

void FUMGSequencerObjectBindingManager::GetRuntimeObjects( const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects ) const
{
	TArray<TWeakObjectPtr<UObject>> PreviewObjects;
	GuidToPreviewObjectsMap.MultiFind(ObjectGuid, PreviewObjects);
	for (TWeakObjectPtr<UObject>& PreviewObject : PreviewObjects)
	{
		OutRuntimeObjects.Add(PreviewObject.Get());
	}
}

bool FUMGSequencerObjectBindingManager::TryGetObjectBindingDisplayName(const FGuid& ObjectGuid, FText& DisplayName) const
{
	// TODO: This gets called every frame for every bound object and could be a potential performance issue for a really complicated animation.
	TArray<TWeakObjectPtr<UObject>> BindingObjects;
	GuidToPreviewObjectsMap.MultiFind(ObjectGuid, BindingObjects);
	if (BindingObjects.Num() == 0)
	{
		DisplayName = LOCTEXT("NoBoundObjects", "Invalid bound object");
	}
	else if (BindingObjects.Num() == 1)
	{
		UPanelSlot* SlotObject = Cast<UPanelSlot>(BindingObjects[0].Get());
		if (SlotObject != nullptr)
		{
			FText PanelName = SlotObject->Parent != nullptr 
				? FText::FromString(SlotObject->Parent->GetName())
				: LOCTEXT("InvalidPanel", "Invalid Panel");
			FText ContentName = SlotObject->Content != nullptr
				? FText::FromString(SlotObject->Content->GetName())
				: LOCTEXT("EmptyPanel", "Empty");
			DisplayName = FText::Format(LOCTEXT("SlotObject", "{0} ({1} Slot)"), ContentName, PanelName);
		}
		else
		{
			DisplayName = FText::FromString(BindingObjects[0].Get()->GetName());
		}
	}
	else
	{
		DisplayName = LOCTEXT("Multiple bound objects", "Multilple bound objects");
	}
	return true;
}

bool FUMGSequencerObjectBindingManager::HasValidWidgetAnimation() const
{
	UWidgetBlueprint* WidgetBlueprint = WidgetBlueprintEditor.GetWidgetBlueprintObj();
	return WidgetAnimation != nullptr && WidgetBlueprint->Animations.Contains( WidgetAnimation );
}

void FUMGSequencerObjectBindingManager::InitPreviewObjects()
{
	// Populate preview object to guid map
	UUserWidget* PreviewWidget = WidgetBlueprintEditor.GetPreview();

	if( PreviewWidget )
	{
		UWidgetTree* WidgetTree = PreviewWidget->WidgetTree;

		for(const FWidgetAnimationBinding& Binding : WidgetAnimation->AnimationBindings)
		{
			UObject* FoundObject = Binding.FindRuntimeObject( *WidgetTree );
			if( FoundObject )
			{
				PreviewObjectToGuidMap.Add(FoundObject, Binding.AnimationGuid);
				GuidToPreviewObjectsMap.Add(Binding.AnimationGuid, FoundObject);
			}
		}
	}
}

void FUMGSequencerObjectBindingManager::OnWidgetPreviewUpdated()
{
	PreviewObjectToGuidMap.Empty();
	GuidToPreviewObjectsMap.Empty();

	InitPreviewObjects();

	WidgetBlueprintEditor.GetSequencer()->UpdateRuntimeInstances();
}

#undef LOCTEXT_NAMESPACE